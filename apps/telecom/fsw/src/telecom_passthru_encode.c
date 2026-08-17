#include "telecom_encode.h"

CFE_Status_t TELECOM_encode_output_message(
    const CFE_SB_Buffer_t  *source_buf_p, 
    const void            **dest_buf_out,
    size_t                 *dest_size_out)
{
    CFE_Status_t   result_status;
    CFE_MSG_Size_t source_buff_size;

    result_status = CFE_MSG_GetSize(&source_buf_p->Msg, &source_buff_size);

    *dest_buf_out = source_buf_p;
    *dest_size_out   = source_buff_size;

    return result_status;
}


