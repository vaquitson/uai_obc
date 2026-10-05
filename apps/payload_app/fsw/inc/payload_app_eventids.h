#ifndef PAYLOAD_APP_EVENTIDS_H
#define PAYLOAD_APP_EVENTIDS_H

#define PAYLOAD_APP_INIT_FAILURE_EID     1
#define PAYLOAD_APP_INIT_SUCCESSFUL_EID  2
#define PAYLOAD_APP_HK_TRANSMIT_ERR_EID  3

#define PAYLOAD_APP_NOOP_EID             4
#define PAYLOAD_APP_RESET_COUNTERS_EID   5
#define PAYLOAD_APP_INVALID_MID_EID      6
#define PAYLOAD_APP_INVALID_CC_EID       7

#define PAYLOAD_APP_SOCKET_PATH_EID         8  /* INFO: effective socket path at startup */
#define PAYLOAD_APP_SERVICE_UNAVAILABLE_EID 9  /* ERROR: connect() failed (throttled per errno) */
#define PAYLOAD_APP_SERVICE_RECOVERED_EID   10 /* INFO: connect() works again */
#define PAYLOAD_APP_REQ_TIMEOUT_EID         11 /* ERROR: request deadline expired */
#define PAYLOAD_APP_FRAME_ERR_EID           12 /* ERROR: malformed/oversized frame or peer closed */
#define PAYLOAD_APP_RID_MISMATCH_EID        13 /* ERROR: response request_id != sent request_id */
#define PAYLOAD_APP_STATUS_ERR_EID          14 /* ERROR: status_code 1..8 with data.message */
#define PAYLOAD_APP_BUSY_EXHAUSTED_EID      15 /* ERROR: BUSY on every attempt */
#define PAYLOAD_APP_VERSION_MISMATCH_EID    16 /* ERROR: interface_version != expected */
#define PAYLOAD_APP_VERSION_OK_EID          17 /* INFO: interface_version == expected */
#define PAYLOAD_APP_VALIDATION_ERR_EID      18 /* ERROR: command args rejected before sending */
#define PAYLOAD_APP_QUEUE_FULL_EID          19 /* ERROR: request queue full, command dropped */
#define PAYLOAD_APP_ACQ_INFO_EID            20 /* INFO: START/STOP data.message */
#define PAYLOAD_APP_LIST_EMPTY_EID          21 /* INFO: LIST_SESSIONS returned no sessions */
#define PAYLOAD_APP_READ_EOF_EID            22 /* INFO: READ_SESSION_RANGE reached eof */
#define PAYLOAD_APP_NOT_IMPLEMENTED_EID     23 /* ERROR: command code accepted but not implemented yet */
#define PAYLOAD_APP_JSON_ERR_EID            24 /* ERROR: response JSON invalid or unexpected */
#define PAYLOAD_APP_TLM_TRANSMIT_ERR_EID    25 /* ERROR: data TLM transmit failed */
#define PAYLOAD_APP_CMD_LEN_ERR_EID         26 /* ERROR: command packet length mismatch */
#define PAYLOAD_APP_BUSY_RETRY_EID          27 /* INFO: BUSY received, retrying */
#define PAYLOAD_APP_WORKER_ERR_EID          28 /* ERROR: worker/queue internal error or request discarded */

#endif
