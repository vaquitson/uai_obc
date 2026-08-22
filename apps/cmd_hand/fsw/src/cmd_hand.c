#include <stdio.h>
#include <string.h>

#include "common_types.h"
#include "cfe_evs.h"
#include "cfe_es.h"
#include "cfe_sb.h"
#include "cmd_hand.h"

#include "cmd_hand_msgids.h"
#include "cmd_hand_eventids.h"
#include "cfe_psp.h"
#include "osapi-sockets.h"

CMD_HAND_GlobalData_t CMD_HAND_data;


/**
  * This function set up the sb pipe service 
  * for the applicaciotn.
*/
CFE_Status_t CMD_HAND_pipe_set_up(void){
  CFE_Status_t status;

  status = CFE_SB_CreatePipe(&CMD_HAND_data.cmd_pipe, 
                             CMD_HAND_PLAFORM_PIPE_DEPTH,
                             "CMD_HAND_PIPE");

  if (status == CFE_SUCCESS){
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CMD_HAND_CMD_MID), CMD_HAND_data.cmd_pipe);
    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(CMD_HAND_SUBSCRIPTION_CMD_ERR_EID, 
                        CFE_EVS_EventType_ERROR,
                        "Error subscribing to SB Commands, RC = 0x%08X, LINE = %d", 
                        (unsigned int)status,
                        __LINE__);
    }
  } else {
    CFE_EVS_SendEvent(CMD_HAND_PIPE_CREATION_ERR_EID, 
                      CFE_EVS_EventType_ERROR,
                      "Error creating SB Commands pipe, RC = 0x%08X, LINE = %d", 
                      (unsigned int)status,
                      __LINE__);
  }

  return status;
}

/**
  * This function handles the setup of the 
  * listening socket of the app;
*/
CFE_Status_t CMD_HAND_listening_sock_set_up(void){
  CFE_Status_t status;
  uint16 default_listening_port;

  default_listening_port = CMD_HAND_LISTENING_PORT + CFE_PSP_GetProcessorId() - 1;

  status = OS_SocketOpen(&CMD_HAND_data.tc_sock_id, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
  if (status != CFE_SUCCESS){
    CFE_EVS_SendEvent(CMD_HAND_OPENING_SOCKET_ERR, 
                      CFE_EVS_EventType_ERROR,
                      "Error opening telecommand socket, RC = 0x%08X, LINE = %d",
                      (unsigned int)status,
                      __LINE__);
  } else {
    OS_SocketAddrInit(&CMD_HAND_data.sock_addr, OS_SocketDomain_INET);
    OS_SocketAddrSetPort(&CMD_HAND_data.sock_addr, default_listening_port);

    status = OS_SocketBind(CMD_HAND_data.tc_sock_id, &CMD_HAND_data.sock_addr);
    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(CMD_HAND_BINDING_SOCKET_ERR, 
                        CFE_EVS_EventType_ERROR,
                        "Error binding the cmd hand listening socket, RC = 0x%08X, LINE = %d", 
                        (unsigned int)status,
                        __LINE__);
    } else {
      CMD_HAND_data.run_status = true;
      CFE_EVS_SendEvent(CMD_HAND_SUCCESS_EID, 
                        CFE_EVS_EventType_INFORMATION,
                        "CMD HAND listening on port %d, RC = 0x%08X",
                        default_listening_port,
                        (unsigned int)status);

    }
  }

  return status;
}


void CMD_HAND_delete_callback(void)
{
    OS_printf("CMD HAND delete callback -- Closing CMD HAND Network socket.\n");
    OS_close(CMD_HAND_data.tc_sock_id);
}


CFE_Status_t CMD_HAND_init(void){
  CFE_Status_t status;

  memset(&CMD_HAND_data, 0, sizeof(CMD_HAND_GlobalData_t));

  CMD_HAND_data.run_status = CFE_ES_RunStatus_APP_RUN;  
  CMD_HAND_data.sock_listening = false;

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS) {
    CFE_ES_WriteToSysLog("CMD HAND: Error registering for Event Services, RC = 0x%08X, LINE = %d\n",
                         (unsigned int)status,
                         __LINE__);
  }
  
  // Should wee shout down execuction on a pipe error ???
  status = CMD_HAND_pipe_set_up();

  status = CMD_HAND_listening_sock_set_up();

  OS_TaskInstallDeleteHandler(&CMD_HAND_delete_callback);

  if (status == CFE_SUCCESS){
    CFE_EVS_SendEvent(CMD_HAND_SUCCESS_EID, 
                      CFE_EVS_EventType_INFORMATION,
                      "CMD HAND intialized successfuly, RC = 0x%08X",
                      (unsigned int)status);
  } else {
    CFE_EVS_SendEvent(CMD_HAND_SUBSCRIPTION_CMD_ERR_EID, 
                      CFE_EVS_EventType_ERROR,
                      "Error initializing the CMD HAND app, RC = 0x%08X, LINE = %d", 
                      (unsigned int)status,
                      __LINE__);  
  }

  return status;
}


void CMD_HAND_AppMain(void){ 
  CFE_Status_t status;

  status = CMD_HAND_init();

  if (status != CFE_SUCCESS){
    CMD_HAND_data.run_status = CFE_ES_RunStatus_APP_ERROR;
  }
  
  while (CFE_ES_RunLoop(&CMD_HAND_data.run_status) == true){ 
    OS_TaskDelay(10000);
  }

  CFE_ES_ExitApp(CMD_HAND_data.run_status);
}
