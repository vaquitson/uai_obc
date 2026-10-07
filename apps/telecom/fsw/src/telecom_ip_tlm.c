#include "osapi.h"
#include "cfe_sb.h"
#include "cfe_evs.h"

#include "telecom_app.h"
#include "telecom_interface_cfg.h"
#include "telecom_internal_cfg.h"
#include "telecom_encode.h"
#include "telecom_eventids.h"

CFE_Status_t TELECOM_init_communication_dev(void){
  int32 status;
  static OS_SockAddr_t  listen_addr;

  strcpy(TELECOM_data.listening_port, TELECOM_MISSION_LISTENING_IP_PORT);

  status = OS_SocketOpen(&TELECOM_data.tlm_sock_id, 
                         OS_SocketDomain_INET, 
                         OS_SocketType_DATAGRAM);

  OS_SocketAddrInit(&listen_addr, OS_SocketDomain_INET);
  OS_SocketAddrSetPort(&listen_addr, atol(TELECOM_data.listening_port));
  OS_SocketAddrFromString(&listen_addr, TELECOM_MISSION_LISTENING_IP_ADDR);

  status = OS_SocketBind(TELECOM_data.tlm_sock_id, &listen_addr);

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

CFE_Status_t TELECOM_open_tlm(const void *ptr){
  // Const void *ptr
  // first 16 bytes are the IP addr 
  const TELECOM_OpenTlmCmd_Payload_t *data = (TELECOM_OpenTlmCmd_Payload_t *)ptr;
  
  TELECOM_data.downlink_on = true;
  strcpy(TELECOM_data.tlm_dest_ip, data->dest_IP);
  strcpy(TELECOM_data.tlm_port, data->dest_port);

  return CFE_SUCCESS;
}

void TELECOM_forward_tlm(void){
  OS_SockAddr_t  dest_addr;
  int32                 os_status = 0;
  uint32                pkt_count = 0;

  const void           *net_buf_p;
  size_t               net_buf_s;
  CFE_Status_t         cfe_status;
  CFE_SB_Buffer_t      *sb_buf_p;
  
  OS_SocketAddrInit(&dest_addr, OS_SocketDomain_INET);
  OS_SocketAddrFromString(&dest_addr, TELECOM_data.tlm_dest_ip); 
  OS_SocketAddrSetPort(&dest_addr, atoi(TELECOM_data.tlm_port));

  do { 
    cfe_status = CFE_SB_ReceiveBuffer(&sb_buf_p, 
                                      TELECOM_data.tlm_pipe, 
                                      TELECOM_PLATFORM_TLM_PIPE_TIMEOUT);


    if (cfe_status == CFE_SUCCESS){
      os_status = OS_SUCCESS;

      if (TELECOM_data.downlink_on == true) {
        cfe_status = TELECOM_encode_output_message(
          sb_buf_p,
          &net_buf_p, &net_buf_s);  

        if (cfe_status != CFE_SUCCESS){
          TELECOM_data.err_counter++;
          CFE_EVS_SendEvent(TELECOM_ENCODE_ERR_EID, CFE_EVS_EventType_ERROR, 
                            "Error packing output: %d\n",
                            (int)cfe_status);
        } else { 
          os_status = OS_SocketSendTo(
            TELECOM_data.tlm_sock_id, 
            net_buf_p, net_buf_s, 
            &dest_addr);

          if (os_status < 0){
            TELECOM_data.err_counter++;
            CFE_EVS_SendEvent(TELECOM_SENDING_ERR_EID, CFE_EVS_EventType_ERROR,
                              "TELECOM: 1 sendto error %d. Tlm output error\n", (int)os_status);
          }
        }
      }
    }
    pkt_count++; 
    TELECOM_data.tlm_paquet_counter++;
  } while (cfe_status == CFE_SUCCESS && pkt_count < TELECOM_PLATFORM_MAX_TLM_PKTS);

}



void TELECOM_forward_ground_cmd(void){
  char buffer[500];
  CFE_SB_Buffer_t *buff; 
  OS_SockAddr_t sender_addr;
  int32 read_size;

  read_size = OS_SocketRecvFrom(
    TELECOM_data.tlm_sock_id,
    buffer,
    sizeof(buffer),
    &sender_addr,
    OS_CHECK
  );

  buff = (CFE_SB_Buffer_t *)&buffer;

  if (read_size > 0) {
    CFE_SB_TransmitMsg(CFE_MSG_PTR(*buff), false); 
    TELECOM_data.cmd_ingest_counter++;
  }
}
