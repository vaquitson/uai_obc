#ifndef PAYLOAD_APP_H
#define PAYLOAD_APP_H

#include "cfe_sb.h"
#include "payload_app_msgdefs.h"
#include "payload_app_platform_cfg.h"

#define PAYLOAD_APP_PIPE_NAME "PAYLOAD_APP_PIPE"

/* Scratch buffer big enough for one framed JSON message body */
#define PAYLOAD_APP_SOCK_BUF_SIZE (PAYLOAD_APP_SOCK_MAX_MSG_SIZE + 1)

typedef struct {
  CFE_SB_PipeId_t cmd_pipe;

  uint32 run_status;

  PAYLOAD_APP_HkPacket_t hk_packet;

  /* last message parsed from Payload's socket, and what gets published on
  ** PAYLOAD_APP_DATA_TLM_MID / printed to stdout */
  PAYLOAD_APP_DataMsg_t data_msg;

  /* scratch buffer for the raw JSON body read off the socket each cycle */
  char sock_buf[PAYLOAD_APP_SOCK_BUF_SIZE];
} PAYLOAD_APP_GlobalApp_t;

extern PAYLOAD_APP_GlobalApp_t PAYLOAD_APP_Global;

CFE_Status_t PAYLOAD_APP_Init(void);
void         PAYLOAD_APP_AppMain(void);

void PAYLOAD_APP_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr);
void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr);

void PAYLOAD_APP_Noop(void);
void PAYLOAD_APP_ResetCounters(void);

void PAYLOAD_APP_SendHk(void);

#endif
