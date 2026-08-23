#include "cmd_hand_decode.h"
#include "cmd_hand_internal_cfg.h"
#include "cmd_hand_eventids.h"

#include "cfe_msg.h"
#include "cfe_sb.h"
#include "cfe_evs.h"


CFE_Status_t CMD_HAND_get_input_buf(void **buf_out, size_t *size_out){
    CFE_SB_Buffer_t *ingest_buf;
    const size_t     IngestSize = CMD_HAND_MAX_INGEST;

    ingest_buf = CFE_SB_AllocateMessageBuffer(IngestSize);
    if (ingest_buf == NULL){
        *buf_out = NULL;
        *size_out   = 0;

        CFE_EVS_SendEvent(CMD_HAND_INGEST_BUF_ALLOC_ERR, 
                          CFE_EVS_EventType_ERROR, 
                          "CMD HAND: buffer allocation failed, RC = 0x%08X, %s|%d \n",
                          CFE_SB_BUF_ALOC_ERR,
                          __FILE__,
                          __LINE__);

        return CFE_SB_BUF_ALOC_ERR;
    }

    *buf_out =  ingest_buf;
    *size_out = IngestSize;

    return CFE_SUCCESS;
}


CFE_Status_t CMD_HAND_decode_input_msg(void *source_buf_p,
                                       size_t source_size, 
                                       CFE_SB_Buffer_t **dest_buf_out){
  CFE_SB_Buffer_t *msg_buf_p;
  CFE_MSG_Size_t   msg_size;
  CFE_Status_t     status;

  if (source_size < sizeof(CFE_MSG_CommandHeader_t)){
    msg_buf_p = NULL;
    status    = CFE_STATUS_WRONG_MSG_LENGTH;

    CFE_EVS_SendEvent(CMD_HAND_INGEST_LENGTH_ERR_EID, CFE_EVS_EventType_ERROR,
                      "CMD HAND: cmd dropped, bad packet length=%lu\n", (unsigned long)source_size);
  } else {
    msg_buf_p = source_buf_p;

    /* Check the size from within the header itself, compare against network buffer size */
    CFE_MSG_GetSize(&msg_buf_p->Msg, &msg_size);

    if (msg_size > source_size) {
      status = CFE_STATUS_WRONG_MSG_LENGTH;

      CFE_EVS_SendEvent(CMD_HAND_INGEST_LENGTH_ERR_EID, CFE_EVS_EventType_ERROR,
                        "CMD HAND: cmd dropped - length mismatch, %lu (hdr) / %lu (packet)\n", (unsigned long)msg_size,
                        (unsigned long)source_size);
    } else {
      status = CFE_SUCCESS;
    }
  }

  *dest_buf_out = msg_buf_p;

  return status;
}
