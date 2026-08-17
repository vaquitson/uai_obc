#ifndef TELECOM_ENCODE_H
#define TELECOM_ENCODE_H

#include "common_types.h"
#include "cfe_msg.h"
#include "cfe_error.h"

CFE_Status_t TELECOM_encode_output_message(
    const CFE_SB_Buffer_t  *SourceBuffer, 
    const void            **DestBufferOut,
    size_t                 *DestSizeOut);

#endif
