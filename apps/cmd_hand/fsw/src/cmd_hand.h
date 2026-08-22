#ifndef CMD_HAND_H
#define CMD_HAND_H


#include "common_types.h"
#include "cfe_sb.h"

typedef struct
{
    bool            sock_listening;
    bool            Scheduled;
    bool            AllowPassthrough;

    CFE_SB_PipeId_t cmd_pipe;
    uint32          run_status;

    osal_id_t       tc_sock_id;
    OS_SockAddr_t   sock_addr;

    void * NetBufPtr;
    size_t NetBufSize;

} CMD_HAND_GlobalData_t;

#define CMD_HAND_PLAFORM_PIPE_DEPTH 32

#endif
