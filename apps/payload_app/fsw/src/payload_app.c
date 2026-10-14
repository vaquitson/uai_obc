#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfe.h"

#include "payload_api_client.h"
#include "payload_api_json.h"
#include "payload_app_eventids.h"
#include "payload_app_fcncodes.h"
#include "payload_app_msg.h"
#include "payload_app_msgdefs.h"
#include "payload_app_tlm.h"
#include "payload_app_worker.h"
#include "payload_app.h"

/* Compile-time layout checks: no implicit padding on any target (see msgdefs) */
#define PAYLOAD_APP_SIZE_CHECK(name, cond) typedef char name[(cond) ? 1 : -1]
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_HkPayloadSize_t, sizeof(PAYLOAD_APP_HkTlm_Payload_t) == 64);
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_SessionInfoCmdSize_t, sizeof(PAYLOAD_APP_SessionInfoCmd_Payload_t) == 20);
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_ReadRangeCmdSize_t, sizeof(PAYLOAD_APP_ReadRangeCmd_Payload_t) == 28);
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_LivePayloadSize_t, sizeof(PAYLOAD_APP_LiveTlm_Payload_t) == 80);
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_SessionPayloadSize_t, sizeof(PAYLOAD_APP_SessionTlm_Payload_t) == 80);
PAYLOAD_APP_SIZE_CHECK(PAYLOAD_APP_ChunkPayloadSize_t,
                       sizeof(PAYLOAD_APP_ChunkTlm_Payload_t) == 40 + PAYLOAD_APP_CHUNK_TLM_MAX);

#if PAYLOAD_APP_CHUNK_TLM_MAX % 8 != 0 || PAYLOAD_APP_CHUNK_TLM_MAX > 0xFFFF
#error "PAYLOAD_APP_CHUNK_TLM_MAX must be a multiple of 8 (no implicit padding) and fit n_bytes"
#endif

/* READ_SESSION_RANGE length bounds from the API contract */
#define PAYLOAD_APP_READ_LENGTH_MIN 1
#define PAYLOAD_APP_READ_LENGTH_MAX 4096

PAYLOAD_APP_GlobalApp_t PAYLOAD_APP_Global;

const char *PAYLOAD_APP_OpName(uint8 op)
{
  switch (op)
  {
    case PAYLOAD_APP_GET_STATUS_CC:
      return "GET_STATUS";
    case PAYLOAD_APP_START_ACQUISITION_CC:
      return "START_ACQUISITION";
    case PAYLOAD_APP_STOP_ACQUISITION_CC:
      return "STOP_ACQUISITION";
    case PAYLOAD_APP_GET_LIVE_STATE_CC:
      return "GET_LIVE_STATE";
    case PAYLOAD_APP_LIST_SESSIONS_CC:
      return "LIST_SESSIONS";
    case PAYLOAD_APP_GET_SESSION_INFO_CC:
      return "GET_SESSION_INFO";
    case PAYLOAD_APP_READ_SESSION_RANGE_CC:
      return "READ_SESSION_RANGE";
    default:
      return NULL;
  }
}

void PAYLOAD_APP_HkLock(void)
{
  OS_MutSemTake(PAYLOAD_APP_Global.hk_mutex);
}

void PAYLOAD_APP_HkUnlock(void)
{
  OS_MutSemGive(PAYLOAD_APP_Global.hk_mutex);
}

static void PAYLOAD_APP_CountCmd(bool ok)
{
  PAYLOAD_APP_HkLock();
  if (ok)
    PAYLOAD_APP_Global.hk_packet.payload.cmd_counter++;
  else
    PAYLOAD_APP_Global.hk_packet.payload.err_counter++;
  PAYLOAD_APP_HkUnlock();
}

