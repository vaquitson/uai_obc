#ifndef OBC_HK_H
#define OBC_HK_H

#include "obc_hk_msgdefs.h"
#include "cfe_sb.h"
#include "obc_hk_internal_cfg.h"

typedef struct {
  char cmd_pip_name[OBC_HK_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;

  uint32 run_status;
  OBC_HK_HkPacket_t hk_packet;
} OBC_HK_GlobalApp_t;

extern OBC_HK_GlobalApp_t OBC_HK_data;

#endif 
