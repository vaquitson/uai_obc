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

CFE_Status_t TELECOM_open_tlm(const void *ptr){
  // const void *ptr
  // first 16 bytes are the downlink frequency
  // next 16 bytes are the uplink frequency

  const TELECOM_OpenTlmCmd_Payload_t *payload;
  int rc;
  rc = lora_controller_init(&TELECOM_data.controller, TELECOM_MISSION_TLM_LORA_DEVICE_PATH);
  
  payload = (TELECOM_OpenTlmCmd_Payload_t *)ptr;

  memcpy(
    TELECOM_data.controller.downlink_freq,
    payload->downlink_freq,
    10);

  memcpy(
    TELECOM_data.controller.uplink_freq,
    payload->uplink_freq,
    10);
  
  if (rc > 0){
    CFE_EVS_SendEvent(TELECOM_SUCCESS_EID, 
                      CFE_EVS_EventType_INFORMATION, 
                      "LoRa Controller Ready");

    TELECOM_data.downlink_on = true;
    return CFE_SUCCESS;
  } else {
    CFE_EVS_SendEvent(TELECOM_OPEN_ERR_EID,
                      CFE_EVS_EventType_ERROR,
                      "LoRa Controller faild initialization");

    return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
  }
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
