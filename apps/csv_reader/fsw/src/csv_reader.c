#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"
#include "osapi.h"

#include "csv_reader_msgdefs.h"
#include "csv_reader_msg.h"
#include "csv_reader_eventids.h"
#include "csv_reader_fcncodes.h"
#include "csv_reader.h"

CSV_READER_GlobalApp_t CSV_READER_Global;

CFE_Status_t CSV_READER_Init(void)
{
  int32 status;

  memset(&CSV_READER_Global, 0, sizeof(CSV_READER_Global));

  CSV_READER_Global.run_status = CFE_ES_RunStatus_APP_RUN;

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS)
    CFE_ES_WriteToSysLog("CSV_READER: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(CSV_READER_Global.hk_packet.telemetry_header),
                         CFE_SB_ValueToMsgId(CSV_READER_TLM_MID), sizeof(CSV_READER_Global.hk_packet));
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(CSV_READER_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "CSV_READER: Error initializing HK msg, RC = 0x%08lX", (unsigned long)status);

  status = CFE_SB_CreatePipe(&CSV_READER_Global.cmd_pipe, CSV_READER_PIPE_DEPTH, CSV_READER_PIPE_NAME);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(CSV_READER_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "CSV_READER: Error creating cmd pipe, RC = 0x%08lX", (unsigned long)status);

  if (status == CFE_SUCCESS)
  {
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CSV_READER_CMD_MID), CSV_READER_Global.cmd_pipe);
    if (status != CFE_SUCCESS)
      CFE_EVS_SendEvent(CSV_READER_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                         "CSV_READER: Error subscribing to commands, RC = 0x%08lX", (unsigned long)status);
  }

  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(CSV_READER_INIT_SUCCESSFUL_EID, CFE_EVS_EventType_INFORMATION,
                       "CSV_READER: Initialized successfully, RC = 0x%08lX", (unsigned long)status);

  return status;
}

void CSV_READER_AppMain(void)
{
  int32 status;
  CFE_SB_Buffer_t *sb_buf_ptr;

  status = CSV_READER_Init();
  if (status != CFE_SUCCESS)
    CSV_READER_Global.run_status = CFE_ES_RunStatus_APP_ERROR;

  while (CFE_ES_RunLoop(&CSV_READER_Global.run_status) == true)
  {
    status = CFE_SB_ReceiveBuffer(&sb_buf_ptr, CSV_READER_Global.cmd_pipe, CSV_READER_SB_TIMEOUT_MS);

    if (status == CFE_SUCCESS)
      CSV_READER_ProcessCommandPacket(sb_buf_ptr);
    else if (status != CFE_SB_TIME_OUT)
    {
      CFE_ES_WriteToSysLog("CSV_READER: SB pipe read error, RC = 0x%08lX\n", (unsigned long)status);
      CSV_READER_Global.run_status = CFE_ES_RunStatus_APP_ERROR;
    }

    CSV_READER_SendHk();

    /* Pausa antes de volver a leer/imprimir el CSV, mismo patron que usa
    ** obc_hk (OS_TaskDelay) para marcarle el ritmo a lo que imprime telecom */
    OS_TaskDelay(CSV_READER_READ_INTERVAL_MS);
    CSV_READER_ReadFile(CSV_READER_DEFAULT_FILE);
  }

  CFE_ES_ExitApp(CSV_READER_Global.run_status);
}

void CSV_READER_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_SB_MsgId_t msg_id = CFE_SB_INVALID_MSG_ID;

  CFE_MSG_GetMsgId(&sb_buf_ptr->Msg, &msg_id);

  switch (CFE_SB_MsgIdToValue(msg_id))
  {
    case CSV_READER_CMD_MID:
      CSV_READER_ProcessGroundCommand(sb_buf_ptr);
      break;

    default:
      CFE_EVS_SendEvent(CSV_READER_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                         "CSV_READER: invalid command pipe message ID: 0x%X",
                         (unsigned int)CFE_SB_MsgIdToValue(msg_id));
      break;
  }
}

void CSV_READER_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_MSG_FcnCode_t fcn_code = 0;

  CFE_MSG_GetFcnCode(&sb_buf_ptr->Msg, &fcn_code);

  switch (fcn_code)
  {
    case CSV_READER_NOOP_CC:
      CSV_READER_Noop();
      break;

    case CSV_READER_RESET_COUNTERS_CC:
      CSV_READER_ResetCounters();
      break;

    case CSV_READER_READ_FILE_CC:
    {
      const CSV_READER_ReadFileCmd_t *cmd = (const CSV_READER_ReadFileCmd_t *)sb_buf_ptr;
      char file_name[CSV_READER_FILENAME_LEN];

      strncpy(file_name, cmd->file_name, sizeof(file_name) - 1);
      file_name[sizeof(file_name) - 1] = '\0';

      CSV_READER_Global.hk_packet.cmd_counter++;
      CSV_READER_ReadFile(file_name);
      break;
    }

    default:
      CSV_READER_Global.hk_packet.err_counter++;
      CFE_EVS_SendEvent(CSV_READER_INVALID_CC_EID, CFE_EVS_EventType_ERROR,
                         "CSV_READER: invalid command code: %u", (unsigned int)fcn_code);
      break;
  }
}

