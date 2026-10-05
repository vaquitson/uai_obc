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

/* PAYLOAD_APP_LiveTlm_Payload_t.valid_flags: a clear bit means the field was null, missing or invalid */
#define PAYLOAD_APP_LIVE_VALID_CALLSIGN          (1u << 0)
#define PAYLOAD_APP_LIVE_VALID_LATITUDE          (1u << 1)
#define PAYLOAD_APP_LIVE_VALID_LONGITUDE         (1u << 2)
#define PAYLOAD_APP_LIVE_VALID_ALTITUDE_FT       (1u << 3)
#define PAYLOAD_APP_LIVE_VALID_GROUND_SPEED_KT   (1u << 4)
#define PAYLOAD_APP_LIVE_VALID_HEADING_DEG       (1u << 5)
#define PAYLOAD_APP_LIVE_VALID_VERTICAL_RATE_FPM (1u << 6)
#define PAYLOAD_APP_LIVE_VALID_SQUAWK            (1u << 7)
#define PAYLOAD_APP_LIVE_VALID_IS_ON_GROUND      (1u << 8)
#define PAYLOAD_APP_LIVE_VALID_LAST_SEEN_UTC     (1u << 9)
#define PAYLOAD_APP_LIVE_VALID_AGE_S             (1u << 10)
#define PAYLOAD_APP_LIVE_VALID_TIMESTAMP_UTC     (1u << 11) /* response level */
#define PAYLOAD_APP_LIVE_VALID_TTL_S             (1u << 12) /* response level */

/*
** One GET_LIVE_STATE aircraft (80 B payload), lowest age_s first. aircraft_count is the
** total reported by Payload, even when fewer packets are published. Times are UTC epoch seconds.
*/
typedef struct {
  uint32 request_id;
  uint16 aircraft_index;
  uint16 aircraft_count;
  uint32 valid_flags;
  uint32 timestamp_utc;
  double latitude;
  double longitude;
  int32  altitude_ft;
  float  ground_speed_kt;
  float  heading_deg;
  float  vertical_rate_fpm;
  float  age_s;
  uint32 last_seen_utc;
  uint16 ttl_s;
  uint8  is_on_ground;
  char   icao[7];
  char   callsign[9];
  char   squawk[5];
} PAYLOAD_APP_LiveTlm_Payload_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t     telemetry_header;
  PAYLOAD_APP_LiveTlm_Payload_t payload;
} PAYLOAD_APP_LivePacket_t;

/* PAYLOAD_APP_SessionTlm_Payload_t.valid_flags */
#define PAYLOAD_APP_SESSION_VALID_START_UTC           (1u << 0)
#define PAYLOAD_APP_SESSION_VALID_END_UTC             (1u << 1)
#define PAYLOAD_APP_SESSION_VALID_DURATION_S          (1u << 2)
#define PAYLOAD_APP_SESSION_VALID_MESSAGES            (1u << 3)
#define PAYLOAD_APP_SESSION_VALID_UNIQUE_AIRCRAFT     (1u << 4)
#define PAYLOAD_APP_SESSION_VALID_MESSAGES_PER_MINUTE (1u << 5)
#define PAYLOAD_APP_SESSION_VALID_FILE_SIZE_BYTES     (1u << 6)
#define PAYLOAD_APP_SESSION_VALID_RECONNECTS          (1u << 7)
#define PAYLOAD_APP_SESSION_VALID_SOFTWARE_VERSION    (1u << 8)

#define PAYLOAD_APP_SESSION_SOURCE_LIST 0
#define PAYLOAD_APP_SESSION_SOURCE_INFO 1

/* One LIST_SESSIONS entry or the GET_SESSION_INFO result (80 B payload) */
typedef struct {
  uint32 request_id;
  uint16 session_index;
  uint16 session_count;
  uint32 valid_flags;
  uint8  source;
  char   session_id[PAYLOAD_APP_SESSION_ID_LEN];
  uint16 spare;
  uint32 start_utc;
  uint32 end_utc;
  uint32 duration_s;
  uint32 messages;
  uint32 unique_aircraft;
  float  messages_per_minute;
  uint64 file_size_bytes;
  uint32 reconnects;
  char   software_version[12];
} PAYLOAD_APP_SessionTlm_Payload_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t        telemetry_header;
  PAYLOAD_APP_SessionTlm_Payload_t payload;
} PAYLOAD_APP_SessionPacket_t;

/* READ_SESSION_RANGE bytes, split in PAYLOAD_APP_CHUNK_TLM_MAX pieces (40 B + data) */
typedef struct {
  uint32 request_id;
  uint16 n_bytes;
  uint8  eof; /* set only on the last chunk of a response that reached end of file */
  uint8  spare;
  uint64 offset; /* absolute offset of data[0] in the session file */
  char   session_id[PAYLOAD_APP_SESSION_ID_LEN];
  uint8  spare2[7];
  uint8  data[PAYLOAD_APP_CHUNK_TLM_MAX];
} PAYLOAD_APP_ChunkTlm_Payload_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t      telemetry_header;
  PAYLOAD_APP_ChunkTlm_Payload_t payload;
} PAYLOAD_APP_ChunkPacket_t;

#endif
