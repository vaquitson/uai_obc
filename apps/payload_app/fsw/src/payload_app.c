#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"
#include "osapi.h"

#include "payload_app_msgdefs.h"
#include "payload_app_msg.h"
#include "payload_app_eventids.h"
#include "payload_app_fcncodes.h"
#include "payload_app.h"

PAYLOAD_APP_GlobalApp_t PAYLOAD_APP_Global;

CFE_Status_t PAYLOAD_APP_Init(void)
{
  int32 status;

  memset(&PAYLOAD_APP_Global, 0, sizeof(PAYLOAD_APP_Global));

  PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_RUN;

  strncpy(PAYLOAD_APP_Global.active_file, PAYLOAD_APP_DEFAULT_FILE, PAYLOAD_APP_FILENAME_LEN - 1);
  PAYLOAD_APP_Global.active_file[PAYLOAD_APP_FILENAME_LEN - 1] = '\0';

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS)
    CFE_ES_WriteToSysLog("PAYLOAD_APP: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header),
                         CFE_SB_ValueToMsgId(PAYLOAD_APP_TLM_MID), sizeof(PAYLOAD_APP_Global.hk_packet));
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error initializing HK msg, RC = 0x%08lX", (unsigned long)status);

  status = CFE_SB_CreatePipe(&PAYLOAD_APP_Global.cmd_pipe, PAYLOAD_APP_PIPE_DEPTH, PAYLOAD_APP_PIPE_NAME);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating cmd pipe, RC = 0x%08lX", (unsigned long)status);

  if (status == CFE_SUCCESS)
  {
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(PAYLOAD_APP_CMD_MID), PAYLOAD_APP_Global.cmd_pipe);
    if (status != CFE_SUCCESS)
      CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: Error subscribing to commands, RC = 0x%08lX", (unsigned long)status);
  }

  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_SUCCESSFUL_EID, CFE_EVS_EventType_INFORMATION,
                       "PAYLOAD_APP: Initialized successfully, RC = 0x%08lX", (unsigned long)status);

  return status;
}

