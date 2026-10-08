#include "osapi.h"
#include "cfe_sb.h"
#include "cfe_evs.h"

#include "telecom_app.h"
#include "telecom_lora_controller.h"

#include "telecom_interface_cfg.h"
#include "telecom_eventids.h"
#include "telecom_encode.h"
#include "telecom_internal_cfg.h"
#include "telecom_msg.h"


CFE_Status_t TELECOM_init_communication_dev(void){
  int rc;
  rc = lora_controller_init(&TELECOM_data.controller, TELECOM_MISSION_TLM_LORA_DEVICE_PATH); 

  if (rc > 0){
    lora_controller_set_uplink_freq(&(TELECOM_data.controller), TELECOM_MISSION_DEFAULt_UPLINK_FREQ);
    CFE_EVS_SendEvent(TELECOM_SUCCESS_EID, 
                      CFE_EVS_EventType_INFORMATION, 
                      "LoRa Controller Ready For Uplink");

    return CFE_SUCCESS;
  } else {
    CFE_EVS_SendEvent(TELECOM_OPEN_ERR_EID,
                      CFE_EVS_EventType_ERROR,
                      "LoRa Controller faild initialization");

    return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
  }

}

CFE_Status_t TELECOM_open_tlm(const void *ptr){
  // const void *ptr
  // first 16 bytes are the downlink frequency
  // next 16 bytes are the uplink frequency

  const TELECOM_OpenTlmCmd_Payload_t *payload;
  
  payload = (TELECOM_OpenTlmCmd_Payload_t *)ptr;

  lora_controller_set_uplink_freq(&(TELECOM_data.controller), payload->downlink_freq);
  lora_controller_set_downlik_freq(&(TELECOM_data.controller), payload->uplink_freq);

  TELECOM_data.downlink_on = true;
  
  return CFE_SUCCESS;
}

void TELECOM_forward_tlm(void){
  CFE_SB_Buffer_t *sb_buf_p;
  size_t           bytes;
  uint32           pkt_count = 0;
  CFE_Status_t     cfe_status;

  do {
    cfe_status = CFE_SB_ReceiveBuffer(&sb_buf_p, 
                                      TELECOM_data.tlm_pipe, 
                                      TELECOM_PLATFORM_TLM_PIPE_TIMEOUT);
    
    if (cfe_status == CFE_SUCCESS){
      bytes = lora_controller_send(&TELECOM_data.controller, (char *)(&sb_buf_p), sizeof(CFE_SB_Buffer_t));
      if (bytes < 0){
        CFE_EVS_SendEvent(TELECOM_SENDING_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "LoRa sending error");
        cfe_status = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
      } else {
        CFE_EVS_SendEvent(TELECOM_SUCCESS_EID,
                          CFE_EVS_EventType_INFORMATION,
                          "LoRa send %ld bytes", bytes);

      }
    }

    pkt_count++; 
  } while(cfe_status == CFE_SUCCESS && pkt_count < TELECOM_PLATFORM_MAX_TLM_PKTS);
}


void TELECOM_forward_ground_cmd(void){
  CFE_SB_Buffer_t buff; 
  int32 rc;
  int32 read_size;

  read_size = lora_controller_recv(&TELECOM_data.controller, (char *)&buff, sizeof(buff), &rc);
  if (read_size > 0) {
    CFE_SB_TransmitMsg(CFE_MSG_PTR(buff), false);
    TELECOM_data.cmd_ingest_counter++;
  }
}


