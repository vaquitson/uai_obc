#ifndef CMD_HAND_H
#define CMD_HAND_H

#include "common_types.h"
#include "cfe_sb.h"

#define CMD_HAND_CMD_PIPE_NAME "CMD_HAND_CMD_PIPE"
#define CMD_HAND_CMD_PIPE_NAME_MAX 18

#ifdef COMMUNICATION_LORA
tyedef struct {
  LoraController controller;
} CommunicationDev;

#else
typedef struct {
  char  cmd_source_ip[17];
} CommunicationDev;

#endif

typedef struct {
  char cmd_pipe_name[CMD_HAND_CMD_PIPE_NAME_MAX];
  CFE_SB_PipeId_t cmd_pipe;


  osal_id_t       tc_sock_id;
  OS_SockAddr_t   sock_addr;

  // implementation dependent
  CommunicationDev com_dev;

  bool            sock_listening;
  bool            Scheduled;
  bool            AllowPassthrough;

  void * net_buf_ptr;
  size_t net_buf_size;

  uint8 cmd_counter;
  uint8 err_counter;

  uint32 run_status;

} CMD_HAND_GlobalData_t;


#endif
