#include <stdio.h>
#include <string.h>

#include "cfe.h"

#include "payload_api_client.h"
#include "payload_api_json.h"
#include "payload_app.h"
#include "payload_app_eventids.h"
#include "payload_app_fcncodes.h"
#include "payload_app_tlm.h"
#include "payload_app_worker.h"

/* Request JSON: envelope + the largest args object (READ_SESSION_RANGE) */
#define PAYLOAD_APP_TX_BUF_SIZE 512

/* Worker-only buffers: static so they never live on the child task stack */
static char      PAYLOAD_APP_TxBuf[PAYLOAD_APP_TX_BUF_SIZE];
static char      PAYLOAD_APP_RxBuf[PAYLOAD_APP_RX_BUF_SIZE];
static jsmntok_t PAYLOAD_APP_Tokens[PAYLOAD_APP_JSON_MAX_TOKENS];

typedef struct {
  uint32 next_request_id;
  bool   service_down;       /* last connect() failed */
  bool   probing;            /* background GET_STATUS probes until Payload answers again */
  bool   revalidate;         /* recovered through another command: re-check interface_version */
  int    last_connect_errno; /* errno already reported, for event throttling */
  uint32 connect_backoff_ms;
} PAYLOAD_APP_WorkerState_t;

static PAYLOAD_APP_WorkerState_t PAYLOAD_APP_Worker;

static void PAYLOAD_APP_ReqOk(void)
{
  PAYLOAD_APP_HkLock();
  PAYLOAD_APP_Global.hk_packet.payload.req_ok_count++;
  PAYLOAD_APP_Global.hk_packet.payload.last_status_code = PAYLOAD_API_STATUS_OK;
  PAYLOAD_APP_HkUnlock();
}

/* status_code 0 means a transport/parse failure: last_status_code is left as is */
static void PAYLOAD_APP_ReqErr(uint8 status_code)
{
  PAYLOAD_APP_HkLock();
  PAYLOAD_APP_Global.hk_packet.payload.req_err_count++;
  if (status_code != PAYLOAD_API_STATUS_OK)
    PAYLOAD_APP_Global.hk_packet.payload.last_status_code = status_code;
  PAYLOAD_APP_HkUnlock();
}

static void PAYLOAD_APP_SetServiceAvailable(uint8 available)
{
  PAYLOAD_APP_HkLock();
  PAYLOAD_APP_Global.hk_packet.payload.service_available = available;
  PAYLOAD_APP_HkUnlock();
}

static void PAYLOAD_APP_GrowBackoff(void)
{
  PAYLOAD_APP_Worker.connect_backoff_ms *= 2;
  if (PAYLOAD_APP_Worker.connect_backoff_ms > PAYLOAD_APP_CONNECT_BACKOFF_MAX_MS)
    PAYLOAD_APP_Worker.connect_backoff_ms = PAYLOAD_APP_CONNECT_BACKOFF_MAX_MS;
}

/* One event per outage and per errno change; the rest of the retries and probes stay silent */
static void PAYLOAD_APP_ReportConnectFailure(PAYLOAD_API_Result_t result, int err_no)
{
  if (!PAYLOAD_APP_Worker.service_down || err_no != PAYLOAD_APP_Worker.last_connect_errno)
  {
    if (result == PAYLOAD_API_ERR_EACCES)
      CFE_EVS_SendEvent(PAYLOAD_APP_SERVICE_UNAVAILABLE_EID, CFE_EVS_EventType_ERROR,
                        "Payload unreachable: EACCES (errno %d), cFS user must be in group cubesatuai", err_no);
    else
      CFE_EVS_SendEvent(PAYLOAD_APP_SERVICE_UNAVAILABLE_EID, CFE_EVS_EventType_ERROR,
                        "Payload unreachable: %s (errno %d), retrying with backoff",
                        PAYLOAD_API_ResultName(result), err_no);
  }

  PAYLOAD_APP_Worker.service_down       = true;
  PAYLOAD_APP_Worker.last_connect_errno = err_no;
  PAYLOAD_APP_SetServiceAvailable(0);
}

/* Returns true when this connect() ends an outage */
static bool PAYLOAD_APP_ReportConnected(const char *op)
{
  bool recovered = PAYLOAD_APP_Worker.service_down || PAYLOAD_APP_Worker.probing;

  if (recovered)
    CFE_EVS_SendEvent(PAYLOAD_APP_SERVICE_RECOVERED_EID, CFE_EVS_EventType_INFORMATION,
                      "Payload service reachable again (%s)", op);

  PAYLOAD_APP_Worker.service_down       = false;
  PAYLOAD_APP_Worker.probing            = false;
  PAYLOAD_APP_Worker.last_connect_errno = 0;
  PAYLOAD_APP_Worker.connect_backoff_ms = PAYLOAD_APP_CONNECT_BACKOFF_MIN_MS;
  PAYLOAD_APP_SetServiceAvailable(1);
  return recovered;
}