static void PAYLOAD_APP_InitHk(void)
{
  PAYLOAD_APP_HkTlm_Payload_t *hk = &PAYLOAD_APP_Global.hk_packet.payload;

  memset(hk, 0, sizeof(*hk));
  hk->payload_state           = PAYLOAD_APP_UNKNOWN;
  hk->receiver_state          = PAYLOAD_APP_UNKNOWN;
  hk->logger_state            = PAYLOAD_APP_UNKNOWN;
  hk->live_consumer_connected = PAYLOAD_APP_UNKNOWN;
  hk->version_ok              = PAYLOAD_APP_UNKNOWN;
  hk->tlm_layout_version      = PAYLOAD_APP_TLM_LAYOUT_VERSION;
}

static void PAYLOAD_APP_InitSocketPath(void)
{
  const char *env    = getenv(PAYLOAD_APP_SOCKET_PATH_ENV);
  const char *source = "default";

  snprintf(PAYLOAD_APP_Global.socket_path, sizeof(PAYLOAD_APP_Global.socket_path), "%s",
           PAYLOAD_APP_SOCKET_PATH_DEFAULT);

  if (env != NULL && env[0] != '\0')
  {
    if (strlen(env) < sizeof(PAYLOAD_APP_Global.socket_path))
    {
      snprintf(PAYLOAD_APP_Global.socket_path, sizeof(PAYLOAD_APP_Global.socket_path), "%s", env);
      source = "env";
    }
    else
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                        "%s too long (%lu chars, max %lu), using default", PAYLOAD_APP_SOCKET_PATH_ENV,
                        (unsigned long)strlen(env), (unsigned long)sizeof(PAYLOAD_APP_Global.socket_path) - 1);
    }
  }

  CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_PATH_EID, CFE_EVS_EventType_INFORMATION, "Socket (%s): %s", source,
                    PAYLOAD_APP_Global.socket_path);
}

/* Never blocks: a full queue drops the request with an event */
static bool PAYLOAD_APP_Enqueue(const PAYLOAD_APP_Request_t *req)
{
  int32 status = OS_QueuePut(PAYLOAD_APP_Global.req_queue, req, sizeof(*req), 0);

  if (status == OS_SUCCESS)
    return true;

  if (status == OS_QUEUE_FULL)
    CFE_EVS_SendEvent(PAYLOAD_APP_QUEUE_FULL_EID, CFE_EVS_EventType_ERROR,
                      "%s dropped: request queue full (%d)", PAYLOAD_APP_OpName(req->op),
                      PAYLOAD_APP_REQ_QUEUE_DEPTH);
  else
    CFE_EVS_SendEvent(PAYLOAD_APP_WORKER_ERR_EID, CFE_EVS_EventType_ERROR, "%s dropped: queue put RC = %ld",
                      PAYLOAD_APP_OpName(req->op), (long)status);
  return false;
}

