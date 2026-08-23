#ifndef CMD_HAND_DECODE_H
#define CMD_HAND_DECODE_H

#include "common_types.h"
#include "cfe_sb_api_typedefs.h"

/**
 * Get a pointer to one of the software bus' internal 
 * memory buffers that are used for sending messages
 *
 * In a "passthrough" configuration - the data from the network
 * is expected to be a direct instance of the CFE_MSG_Message_t base,
 * and thus something can be sent directly to SB.
 *
 * Instead of using an intermediate buffer, just get a buffer from
 * SB and put it directly in there.  This reduces copying.
*/
CFE_Status_t CMD_HAND_get_input_buf(void **buf_out, size_t *size_out);


/**
 * In a "passthrough" configuration - the data from the network
 * is expected to be a direct instance of the CFE_MSG_Message_t base,
 * and thus something can be sent directly to SB.
 *
 * This just does a simple sanity check on the message size.  But
 * otherwise, the source buffer is used directly as the output buffer.
 */
CFE_Status_t CMD_HAND_decode_input_msg(void *source_buf_p,
                                       size_t source_size, 
                                       CFE_SB_Buffer_t **dest_buf_out);

#endif