static void PAYLOAD_APP_ReportTransportError(const char *op, uint32 rid, PAYLOAD_API_Result_t result, int err_no,
                                             const char *header, uint32 timeout_ms)
{
  switch (result)
  {
    case PAYLOAD_API_ERR_TIMEOUT:
      CFE_EVS_SendEvent(PAYLOAD_APP_REQ_TIMEOUT_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu timed out after %lu ms",
                        op, (unsigned long)rid, (unsigned long)timeout_ms);
      break;
    case PAYLOAD_API_ERR_HEADER:
      CFE_EVS_SendEvent(PAYLOAD_APP_FRAME_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: malformed header '%s'", op,
                        (unsigned long)rid, header);
      break;
    case PAYLOAD_API_ERR_TOO_BIG:
      CFE_EVS_SendEvent(PAYLOAD_APP_FRAME_ERR_EID, CFE_EVS_EventType_ERROR,
                        "%s rid=%lu: frame size '%s' exceeds %lu B buffer", op, (unsigned long)rid, header,
                        (unsigned long)(PAYLOAD_APP_RX_BUF_SIZE - 1));
      break;
    case PAYLOAD_API_ERR_CLOSED:
      CFE_EVS_SendEvent(PAYLOAD_APP_FRAME_ERR_EID, CFE_EVS_EventType_ERROR,
                        "%s rid=%lu: connection closed by Payload (errno %d)", op, (unsigned long)rid, err_no);
      break;
    default:
      CFE_EVS_SendEvent(PAYLOAD_APP_FRAME_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: %s error (errno %d)", op,
                        (unsigned long)rid, PAYLOAD_API_ResultName(result), err_no);
      break;
  }
}

static uint8 PAYLOAD_APP_StateOrUnknown(uint32 valid, uint32 bit, uint8 value)
{
  return (valid & bit) ? value : PAYLOAD_APP_UNKNOWN;
}

/* Copies a GET_STATUS-style object into HK; reports the interface_version check when asked or when it changes */
static void PAYLOAD_APP_ApplyStatus(const PAYLOAD_JSON_Doc_t *doc, int obj, bool report_version)
{
  PAYLOAD_JSON_Status_t        st;
  PAYLOAD_APP_HkTlm_Payload_t *hk = &PAYLOAD_APP_Global.hk_packet.payload;
  uint8                        version_ok;
  bool                         changed;

  PAYLOAD_JSON_ParseStatus(doc, obj, &st);
  version_ok = ((st.valid & PAYLOAD_JSON_STATUS_VERSION) &&
                strcmp(st.interface_version, PAYLOAD_APP_INTERFACE_VERSION) == 0)
                   ? 1
                   : 0;

  PAYLOAD_APP_HkLock();
  hk->payload_state = PAYLOAD_APP_StateOrUnknown(st.valid, PAYLOAD_JSON_STATUS_PAYLOAD_STATE, st.payload_state);
  hk->receiver_state = PAYLOAD_APP_StateOrUnknown(st.valid, PAYLOAD_JSON_STATUS_RECEIVER_STATE, st.receiver_state);
  hk->logger_state   = PAYLOAD_APP_StateOrUnknown(st.valid, PAYLOAD_JSON_STATUS_LOGGER_STATE, st.logger_state);
  hk->live_consumer_connected =
      PAYLOAD_APP_StateOrUnknown(st.valid, PAYLOAD_JSON_STATUS_LIVE_CONSUMER, st.live_consumer_connected);
  hk->live_aircraft_count = (st.valid & PAYLOAD_JSON_STATUS_LIVE_COUNT) ? st.live_aircraft_count : 0;
  hk->storage_free_bytes  = (st.valid & PAYLOAD_JSON_STATUS_STORAGE_FREE) ? st.storage_free_bytes : 0;

  memset(hk->active_session_id, 0, sizeof(hk->active_session_id));
  if (st.valid & PAYLOAD_JSON_STATUS_ACTIVE_SESSION)
    strncpy(hk->active_session_id, st.active_session_id, sizeof(hk->active_session_id) - 1);

  memset(hk->interface_version, 0, sizeof(hk->interface_version));
  if (st.valid & PAYLOAD_JSON_STATUS_VERSION)
    strncpy(hk->interface_version, st.interface_version, sizeof(hk->interface_version) - 1);

  changed        = (hk->version_ok != version_ok);
  hk->version_ok = version_ok;
  PAYLOAD_APP_HkUnlock();

  if (!report_version && !changed)
    return;
  if (version_ok)
    CFE_EVS_SendEvent(PAYLOAD_APP_VERSION_OK_EID, CFE_EVS_EventType_INFORMATION,
                      "Payload interface_version %s matches", st.interface_version);
  else
    CFE_EVS_SendEvent(PAYLOAD_APP_VERSION_MISMATCH_EID, CFE_EVS_EventType_ERROR,
                      "Payload interface_version '%s' != expected '%s'",
                      (st.valid & PAYLOAD_JSON_STATUS_VERSION) ? st.interface_version : "<missing>",
                      PAYLOAD_APP_INTERFACE_VERSION);
}

