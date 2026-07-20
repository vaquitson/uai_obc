#ifndef OBC_HK_H
#define OBC_HK_H

#include "obc_hk_msgdefs.h"
#include "cfe_sb.h"

#define OBC_HK_TLM_PIP_NAME "OBC_HK_TLM_PIPE"
#define OBC_HK_PIPE_NAME_MAX 20

typedef struct {
  char tlm_pip_name[OBC_HK_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  uint32 run_status;
  OBC_HK_HkPacket_t hk_packet;
} OBC_HK_GlobalApp_t;

#endif 
