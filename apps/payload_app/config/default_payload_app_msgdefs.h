#ifndef DEFAULT_PAYLOAD_APP_MSGDEFS_H
#define DEFAULT_PAYLOAD_APP_MSGDEFS_H

#include "cfe_sb.h"
#include "payload_app_platform_cfg.h"

/*
** Command function codes are defined in payload_app_fcncodes.h
*/

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
} PAYLOAD_APP_NoArgsCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t telemetry_header;

  uint8  cmd_counter;
  uint8  err_counter;
  uint8  socket_connected;
  uint8  spare;

  uint32 msgs_received;
  uint32 parse_errors;

  int32  last_request_id;
  uint8  last_status_code;
} PAYLOAD_APP_HkPacket_t;

/*
** Telemetry published from each Payload socket message. Mirrors the
** request/response envelope {request_id, status, status_code, data}
** delivered by Payload over the Unix socket, with "data" parsed into a
** provisional ADS-B-like typed schema (no official schema from Payload
** yet - revise field list once one is defined).
*/
typedef struct {
  CFE_MSG_TelemetryHeader_t telemetry_header;

  int32  request_id;
  uint8  status_code;
  char   status[PAYLOAD_APP_STATUS_LEN];

  char   icao24[PAYLOAD_APP_ICAO24_LEN];
  char   callsign[PAYLOAD_APP_CALLSIGN_LEN];

  double latitude;
  double longitude;
  float  altitude;
  float  ground_speed;
  float  heading;
  float  vertical_rate;
  double timestamp;
} PAYLOAD_APP_DataMsg_t;

#endif