static bool PAYLOAD_APP_HandleAcquisition(const char *op, uint32 rid, const PAYLOAD_JSON_Doc_t *doc,
                                          const PAYLOAD_JSON_Envelope_t *env)
{
  int status_obj = PAYLOAD_JSON_Find(doc, env->data, "status");

  CFE_EVS_SendEvent(PAYLOAD_APP_ACQ_INFO_EID, CFE_EVS_EventType_INFORMATION, "%s: %s", op,
                    env->message[0] != '\0' ? env->message : "(no message)");

  if (status_obj < 0 || doc->tok[status_obj].type != JSMN_OBJECT)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_JSON_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: data.status is not an object",
                      op, (unsigned long)rid);
    return false;
  }
  PAYLOAD_APP_ApplyStatus(doc, status_obj, false);
  return true;
}

/* Handles an OK response; returns false (after its own event) if the data could not be used */
static bool PAYLOAD_APP_HandleOk(const PAYLOAD_APP_Request_t *req, uint32 rid, const PAYLOAD_JSON_Doc_t *doc,
                                 const PAYLOAD_JSON_Envelope_t *env, bool recovered)
{
  const char *op = PAYLOAD_APP_OpName(req->op);

  /* After an outage interface_version is re-checked like at startup; other ops schedule a GET_STATUS for it */
  if (recovered && req->op != PAYLOAD_APP_GET_STATUS_CC)
    PAYLOAD_APP_Worker.revalidate = true;

  switch (req->op)
  {
    case PAYLOAD_APP_GET_STATUS_CC:
      PAYLOAD_APP_ApplyStatus(doc, env->data, req->probe != 0 || recovered);
      return true;

    case PAYLOAD_APP_START_ACQUISITION_CC:
    case PAYLOAD_APP_STOP_ACQUISITION_CC:
      return PAYLOAD_APP_HandleAcquisition(op, rid, doc, env);

    case PAYLOAD_APP_GET_LIVE_STATE_CC:
      return PAYLOAD_APP_TlmLive(doc, env->data, rid);

    case PAYLOAD_APP_LIST_SESSIONS_CC:
      return PAYLOAD_APP_TlmSessionList(doc, env->data, rid);

    case PAYLOAD_APP_GET_SESSION_INFO_CC:
      return PAYLOAD_APP_TlmSessionInfo(doc, env->data, rid, req->session_id);

    case PAYLOAD_APP_READ_SESSION_RANGE_CC:
      return PAYLOAD_APP_TlmChunks(doc, env->data, rid, req->session_id, req->offset);

    default:
      CFE_EVS_SendEvent(PAYLOAD_APP_NOT_IMPLEMENTED_EID, CFE_EVS_EventType_ERROR, "%s response handling not implemented",
                        op);
      return false;
  }
}

static const char *PAYLOAD_APP_JsonResultText(PAYLOAD_JSON_Result_t result)
{
  switch (result)
  {
    case PAYLOAD_JSON_ERR_NOMEM:
      return "more JSON tokens than PAYLOAD_APP_JSON_MAX_TOKENS";
    case PAYLOAD_JSON_ERR_NOT_OBJECT:
      return "root is not a JSON object";
    default:
      return "invalid JSON";
  }
}

/* session_id was validated by the main task (YYYYMMDDTHHMMSSZ), so it needs no JSON escaping */
static void PAYLOAD_APP_BuildArgs(const PAYLOAD_APP_Request_t *req, char *buf, size_t size)
{
  switch (req->op)
  {
    case PAYLOAD_APP_GET_SESSION_INFO_CC:
      snprintf(buf, size, "{\"session_id\":\"%s\"}", req->session_id);
      break;
    case PAYLOAD_APP_READ_SESSION_RANGE_CC:
      snprintf(buf, size, "{\"session_id\":\"%s\",\"offset\":%lu,\"length\":%u}", req->session_id,
               (unsigned long)req->offset, (unsigned int)req->length);
      break;
    default:
      snprintf(buf, size, "{}");
      break;
  }
}

