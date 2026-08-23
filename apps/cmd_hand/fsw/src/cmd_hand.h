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

    void * net_buf_ptr;
    size_t net_buf_size;

    uint32 ingest_errors;
    uint32 ingest_packets;

} CMD_HAND_GlobalData_t;


#endif