CFE_Status_t PAYLOAD_APP_Init(void)
{
  int32                 status;
  PAYLOAD_APP_Request_t startup_req;

  memset(&PAYLOAD_APP_Global, 0, sizeof(PAYLOAD_APP_Global));

  PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_RUN;

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS)
    CFE_ES_WriteToSysLog("PAYLOAD_APP: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header),
                         CFE_SB_ValueToMsgId(PAYLOAD_APP_TLM_MID), sizeof(PAYLOAD_APP_Global.hk_packet));
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error initializing HK msg, RC = 0x%08lX", (unsigned long)status);
  PAYLOAD_APP_InitHk();
  PAYLOAD_APP_TlmInit();

  PAYLOAD_APP_InitSocketPath();

  status = CFE_SB_CreatePipe(&PAYLOAD_APP_Global.cmd_pipe, PAYLOAD_APP_PIPE_DEPTH, PAYLOAD_APP_PIPE_NAME);
  if (status != CFE_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating cmd pipe, RC = 0x%08lX", (unsigned long)status);
    return status;
  }

  status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(PAYLOAD_APP_CMD_MID), PAYLOAD_APP_Global.cmd_pipe);
  if (status != CFE_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error subscribing to commands, RC = 0x%08lX", (unsigned long)status);
    return status;
  }

  status = OS_MutSemCreate(&PAYLOAD_APP_Global.hk_mutex, PAYLOAD_APP_MUTEX_NAME, 0);
  if (status != OS_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating HK mutex, RC = %ld", (long)status);
    return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
  }

  status = OS_QueueCreate(&PAYLOAD_APP_Global.req_queue, PAYLOAD_APP_QUEUE_NAME, PAYLOAD_APP_REQ_QUEUE_DEPTH,
                          sizeof(PAYLOAD_APP_Request_t), 0);
  if (status != OS_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating request queue, RC = %ld", (long)status);
    return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
  }

  status = CFE_ES_CreateChildTask(&PAYLOAD_APP_Global.worker_task, PAYLOAD_APP_WORKER_NAME, PAYLOAD_APP_WorkerMain,
                                  CFE_ES_TASK_STACK_ALLOCATE, PAYLOAD_APP_WORKER_STACK, PAYLOAD_APP_WORKER_PRIORITY,
                                  0);
  if (status != CFE_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating worker task, RC = 0x%08lX", (unsigned long)status);
    return status;
  }

  /* Startup GET_STATUS: checks interface_version and fills HK; probed in the background until Payload answers */
  memset(&startup_req, 0, sizeof(startup_req));
  startup_req.op    = PAYLOAD_APP_GET_STATUS_CC;
  startup_req.probe = 1;
  PAYLOAD_APP_Enqueue(&startup_req);

  CFE_EVS_SendEvent(PAYLOAD_APP_INIT_SUCCESSFUL_EID, CFE_EVS_EventType_INFORMATION,
                     "PAYLOAD_APP: Initialized successfully, RC = 0x%08lX", (unsigned long)CFE_SUCCESS);

  return CFE_SUCCESS;
}

void PAYLOAD_APP_AppMain(void)
{
  int32            status;
  CFE_SB_Buffer_t *sb_buf_ptr;
  uint64_t         now;
  uint64_t         next_hk;

  status = PAYLOAD_APP_Init();
  if (status != CFE_SUCCESS)
    PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;

  next_hk = PAYLOAD_API_NowMs() + PAYLOAD_APP_HK_PERIOD_MS;

  while (CFE_ES_RunLoop(&PAYLOAD_APP_Global.run_status) == true)
  {
    /* Wait for commands only until the next HK is due, so HK keeps its period under command load */
    now    = PAYLOAD_API_NowMs();
    status = CFE_SB_ReceiveBuffer(&sb_buf_ptr, PAYLOAD_APP_Global.cmd_pipe,
                                  (next_hk > now) ? (int32)(next_hk - now) : CFE_SB_POLL);

    if (status == CFE_SUCCESS)
      PAYLOAD_APP_ProcessCommandPacket(sb_buf_ptr);
    else if (status != CFE_SB_TIME_OUT && status != CFE_SB_NO_MESSAGE)
    {
      CFE_ES_WriteToSysLog("PAYLOAD_APP: SB pipe read error, RC = 0x%08lX\n", (unsigned long)status);
      PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;
    }

    now = PAYLOAD_API_NowMs();
    if (now >= next_hk)
    {
      PAYLOAD_APP_SendHk();
      next_hk += PAYLOAD_APP_HK_PERIOD_MS;
      if (next_hk <= now)
        next_hk = now + PAYLOAD_APP_HK_PERIOD_MS;
    }
  }

  CFE_ES_ExitApp(PAYLOAD_APP_Global.run_status);
}

void PAYLOAD_APP_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_SB_MsgId_t msg_id = CFE_SB_INVALID_MSG_ID;

  CFE_MSG_GetMsgId(&sb_buf_ptr->Msg, &msg_id);

  switch (CFE_SB_MsgIdToValue(msg_id))
  {
    case PAYLOAD_APP_CMD_MID:
      PAYLOAD_APP_ProcessGroundCommand(sb_buf_ptr);
      break;

    default:
      CFE_EVS_SendEvent(PAYLOAD_APP_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: invalid command pipe message ID: 0x%X",
                         (unsigned int)CFE_SB_MsgIdToValue(msg_id));
      break;
  }
}

