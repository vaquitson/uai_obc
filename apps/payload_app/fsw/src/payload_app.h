#ifndef PAYLOAD_APP_H
#define PAYLOAD_APP_H

#include <sys/un.h>

#include "cfe.h"
#include "payload_app_msgdefs.h"
#include "payload_app_platform_cfg.h"

#define PAYLOAD_APP_PIPE_NAME   "PAYLOAD_APP_PIPE"
#define PAYLOAD_APP_QUEUE_NAME  "PAYLOAD_REQ_Q"
#define PAYLOAD_APP_MUTEX_NAME  "PAYLOAD_HK_MUT"
#define PAYLOAD_APP_WORKER_NAME "PAYLOAD_WORKER"

#if PAYLOAD_APP_REQ_QUEUE_DEPTH > 10
#error "PAYLOAD_APP_REQ_QUEUE_DEPTH > 10 fails without root (/proc/sys/fs/mqueue/msg_max)"
#endif

/* One queued Payload API request; op is the command code (PAYLOAD_APP_*_CC) */
typedef struct {
  uint8  op;
  uint8  startup; /* startup GET_STATUS: retry connect forever, report version */
  uint16 length;
  uint32 offset;
  char   session_id[PAYLOAD_APP_SESSION_ID_LEN];
  uint8  spare[3];
} PAYLOAD_APP_Request_t;

typedef struct {
  CFE_SB_PipeId_t cmd_pipe;
  uint32          run_status;

  osal_id_t       req_queue;
  osal_id_t       hk_mutex; /* guards hk_packet.payload, written by both tasks */
  CFE_ES_TaskId_t worker_task;

  char socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];

  PAYLOAD_APP_HkPacket_t hk_packet;
} PAYLOAD_APP_GlobalApp_t;

extern PAYLOAD_APP_GlobalApp_t PAYLOAD_APP_Global;

CFE_Status_t PAYLOAD_APP_Init(void);
void         PAYLOAD_APP_AppMain(void);

void PAYLOAD_APP_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr);
void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr);

void PAYLOAD_APP_SendHk(void);

/* Take/give the HK mutex around every access to hk_packet.payload */
void PAYLOAD_APP_HkLock(void);
void PAYLOAD_APP_HkUnlock(void);

/* Operation name for a command code ("GET_STATUS", ...), NULL if not an API operation */
const char *PAYLOAD_APP_OpName(uint8 op);

#endif
