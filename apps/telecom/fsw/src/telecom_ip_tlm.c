#include "osapi.h"
#include "cfe_sb.h"
#include "cfe_evs.h"

#include "telecom_app.h"
#include "telecom_interface_cfg.h"
#include "telecom_encode.h"
#include "telecom_eventids.h"


CFE_Status_t TELECOM_APP_open_telemetry(void){
  int32 status;
  status = OS_SocketOpen(&TELECOM_data.tlm_sock_id, 
                         OS_SocketDomain_INET, 
                         OS_SocketType_DATAGRAM);

  if (status != OS_SUCCESS){
    CFE_EVS_SendEvent(TELECOM_TLM_SOCK_ERR_EID, 
                      CFE_EVS_EventType_ERROR, 
                      "L%d, TO TLM socket error: %d",
                      __LINE__, (int)status);    
  } else {
    CFE_EVS_SendEvent(TELECOM_SUCCESS_EID, 
                      CFE_EVS_EventType_INFORMATION, 
                      "TLM INET SOCKET initialized");    

  }
  return status;
}

void TELECOM_APP_forward_telemetry(void){
  OS_SockAddr_t    dest_addr;
  int32            os_status;
  uint32           pkt_count = 0;
  uint16           port_num = TELECOM_MISSION_TLM_IP_PORT; 

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