static void PAYLOAD_APP_Execute(const PAYLOAD_APP_Request_t *req)
{
  const char             *op                = PAYLOAD_APP_OpName(req->op);
  uint32                  timeout_ms        = PAYLOAD_APP_QUERY_TIMEOUT_MS;
  uint64_t                unreachable_since = 0;
  uint32                  busy_attempt      = 1;
  uint32                  busy_backoff_ms   = PAYLOAD_APP_BUSY_BACKOFF_MS;
  uint32                  rid;
  uint64_t                now;
  int                     req_len;
  size_t                  resp_len;
  int                     err_no;
  bool                    recovered;
  char                    args[96];
  char                    header[PAYLOAD_API_MAX_HEADER + 1];
  char                    why[64];
  PAYLOAD_API_Result_t    result;
  PAYLOAD_JSON_Result_t   json_result;
  PAYLOAD_JSON_Doc_t      doc;
  PAYLOAD_JSON_Envelope_t env;

  if (op == NULL)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_WORKER_ERR_EID, CFE_EVS_EventType_ERROR, "Queued request with unknown op %u",
                      (unsigned int)req->op);
    return;
  }
  if (req->op == PAYLOAD_APP_START_ACQUISITION_CC || req->op == PAYLOAD_APP_STOP_ACQUISITION_CC)
    timeout_ms = PAYLOAD_APP_ACQ_TIMEOUT_MS;
  PAYLOAD_APP_BuildArgs(req, args, sizeof(args));

  for (;;)
  {
    rid     = PAYLOAD_APP_Worker.next_request_id++;
    req_len = PAYLOAD_JSON_BuildRequest(PAYLOAD_APP_TxBuf, sizeof(PAYLOAD_APP_TxBuf), rid, op, args);
    if (req_len < 0)
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_WORKER_ERR_EID, CFE_EVS_EventType_ERROR, "%s: request does not fit %u B", op,
                        (unsigned int)sizeof(PAYLOAD_APP_TxBuf));
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
      return;
    }

    PAYLOAD_APP_HkLock();
    PAYLOAD_APP_Global.hk_packet.payload.last_request_id = rid;
    PAYLOAD_APP_HkUnlock();

    result = PAYLOAD_API_Transact(PAYLOAD_APP_Global.socket_path, PAYLOAD_APP_TxBuf, (size_t)req_len,
                                  PAYLOAD_APP_RxBuf, sizeof(PAYLOAD_APP_RxBuf), &resp_len, timeout_ms, &err_no,
                                  header);

    if (PAYLOAD_API_IsConnectError(result))
    {
      PAYLOAD_APP_ReportConnectFailure(result, err_no);

      /* Probes make a single attempt and are not counted; the worker loop schedules the next one */
      if (req->probe)
      {
        PAYLOAD_APP_Worker.probing = true;
        PAYLOAD_APP_GrowBackoff();
        return;
      }

      now = PAYLOAD_API_NowMs();
      if (unreachable_since == 0)
        unreachable_since = now;
      if (now - unreachable_since + PAYLOAD_APP_Worker.connect_backoff_ms > PAYLOAD_APP_CONNECT_RETRY_MAX_MS)
      {
        /* Never resent automatically (START/STOP above all): the operator decides */
        CFE_EVS_SendEvent(PAYLOAD_APP_WORKER_ERR_EID, CFE_EVS_EventType_ERROR,
                          "%s discarded: Payload unreachable for %lu ms (%s), not resent", op,
                          (unsigned long)(now - unreachable_since), PAYLOAD_API_ResultName(result));
        PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
        PAYLOAD_APP_Worker.probing = true;
        return;
      }

      OS_TaskDelay(PAYLOAD_APP_Worker.connect_backoff_ms);
      PAYLOAD_APP_GrowBackoff();
      continue;
    }

    /* Anything past connect() means the service is up, even if this exchange then fails */
    recovered = PAYLOAD_APP_ReportConnected(op);

    if (result != PAYLOAD_API_OK)
    {
      PAYLOAD_APP_ReportTransportError(op, rid, result, err_no, header, timeout_ms);
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
      return;
    }

    json_result = PAYLOAD_JSON_Parse(&doc, PAYLOAD_APP_RxBuf, resp_len, PAYLOAD_APP_Tokens,
                                     PAYLOAD_APP_JSON_MAX_TOKENS);
    if (json_result != PAYLOAD_JSON_OK)
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_JSON_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: %s", op, (unsigned long)rid,
                        PAYLOAD_APP_JsonResultText(json_result));
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
      return;
    }
    if (PAYLOAD_JSON_ParseEnvelope(&doc, &env, why, sizeof(why)) != PAYLOAD_JSON_OK)
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_JSON_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: bad envelope, %s", op,
                        (unsigned long)rid, why);
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
      return;
    }
    if (env.request_id != rid)
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_RID_MISMATCH_EID, CFE_EVS_EventType_ERROR,
                        "%s sent rid=%lu but response has rid=%lu, discarded", op, (unsigned long)rid,
                        (unsigned long)env.request_id);
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
      return;
    }

    if (env.status_code == PAYLOAD_API_STATUS_BUSY)
    {
      if (busy_attempt < PAYLOAD_APP_BUSY_MAX_ATTEMPTS)
      {
        CFE_EVS_SendEvent(PAYLOAD_APP_BUSY_RETRY_EID, CFE_EVS_EventType_INFORMATION,
                          "%s rid=%lu BUSY, retry %lu/%lu in %lu ms", op, (unsigned long)rid,
                          (unsigned long)busy_attempt + 1, (unsigned long)PAYLOAD_APP_BUSY_MAX_ATTEMPTS,
                          (unsigned long)busy_backoff_ms);
        OS_TaskDelay(busy_backoff_ms);
        busy_backoff_ms *= 2;
        busy_attempt++;
        continue;
      }
      CFE_EVS_SendEvent(PAYLOAD_APP_BUSY_EXHAUSTED_EID, CFE_EVS_EventType_ERROR, "%s BUSY after %lu attempts: %s",
                        op, (unsigned long)busy_attempt, env.message);
      PAYLOAD_APP_ReqErr(env.status_code);
      return;
    }

    if (env.status_code != PAYLOAD_API_STATUS_OK)
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_STATUS_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu %s(%u): %s", op,
                        (unsigned long)rid, PAYLOAD_JSON_StatusCodeName(env.status_code),
                        (unsigned int)env.status_code, env.message);
      PAYLOAD_APP_ReqErr(env.status_code);
      return;
    }

    if (PAYLOAD_APP_HandleOk(req, rid, &doc, &env, recovered))
      PAYLOAD_APP_ReqOk();
    else
      PAYLOAD_APP_ReqErr(PAYLOAD_API_STATUS_OK);
    return;
  }
}

