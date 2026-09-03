#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cfe_evs.h"

#include "obc_hk_msg.h"
#include "obc_hk_msgdefs.h"

#include "cmd_hand_msgids.h"

#include "telecom_interface_cfg.h"
#include "telecom_internal_cfg.h"
#include "telecom_eventids.h"
#include "telecom_errors.h"
#include "telecom_app.h"
#include "telecom_encode.h"

TELECOM_GlobalApp_t TELECOM_data;

CFE_Status_t TELECOM_APP_Init(void){ 
  CFE_Status_t status;

  memset(&TELECOM_data, 0, sizeof(TELECOM_data));
  TELECOM_data.suppress_sendto = false;
  TELECOM_data.downlink_on = false;

  TELECOM_data.run_status = CFE_ES_RunStatus_APP_RUN; 

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }
  
  strncpy(TELECOM_data.tlm_pip_name, TELECOM_TLM_PIPE_NAME, TELECOM_TLM_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_data.tlm_pipe, 10, TELECOM_data.tlm_pip_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the tlm pipe,  RC = 0x%08lX", (unsigned long)status);

  status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(OBC_HK_TLM_MID), TELECOM_data.tlm_pipe);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_SUBSCRIPTION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to subscribe to OBC HK,  RC = 0x%08lX", (unsigned long)status);

  
 
  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_INIT_SUCCESFULL_EID, CFE_EVS_EventType_INFORMATION,
                      "TELECOM: Initialized succesfuly, RC = 0x%08lX", (unsigned long)status);

  return status;
}


void TELECOM_APP_open_telemetry(void){
  int32 status;
  status = OS_SocketOpen(&TELECOM_data.tlm_sock_id, 
                         OS_SocketDomain_INET, 
                         OS_SocketType_DATAGRAM);

  if (status != OS_SUCCESS){
    CFE_EVS_SendEvent(TELECOM_TLM_SOCK_ERR_EID, 
                      CFE_EVS_EventType_ERROR, 
                      "L%d, TO TLM socket error: %d",
                      __LINE__, (int)status);    
  }
}




void TELECOM_APP_forward_telemetry(void){
  OS_SockAddr_t    dest_addr;
  int32            os_status;
  uint32           pkt_count = 0;
  uint16           port_num = TELECOM_MISSION_TLM_PORT; 

  const void      *net_buf_p;
  size_t           net_buf_s;
  CFE_Status_t     cfe_status;
  CFE_SB_Buffer_t *sb_buf_p;

  OS_SocketAddrInit(&dest_addr, OS_SocketDomain_INET);
  OS_SocketAddrSetPort(&dest_addr, port_num);
  OS_SocketAddrFromString(&dest_addr, TELECOM_data.tlm_dest_ip);
  os_status = 0;

  do { 
    cfe_status = CFE_SB_ReceiveBuffer(&sb_buf_p, 
                                      TELECOM_data.tlm_pipe, 
                                      TELECOM_PLATFORM_TLM_PIPE_TIMEOUT);


    if (cfe_status == CFE_SUCCESS && TELECOM_data.suppress_sendto != false){
      os_status = OS_SUCCESS;

      if (TELECOM_data.downlink_on == true) {
        cfe_status = TELECOM_encode_output_message(
          sb_buf_p,
          &net_buf_p, &net_buf_s);  

        if (cfe_status != CFE_SUCCESS){
          CFE_EVS_SendEvent(TELECOM_ENCODE_ERR_EID, CFE_EVS_EventType_ERROR, 
                            "Error packing output: %d\n",
                            (int)cfe_status);
        } else { 
          os_status = OS_SocketSendTo(
            TELECOM_data.tlm_sock_id, 
            net_buf_p, net_buf_s, 
            &dest_addr);

          if (os_status < 0){
            CFE_EVS_SendEvent(TELECOM_SENDING_ERR_EID, CFE_EVS_EventType_ERROR,
                              "L%d TO sendto error %d. Tlm output error\n", __LINE__, (int)os_status);
            TELECOM_data.suppress_sendto = true;
          }
        }
      }
    }
    pkt_count++; 
  } while (cfe_status == CFE_SUCCESS && pkt_count < TELECOM_PLATFORM_MAX_TLM_PKTS);

}


void TELECOM_AppMain(void){
  int32 status;
  CFE_SB_Buffer_t *sb_buf_p;

  if (TELECOM_APP_Init() != CFE_SUCCESS){
    TELECOM_data.run_status = CFE_ES_RunStatus_APP_ERROR;     
  }

  while (CFE_ES_RunLoop(&TELECOM_data.run_status) == true){
    status = CFE_SB_ReceiveBuffer(&sb_buf_p, 
                                  TELECOM_data.tlm_pipe, 
                                  CFE_SB_PEND_FOREVER);

    if (status == CFE_SUCCESS){
      printf("FROM TELECOM:\n\tcpu temp %f\n\tmem used: %ld\n\tmem percentage: %f%%\n\tcpu usage: %f%%\n", 
             ((OBC_HK_HkPacket_t *)sb_buf_p)->cpu_temp,
             ((OBC_HK_HkPacket_t *)sb_buf_p)->ram_usage,
             ((OBC_HK_HkPacket_t *)sb_buf_p)->ram_usage_percent,
             ((OBC_HK_HkPacket_t *)sb_buf_p)->cpu_usage);
    }
  }

  CFE_ES_ExitApp(TELECOM_data.run_status);
}

