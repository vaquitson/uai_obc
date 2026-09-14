#ifndef PAYLOAD_APP_H
#define PAYLOAD_APP_H

#include "cfe_sb.h"
#include "payload_app_msgdefs.h"
#include "payload_app_platform_cfg.h"

#define PAYLOAD_APP_PIPE_NAME "PAYLOAD_APP_PIPE"

typedef struct {
  CFE_SB_PipeId_t cmd_pipe;

  uint32 run_status;

  PAYLOAD_APP_HkPacket_t hk_packet;

  /* path read on each cycle by the main loop; seeded from PAYLOAD_APP_DEFAULT_FILE
  ** at init, kept as a variable so it can later be changed at runtime */
  char   active_file[PAYLOAD_APP_FILENAME_LEN];

  /* last CSV file parsed into memory, row-major */
  float  data[PAYLOAD_APP_MAX_ROWS][PAYLOAD_APP_MAX_COLS];
} PAYLOAD_APP_GlobalApp_t;

CFE_Status_t PAYLOAD_APP_Init(void);
void         PAYLOAD_APP_AppMain(void);

void PAYLOAD_APP_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr);
void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr);

void PAYLOAD_APP_Noop(void);
void PAYLOAD_APP_ResetCounters(void);
void PAYLOAD_APP_ReadFile(const char *file_name);

void PAYLOAD_APP_SendHk(void);

#endif