/* Startup/recovery GET_STATUS: single attempt, reports interface_version */
static void PAYLOAD_APP_Probe(void)
{
  PAYLOAD_APP_Request_t req;

  memset(&req, 0, sizeof(req));
  req.op    = PAYLOAD_APP_GET_STATUS_CC;
  req.probe = 1;
  PAYLOAD_APP_Execute(&req);
}

void PAYLOAD_APP_WorkerMain(void)
{
  PAYLOAD_APP_Request_t req;
  size_t                copied;
  int32                 status;

  memset(&PAYLOAD_APP_Worker, 0, sizeof(PAYLOAD_APP_Worker));
  PAYLOAD_APP_Worker.next_request_id    = 1;
  PAYLOAD_APP_Worker.connect_backoff_ms = PAYLOAD_APP_CONNECT_BACKOFF_MIN_MS;

  for (;;)
  {
    if (PAYLOAD_APP_Worker.revalidate && !PAYLOAD_APP_Worker.probing)
    {
      PAYLOAD_APP_Worker.revalidate = false;
      PAYLOAD_APP_Probe();
      continue;
    }

    /* While probing, the backoff wait is a queue wait: an operator command is served immediately */
    copied = 0;
    status = OS_QueueGet(PAYLOAD_APP_Global.req_queue, &req, sizeof(req), &copied,
                         PAYLOAD_APP_Worker.probing ? (int32)PAYLOAD_APP_Worker.connect_backoff_ms : 1000);
    if (status == OS_SUCCESS && copied == sizeof(req))
    {
      PAYLOAD_APP_Execute(&req);
    }
    else if (status == OS_QUEUE_TIMEOUT)
    {
      if (PAYLOAD_APP_Worker.probing)
        PAYLOAD_APP_Probe();
    }
    else
    {
      CFE_EVS_SendEvent(PAYLOAD_APP_WORKER_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Request queue read failed, RC = %ld, size = %lu", (long)status, (unsigned long)copied);
      OS_TaskDelay(1000);
    }
  }
}
