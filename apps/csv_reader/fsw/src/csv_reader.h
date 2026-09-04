#ifndef CSV_READER_H
#define CSV_READER_H

#include "cfe_sb.h"
#include "csv_reader_msgdefs.h"
#include "csv_reader_platform_cfg.h"

#define CSV_READER_PIPE_NAME "CSV_READER_CMD_PIPE"

typedef struct {
  CFE_SB_PipeId_t cmd_pipe;

  uint32 run_status;

  CSV_READER_HkPacket_t hk_packet;

  /* last CSV file parsed into memory, row-major */
  float  data[CSV_READER_MAX_ROWS][CSV_READER_MAX_COLS];
} CSV_READER_GlobalApp_t;

CFE_Status_t CSV_READER_Init(void);
void         CSV_READER_AppMain(void);

void CSV_READER_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr);
void CSV_READER_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr);

void CSV_READER_Noop(void);
void CSV_READER_ResetCounters(void);
void CSV_READER_ReadFile(const char *file_name);

void CSV_READER_SendHk(void);

#endif
