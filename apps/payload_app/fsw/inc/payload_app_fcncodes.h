#ifndef PAYLOAD_APP_FCNCODES_H
#define PAYLOAD_APP_FCNCODES_H

/** No-op, only bumps the command counter and issues an event */
#define PAYLOAD_APP_NOOP_CC 0

/** Reset the cmd/err/req_ok/req_err counters */
#define PAYLOAD_APP_RESET_COUNTERS_CC 1

/* One command code per Payload API operation; the value is also the internal op id */
#define PAYLOAD_APP_GET_STATUS_CC         2
#define PAYLOAD_APP_START_ACQUISITION_CC  3
#define PAYLOAD_APP_STOP_ACQUISITION_CC   4
#define PAYLOAD_APP_GET_LIVE_STATE_CC     5
#define PAYLOAD_APP_LIST_SESSIONS_CC      6
#define PAYLOAD_APP_GET_SESSION_INFO_CC   7
#define PAYLOAD_APP_READ_SESSION_RANGE_CC 8

#endif