static bool PAYLOAD_APP_VerifyLength(const CFE_SB_Buffer_t *sb_buf_ptr, size_t expected)
{
  CFE_MSG_Size_t    actual   = 0;
  CFE_MSG_FcnCode_t fcn_code = 0;

  CFE_MSG_GetSize(&sb_buf_ptr->Msg, &actual);
  if (actual == expected)
    return true;

  CFE_MSG_GetFcnCode(&sb_buf_ptr->Msg, &fcn_code);
  CFE_EVS_SendEvent(PAYLOAD_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                    "Invalid length for CC %u: %lu B, expected %lu B", (unsigned int)fcn_code,
                    (unsigned long)actual, (unsigned long)expected);
  PAYLOAD_APP_CountCmd(false);
  return false;
}

static void PAYLOAD_APP_Noop(void)
{
  PAYLOAD_APP_CountCmd(true);
  CFE_EVS_SendEvent(PAYLOAD_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION, "PAYLOAD_APP: NOOP command received");
}

static void PAYLOAD_APP_ResetCounters(void)
{
  PAYLOAD_APP_HkLock();
  PAYLOAD_APP_Global.hk_packet.payload.cmd_counter   = 0;
  PAYLOAD_APP_Global.hk_packet.payload.err_counter   = 0;
  PAYLOAD_APP_Global.hk_packet.payload.req_ok_count  = 0;
  PAYLOAD_APP_Global.hk_packet.payload.req_err_count = 0;
  PAYLOAD_APP_HkUnlock();
  CFE_EVS_SendEvent(PAYLOAD_APP_RESET_COUNTERS_EID, CFE_EVS_EventType_INFORMATION,
                     "PAYLOAD_APP: counters reset");
}

static void PAYLOAD_APP_QueueOp(uint8 op)
{
  PAYLOAD_APP_Request_t req;

  memset(&req, 0, sizeof(req));
  req.op = op;
  PAYLOAD_APP_CountCmd(PAYLOAD_APP_Enqueue(&req));
}

/* Copies a command session_id; it must be NUL-terminated and look like YYYYMMDDTHHMMSSZ */
static bool PAYLOAD_APP_ValidSessionId(uint8 op, const char *raw, char *out)
{
  char shown[PAYLOAD_APP_SESSION_ID_LEN];
  int  i;

  if (memchr(raw, '\0', PAYLOAD_APP_SESSION_ID_LEN) != NULL && PAYLOAD_JSON_IsValidSessionId(raw))
  {
    memcpy(out, raw, PAYLOAD_APP_SESSION_ID_LEN);
    return true;
  }

  for (i = 0; i < PAYLOAD_APP_SESSION_ID_LEN - 1 && raw[i] != '\0'; i++)
    shown[i] = (raw[i] >= 0x20 && raw[i] < 0x7F) ? raw[i] : '?';
  shown[i] = '\0';
  CFE_EVS_SendEvent(PAYLOAD_APP_VALIDATION_ERR_EID, CFE_EVS_EventType_ERROR,
                    "%s rejected: session_id '%s' is not YYYYMMDDTHHMMSSZ", PAYLOAD_APP_OpName(op), shown);
  return false;
}

static void PAYLOAD_APP_QueueSessionInfo(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  const PAYLOAD_APP_SessionInfoCmd_t *cmd = (const PAYLOAD_APP_SessionInfoCmd_t *)sb_buf_ptr;
  PAYLOAD_APP_Request_t               req;

  memset(&req, 0, sizeof(req));
  req.op = PAYLOAD_APP_GET_SESSION_INFO_CC;
  if (!PAYLOAD_APP_ValidSessionId(req.op, cmd->payload.session_id, req.session_id))
  {
    PAYLOAD_APP_CountCmd(false);
    return;
  }
  PAYLOAD_APP_CountCmd(PAYLOAD_APP_Enqueue(&req));
}

