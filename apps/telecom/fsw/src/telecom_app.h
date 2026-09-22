#ifndef TELECOM_H
#define TELECOM_H

#include "cfe_es.h"
#include "cfe_sb.h"
#include "common_types.h"

void  SAMPLE_APP_Main(void);
int   TELECOM_APP_Init(void);

#define TELECOM_TLM_PIPE_NAME "TELECOM_TLM_PIPE"
#define TELECOM_TLM_PIPE_NAME_MAX 17

typedef uint32 TELECOM_Err_t;

#ifdef COMMUNICATION_LORA

#include "telecom_lora_controller.h"
#include "telecom_serial.h"

typedef struct {
  char            tlm_pip_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  char            cmd_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;

  LoraController controller;
  
  uint32          run_status;
  bool            downlink_on;
  bool            suppress_sendto;

} TELECOM_GlobalApp_t;

#else

typedef struct {
  char            tlm_pip_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  char            cmd_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;

  char            tlm_dest_ip[17];
  osal_id_t       tlm_sock_id;

  uint32          run_status;
  bool            downlink_on;
  bool            suppress_sendto;

} TELECOM_GlobalApp_t;

#endif 

extern TELECOM_GlobalApp_t TELECOM_data;


void TELECOM_APP_open_tlm(void);
void TELECOM_APP_forward_tlm(void);

#endif
