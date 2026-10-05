#ifndef DEFAULT_PAYLOAD_APP_MSGDEFS_H
#define DEFAULT_PAYLOAD_APP_MSGDEFS_H

#include "cfe_sb.h"
#include "payload_app_platform_cfg.h"

/*
** All packet payloads use fixed-width types laid out so that every field is
** naturally aligned and the size is a multiple of 8: identical on i686,
** x86-64 and aarch64, with no implicit compiler padding. Sizes are checked
** at compile time in payload_app.c. Offsets are documented in README.md.
*/

/** Length of a session_id string "YYYYMMDDTHHMMSSZ" plus NUL */
#define PAYLOAD_APP_SESSION_ID_LEN 17

/** Bumped whenever a TLM layout below changes */
#define PAYLOAD_APP_TLM_LAYOUT_VERSION 1

/** Value of the HK state/flag bytes while unknown */
#define PAYLOAD_APP_UNKNOWN 0xFF

/*
** Command function codes are defined in payload_app_fcncodes.h
*/

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
} PAYLOAD_APP_NoArgsCmd_t;

typedef struct {
  char  session_id[PAYLOAD_APP_SESSION_ID_LEN];
  uint8 spare[3];
} PAYLOAD_APP_SessionInfoCmd_Payload_t;

typedef struct {
  CFE_MSG_CommandHeader_t              command_header;
  PAYLOAD_APP_SessionInfoCmd_Payload_t payload;
} PAYLOAD_APP_SessionInfoCmd_t;

typedef struct {
  char   session_id[PAYLOAD_APP_SESSION_ID_LEN];
  uint8  spare[3];
  uint32 offset;
  uint16 length;
  uint16 spare2;
} PAYLOAD_APP_ReadRangeCmd_Payload_t;

typedef struct {
  CFE_MSG_CommandHeader_t            command_header;
  PAYLOAD_APP_ReadRangeCmd_Payload_t payload;
} PAYLOAD_APP_ReadRangeCmd_t;

/*
** Housekeeping (64 B payload). States: payload_state 0 READY / 1 ACQUIRING,
** receiver/logger 0 INACTIVE / 1 ACTIVE, flags 0/1; PAYLOAD_APP_UNKNOWN until
** the first successful GET_STATUS (or when the field came back null/invalid).
*/
typedef struct {
  uint8  cmd_counter;
  uint8  err_counter;
  uint8  payload_state;
  uint8  receiver_state;
  uint8  logger_state;
  uint8  live_consumer_connected;
  uint8  last_status_code;
  uint8  version_ok;

  uint32 req_ok_count;
  uint32 req_err_count;
  uint32 last_request_id;

  uint16 live_aircraft_count;
  uint8  service_available;
  uint8  tlm_layout_version;

  uint64 storage_free_bytes;

  char   active_session_id[PAYLOAD_APP_SESSION_ID_LEN];
  char   interface_version[8];
  uint8  spare[7];
} PAYLOAD_APP_HkTlm_Payload_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t   telemetry_header;
  PAYLOAD_APP_HkTlm_Payload_t payload;
} PAYLOAD_APP_HkPacket_t;

#endif
