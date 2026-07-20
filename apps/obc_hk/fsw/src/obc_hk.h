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


#define OBC_HK_MSG_INIT_FAILURE_EID 1
#define OBC_HK_MSG_INIT_SUCCESFULY_EID 2
#define OBC_HK_MSH_TRANSMITION_ERR_EID 3

#endif 
