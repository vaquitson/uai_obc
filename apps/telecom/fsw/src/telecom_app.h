#ifndef TELECOM_H
#define TELECOM_H

#include "cfe_es.h"
#include "cfe_sb.h"
#include "telecom_msg.h"
#include "common_types.h"

void  SAMPLE_APP_Main(void);
int   TELECOM_APP_init(void);

#define TELECOM_TLM_PIPE_NAME "TELECOM_TLM_PIPE"
#define TELECOM_TLM_PIPE_NAME_MAX 17

#define TELECOM_CMD_PIPE_NAME "TELECOM_CMD_PIPE"
#define TELECOM_CMD_PIPE_NAME_MAX 17

typedef uint32 TELECOM_Err_t;

#ifdef COMMUNICATION_LORA

#include "telecom_lora_controller.h"

typedef struct {
  TELECOM_HkTlm_t hk_tlm;

  char            tlm_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  char            cmd_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;

  LoraController controller;

  uint8 cmd_counter;
  uint8 err_counter;

  uint32          run_status;
  bool            downlink_on;
} TELECOM_GlobalApp_t;

#else

typedef struct {
  TELECOM_HkTlm_t hk_tlm;

  char            tlm_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  char            cmd_pipe_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;

  char            tlm_dest_ip[17];
  char            tlm_port[10];
  osal_id_t       tlm_sock_id;

  uint8 cmd_counter;
  uint8 err_counter;
  uint8 send_counter;

  uint32          run_status;
  bool            downlink_on;
} TELECOM_GlobalApp_t;

#endif 

extern TELECOM_GlobalApp_t TELECOM_data;


void TELECOM_APP_open_tlm(void);
void TELECOM_APP_forward_tlm(void);

#endif
