#ifndef OBC_HK_H
#define OBC_HK_H

#define OBC_HK_TLM_PIP_NAME "OBC_HK_TLM_PIPE"
#define OBC_HK_PIPE_NAME_MAX 20

#include "cfe_sb.h"

typedef struct {
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  float          cpu_temp;
  uint8          cmd_counter;
  uint8          err_counter;
} OBC_HK_HkPacket_t;


typedef struct {
  char tlm_pip_name[OBC_HK_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  uint32 run_status;
  OBC_HK_HkPacket_t hk_packet;
} OBC_HK_GlobalApp_t;

#define OBC_HK_TLM_MSG_ID 56

#define OBC_HK_MSG_INIT_FAILURE_EID 1
#define OBC_HK_MSG_INIT_SUCCESFULY_EID 2



#endif 