static void PAYLOAD_APP_QueueReadRange(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  const PAYLOAD_APP_ReadRangeCmd_t *cmd = (const PAYLOAD_APP_ReadRangeCmd_t *)sb_buf_ptr;
  PAYLOAD_APP_Request_t             req;

  memset(&req, 0, sizeof(req));
  req.op = PAYLOAD_APP_READ_SESSION_RANGE_CC;
  if (!PAYLOAD_APP_ValidSessionId(req.op, cmd->payload.session_id, req.session_id))
  {
    PAYLOAD_APP_CountCmd(false);
    return;
  }
  /* offset is a uint32, so offset >= 0 always holds */
  if (cmd->payload.length < PAYLOAD_APP_READ_LENGTH_MIN || cmd->payload.length > PAYLOAD_APP_READ_LENGTH_MAX)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_VALIDATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "READ_SESSION_RANGE rejected: length %u outside %u..%u", (unsigned int)cmd->payload.length,
                      PAYLOAD_APP_READ_LENGTH_MIN, PAYLOAD_APP_READ_LENGTH_MAX);
    PAYLOAD_APP_CountCmd(false);
    return;
  }
  req.offset = cmd->payload.offset;
  req.length = cmd->payload.length;
  PAYLOAD_APP_CountCmd(PAYLOAD_APP_Enqueue(&req));
}

void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_MSG_FcnCode_t fcn_code = 0;

  CFE_MSG_GetFcnCode(&sb_buf_ptr->Msg, &fcn_code);

  switch (fcn_code)
  {
    case PAYLOAD_APP_NOOP_CC:
      if (PAYLOAD_APP_VerifyLength(sb_buf_ptr, sizeof(PAYLOAD_APP_NoArgsCmd_t)))
        PAYLOAD_APP_Noop();
      break;

    case PAYLOAD_APP_RESET_COUNTERS_CC:
      if (PAYLOAD_APP_VerifyLength(sb_buf_ptr, sizeof(PAYLOAD_APP_NoArgsCmd_t)))
        PAYLOAD_APP_ResetCounters();
      break;

    case PAYLOAD_APP_GET_STATUS_CC:
    case PAYLOAD_APP_START_ACQUISITION_CC:
    case PAYLOAD_APP_STOP_ACQUISITION_CC:
    case PAYLOAD_APP_GET_LIVE_STATE_CC:
    case PAYLOAD_APP_LIST_SESSIONS_CC:
      if (PAYLOAD_APP_VerifyLength(sb_buf_ptr, sizeof(PAYLOAD_APP_NoArgsCmd_t)))
        PAYLOAD_APP_QueueOp((uint8)fcn_code);
      break;

    case PAYLOAD_APP_GET_SESSION_INFO_CC:
      if (PAYLOAD_APP_VerifyLength(sb_buf_ptr, sizeof(PAYLOAD_APP_SessionInfoCmd_t)))
        PAYLOAD_APP_QueueSessionInfo(sb_buf_ptr);
      break;

    case PAYLOAD_APP_READ_SESSION_RANGE_CC:
      if (PAYLOAD_APP_VerifyLength(sb_buf_ptr, sizeof(PAYLOAD_APP_ReadRangeCmd_t)))
        PAYLOAD_APP_QueueReadRange(sb_buf_ptr);
      break;

    default:
      PAYLOAD_APP_CountCmd(false);
      CFE_EVS_SendEvent(PAYLOAD_APP_INVALID_CC_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: invalid command code: %u", (unsigned int)fcn_code);
      break;
  }
}

void PAYLOAD_APP_SendHk(void)
{
  int32 status;

  PAYLOAD_APP_HkLock();
  status = CFE_SB_TransmitMsg(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header), true);
  PAYLOAD_APP_HkUnlock();

  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_HK_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: HK transmit error, RC = 0x%08lX", (unsigned long)status);
}