void CSV_READER_Noop(void)
{
  CSV_READER_Global.hk_packet.cmd_counter++;
  CFE_EVS_SendEvent(CSV_READER_NOOP_EID, CFE_EVS_EventType_INFORMATION, "CSV_READER: NOOP command received");
}

void CSV_READER_ResetCounters(void)
{
  CSV_READER_Global.hk_packet.cmd_counter = 0;
  CSV_READER_Global.hk_packet.err_counter = 0;
  CFE_EVS_SendEvent(CSV_READER_RESET_COUNTERS_EID, CFE_EVS_EventType_INFORMATION,
                     "CSV_READER: counters reset");
}

void CSV_READER_ReadFile(const char *file_name)
{
  static char file_buf[CSV_READER_FILE_BUF_SIZE];

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

  strncpy(CSV_READER_Global.hk_packet.last_file, file_name, CSV_READER_FILENAME_LEN - 1);
  CSV_READER_Global.hk_packet.last_file[CSV_READER_FILENAME_LEN - 1] = '\0';

  status = OS_OpenCreate(&fd, file_name, OS_FILE_FLAG_NONE, OS_READ_ONLY);
  if (status != OS_SUCCESS)
  {
    CFE_EVS_SendEvent(CSV_READER_FILE_OPEN_ERR_EID, CFE_EVS_EventType_ERROR,
                       "CSV_READER: failed to open '%s', RC = 0x%08lX", file_name, (unsigned long)status);
    CSV_READER_Global.hk_packet.last_read_valid = false;
    CSV_READER_Global.hk_packet.err_counter++;
    return;
  }

  bytes_read = OS_read(fd, file_buf, sizeof(file_buf) - 1);
  OS_close(fd);

  if (bytes_read <= 0)
  {
    CFE_EVS_SendEvent(CSV_READER_FILE_EMPTY_EID, CFE_EVS_EventType_ERROR, "CSV_READER: '%s' is empty or unreadable",
                       file_name);
    CSV_READER_Global.hk_packet.last_read_valid = false;
    CSV_READER_Global.hk_packet.err_counter++;
    return;
  }

  file_buf[bytes_read] = '\0';

  line = strtok_r(file_buf, "\r\n", &line_ctx);
  while (line != NULL && rows < CSV_READER_MAX_ROWS)
  {
    char *field_ctx = NULL;
    char *field      = strtok_r(line, ",", &field_ctx);
    uint32 cols      = 0;

    while (field != NULL && cols < CSV_READER_MAX_COLS)
    {
      char *end = NULL;
      float value = strtof(field, &end);

      /* skip non-numeric fields, e.g. a header row */
      if (end != field)
      {
        CSV_READER_Global.data[rows][cols] = value;
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

  CSV_READER_Global.hk_packet.rows_read        = rows;
  CSV_READER_Global.hk_packet.cols_read        = max_cols_seen;
  CSV_READER_Global.hk_packet.value_sum        = sum;
  CSV_READER_Global.hk_packet.value_min        = have_value ? min_val : 0.0f;
  CSV_READER_Global.hk_packet.value_max        = have_value ? max_val : 0.0f;
  CSV_READER_Global.hk_packet.last_read_valid  = (rows > 0);

  if (rows > 0)
  {
    uint32 r;
    uint32 c;

    CFE_EVS_SendEvent(CSV_READER_FILE_READ_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                       "CSV_READER: parsed '%s' - %u rows x %u cols", file_name, (unsigned int)rows,
                       (unsigned int)max_cols_seen);

    /* Print de los datos leidos para verificar el resultado al correr la app */
    printf("CSV_READER: '%s' -> %u filas x %u columnas\n", file_name, (unsigned int)rows,
           (unsigned int)max_cols_seen);
    for (r = 0; r < rows; r++)
    {
      printf("  fila %u:", (unsigned int)r);
      for (c = 0; c < max_cols_seen; c++)
      {
        printf(" %.4f", CSV_READER_Global.data[r][c]);
      }
      printf("\n");
    }
    printf("CSV_READER: sum=%.4f min=%.4f max=%.4f\n", sum, min_val, max_val);
  }
  else
  {
    CFE_EVS_SendEvent(CSV_READER_FILE_EMPTY_EID, CFE_EVS_EventType_ERROR, "CSV_READER: no numeric rows found in '%s'",
                       file_name);
    CSV_READER_Global.hk_packet.err_counter++;
  }
}

void CSV_READER_SendHk(void)
{
  int32 status;

  status = CFE_SB_TransmitMsg(CFE_MSG_PTR(CSV_READER_Global.hk_packet.telemetry_header), true);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(CSV_READER_HK_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "CSV_READER: HK transmit error, RC = 0x%08lX", (unsigned long)status);
}