void PAYLOAD_APP_AppMain(void)
{
  int32 status;
  CFE_SB_Buffer_t *sb_buf_ptr;

  status = PAYLOAD_APP_Init();
  if (status != CFE_SUCCESS)
    PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;

  while (CFE_ES_RunLoop(&PAYLOAD_APP_Global.run_status) == true)
  {
    status = CFE_SB_ReceiveBuffer(&sb_buf_ptr, PAYLOAD_APP_Global.cmd_pipe, PAYLOAD_APP_SB_TIMEOUT_MS);

    if (status == CFE_SUCCESS)
      PAYLOAD_APP_ProcessCommandPacket(sb_buf_ptr);
    else if (status != CFE_SB_TIME_OUT)
    {
      CFE_ES_WriteToSysLog("PAYLOAD_APP: SB pipe read error, RC = 0x%08lX\n", (unsigned long)status);
      PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;
    }

    PAYLOAD_APP_SendHk();

    /* Pausa antes de volver a leer/imprimir el CSV, mismo patron que usa
    ** obc_hk (OS_TaskDelay) para marcarle el ritmo a lo que imprime telecom */
    OS_TaskDelay(PAYLOAD_APP_READ_INTERVAL_MS);
    PAYLOAD_APP_ReadFile(PAYLOAD_APP_Global.active_file);
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

void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_MSG_FcnCode_t fcn_code = 0;

  CFE_MSG_GetFcnCode(&sb_buf_ptr->Msg, &fcn_code);

  switch (fcn_code)
  {
    case PAYLOAD_APP_NOOP_CC:
      PAYLOAD_APP_Noop();
      break;

    case PAYLOAD_APP_RESET_COUNTERS_CC:
      PAYLOAD_APP_ResetCounters();
      break;

    case PAYLOAD_APP_READ_FILE_CC:
    {
      const PAYLOAD_APP_ReadFileCmd_t *cmd = (const PAYLOAD_APP_ReadFileCmd_t *)sb_buf_ptr;
      char file_name[PAYLOAD_APP_FILENAME_LEN];

      strncpy(file_name, cmd->file_name, sizeof(file_name) - 1);
      file_name[sizeof(file_name) - 1] = '\0';

      PAYLOAD_APP_Global.hk_packet.cmd_counter++;
      PAYLOAD_APP_ReadFile(file_name);
      break;
    }

    default:
      PAYLOAD_APP_Global.hk_packet.err_counter++;
      CFE_EVS_SendEvent(PAYLOAD_APP_INVALID_CC_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: invalid command code: %u", (unsigned int)fcn_code);
      break;
  }
}

void PAYLOAD_APP_Noop(void)
{
  PAYLOAD_APP_Global.hk_packet.cmd_counter++;
  CFE_EVS_SendEvent(PAYLOAD_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION, "PAYLOAD_APP: NOOP command received");
}

void PAYLOAD_APP_ResetCounters(void)
{
  PAYLOAD_APP_Global.hk_packet.cmd_counter = 0;
  PAYLOAD_APP_Global.hk_packet.err_counter = 0;
  CFE_EVS_SendEvent(PAYLOAD_APP_RESET_COUNTERS_EID, CFE_EVS_EventType_INFORMATION,
                     "PAYLOAD_APP: counters reset");
}

void PAYLOAD_APP_ReadFile(const char *file_name)
{
  static char file_buf[PAYLOAD_APP_FILE_BUF_SIZE];

  osal_id_t fd;
  int32     status;
  int32     bytes_read;
  uint32    rows          = 0;
  uint32    max_cols_seen = 0;
  float     sum           = 0.0f;
  float     min_val       = 0.0f;
  float     max_val       = 0.0f;
  bool      have_value    = false;
  char     *line_ctx      = NULL;
  char     *line;

  strncpy(PAYLOAD_APP_Global.hk_packet.last_file, file_name, PAYLOAD_APP_FILENAME_LEN - 1);
  PAYLOAD_APP_Global.hk_packet.last_file[PAYLOAD_APP_FILENAME_LEN - 1] = '\0';

  status = OS_OpenCreate(&fd, file_name, OS_FILE_FLAG_NONE, OS_READ_ONLY);
  if (status != OS_SUCCESS)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_FILE_OPEN_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: failed to open '%s', RC = 0x%08lX", file_name, (unsigned long)status);
    PAYLOAD_APP_Global.hk_packet.last_read_valid = false;
    PAYLOAD_APP_Global.hk_packet.err_counter++;
    return;
  }

  bytes_read = OS_read(fd, file_buf, sizeof(file_buf) - 1);
  OS_close(fd);

  if (bytes_read <= 0)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_FILE_EMPTY_EID, CFE_EVS_EventType_ERROR, "PAYLOAD_APP: '%s' is empty or unreadable",
                       file_name);
    PAYLOAD_APP_Global.hk_packet.last_read_valid = false;
    PAYLOAD_APP_Global.hk_packet.err_counter++;
    return;
  }

  file_buf[bytes_read] = '\0';

  line = strtok_r(file_buf, "\r\n", &line_ctx);
  while (line != NULL && rows < PAYLOAD_APP_MAX_ROWS)
  {
    char *field_ctx = NULL;
    char *field      = strtok_r(line, ",", &field_ctx);
    uint32 cols      = 0;

    while (field != NULL && cols < PAYLOAD_APP_MAX_COLS)
    {
      char *end = NULL;
      float value = strtof(field, &end);

      /* skip non-numeric fields, e.g. a header row */
      if (end != field)
      {
        PAYLOAD_APP_Global.data[rows][cols] = value;
        sum += value;
        if (!have_value || value < min_val)
          min_val = value;
        if (!have_value || value > max_val)
          max_val = value;
        have_value = true;
        cols++;
      }

      field = strtok_r(NULL, ",", &field_ctx);
    }

    if (cols > 0)
    {
      if (cols > max_cols_seen)
        max_cols_seen = cols;
      rows++;
    }

    line = strtok_r(NULL, "\r\n", &line_ctx);
  }

  PAYLOAD_APP_Global.hk_packet.rows_read        = rows;
  PAYLOAD_APP_Global.hk_packet.cols_read        = max_cols_seen;
  PAYLOAD_APP_Global.hk_packet.value_sum        = sum;
  PAYLOAD_APP_Global.hk_packet.value_min        = have_value ? min_val : 0.0f;
  PAYLOAD_APP_Global.hk_packet.value_max        = have_value ? max_val : 0.0f;
  PAYLOAD_APP_Global.hk_packet.last_read_valid  = (rows > 0);

  if (rows > 0)
  {
    uint32 r;
    uint32 c;

    CFE_EVS_SendEvent(PAYLOAD_APP_FILE_READ_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                       "PAYLOAD_APP: parsed '%s' - %u rows x %u cols", file_name, (unsigned int)rows,
                       (unsigned int)max_cols_seen);

    /* Print de los datos leidos para verificar el resultado al correr la app */
    printf("PAYLOAD_APP: '%s' -> %u filas x %u columnas\n", file_name, (unsigned int)rows,
           (unsigned int)max_cols_seen);
    for (r = 0; r < rows; r++)
    {
      printf("  fila %u:", (unsigned int)r);
      for (c = 0; c < max_cols_seen; c++)
      {
        printf(" %.4f", PAYLOAD_APP_Global.data[r][c]);
      }
      printf("\n");
    }
    printf("PAYLOAD_APP: sum=%.4f min=%.4f max=%.4f\n", sum, min_val, max_val);
  }
  else
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_FILE_EMPTY_EID, CFE_EVS_EventType_ERROR, "PAYLOAD_APP: no numeric rows found in '%s'",
                       file_name);
    PAYLOAD_APP_Global.hk_packet.err_counter++;
  }
}

void PAYLOAD_APP_SendHk(void)
{
  int32 status;

  status = CFE_SB_TransmitMsg(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header), true);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_HK_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: HK transmit error, RC = 0x%08lX", (unsigned long)status);
}
