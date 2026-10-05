#include <stdio.h>
#include <string.h>

#include "cfe.h"

#include "payload_api_json.h"
#include "payload_app.h"
#include "payload_app_eventids.h"
#include "payload_app_msg.h"
#include "payload_app_tlm.h"
#include "payload_base64.h"

/* Max READ_SESSION_RANGE length and its Base64 size (4096 B -> 5464 chars) */
#define PAYLOAD_APP_READ_MAX       4096
#define PAYLOAD_APP_READ_B64_MAX   (((PAYLOAD_APP_READ_MAX + 2) / 3) * 4)

/* Sort key for aircraft without age_s: after every real age */
#define PAYLOAD_APP_AGE_UNKNOWN 1.0e30

static PAYLOAD_APP_LivePacket_t    PAYLOAD_APP_LivePkt;
static PAYLOAD_APP_SessionPacket_t PAYLOAD_APP_SessionPkt;
static PAYLOAD_APP_ChunkPacket_t   PAYLOAD_APP_ChunkPkt;

typedef struct {
  PAYLOAD_APP_LiveTlm_Payload_t payload;
  double                        age_key;
} PAYLOAD_APP_LiveEntry_t;

/* Worker-only scratch (static: never on the child task stack) */
static PAYLOAD_APP_LiveEntry_t PAYLOAD_APP_LiveBest[PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ];
static char                    PAYLOAD_APP_B64Text[PAYLOAD_APP_READ_B64_MAX + 1];
static uint8                   PAYLOAD_APP_ReadBytes[PAYLOAD_APP_READ_MAX];

void PAYLOAD_APP_TlmInit(void)
{
  CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_LivePkt.telemetry_header), CFE_SB_ValueToMsgId(PAYLOAD_APP_DATA_TLM_MID),
               sizeof(PAYLOAD_APP_LivePkt));
  CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_SessionPkt.telemetry_header),
               CFE_SB_ValueToMsgId(PAYLOAD_APP_SESSION_TLM_MID), sizeof(PAYLOAD_APP_SessionPkt));
  CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_ChunkPkt.telemetry_header), CFE_SB_ValueToMsgId(PAYLOAD_APP_CHUNK_TLM_MID),
               sizeof(PAYLOAD_APP_ChunkPkt));
}

static bool PAYLOAD_APP_JsonErr(const char *op, uint32 request_id, const char *what)
{
  CFE_EVS_SendEvent(PAYLOAD_APP_JSON_ERR_EID, CFE_EVS_EventType_ERROR, "%s rid=%lu: %s", op,
                    (unsigned long)request_id, what);
  return false;
}

/* Publishes one packet; 'more' spaces bursts so to_lab's pipe is not overrun */
static void PAYLOAD_APP_Publish(CFE_MSG_TelemetryHeader_t *hdr, bool more)
{
  CFE_MSG_Size_t    size   = 0;
  CFE_SB_MsgId_t    msg_id = CFE_SB_INVALID_MSG_ID;
  int32             status = CFE_SB_TransmitMsg(CFE_MSG_PTR(*hdr), true);

  if (status != CFE_SUCCESS)
  {
    CFE_MSG_GetMsgId(CFE_MSG_PTR(*hdr), &msg_id);
    CFE_MSG_GetSize(CFE_MSG_PTR(*hdr), &size);
    CFE_EVS_SendEvent(PAYLOAD_APP_TLM_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TLM 0x%04X (%lu B) transmit error, RC = 0x%08lX",
                      (unsigned int)CFE_SB_MsgIdToValue(msg_id), (unsigned long)size, (unsigned long)status);
  }
  if (more)
    OS_TaskDelay(PAYLOAD_APP_TLM_BURST_DELAY_MS);
}

static bool PAYLOAD_APP_GetDouble(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key, double *out)
{
  return PAYLOAD_JSON_TokDouble(doc, PAYLOAD_JSON_Find(doc, obj, key), out) == PAYLOAD_JSON_FIELD_VALID;
}

static bool PAYLOAD_APP_GetU32(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key, uint32 *out)
{
  double v;

  /* numbers like durations may come as 1800 or 1800.0 */
  if (!PAYLOAD_APP_GetDouble(doc, obj, key, &v) || v < 0.0 || v > 4294967295.0)
    return false;
  *out = (uint32)v;
  return true;
}

static bool PAYLOAD_APP_GetString(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key, char *out, size_t size)
{
  return PAYLOAD_JSON_TokString(doc, PAYLOAD_JSON_Find(doc, obj, key), out, size) == PAYLOAD_JSON_FIELD_VALID;
}

static bool PAYLOAD_APP_GetIso(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key, uint32 *out)
{
  char text[40];

  return PAYLOAD_APP_GetString(doc, obj, key, text, sizeof(text)) && PAYLOAD_JSON_IsoToEpoch(text, out);
}

static int PAYLOAD_APP_ArrayOf(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key)
{
  int idx = PAYLOAD_JSON_Find(doc, obj, key);

  return (idx >= 0 && doc->tok[idx].type == JSMN_ARRAY) ? idx : -1;
}

/* ---------------------------------------------------------------- live */

static bool PAYLOAD_APP_ParseAircraft(const PAYLOAD_JSON_Doc_t *doc, int obj, PAYLOAD_APP_LiveEntry_t *e)
{
  PAYLOAD_APP_LiveTlm_Payload_t *p = &e->payload;
  double                         v;
  int                            b;

  memset(e, 0, sizeof(*e));
  e->age_key = PAYLOAD_APP_AGE_UNKNOWN;

  if (doc->tok[obj].type != JSMN_OBJECT || !PAYLOAD_APP_GetString(doc, obj, "icao", p->icao, sizeof(p->icao)) ||
      p->icao[0] == '\0')
    return false;

  if (PAYLOAD_APP_GetString(doc, obj, "callsign", p->callsign, sizeof(p->callsign)))
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_CALLSIGN;
  if (PAYLOAD_APP_GetDouble(doc, obj, "latitude", &p->latitude))
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_LATITUDE;
  if (PAYLOAD_APP_GetDouble(doc, obj, "longitude", &p->longitude))
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_LONGITUDE;
  if (PAYLOAD_APP_GetDouble(doc, obj, "altitude_ft", &v) && v > -2147483648.0 && v < 2147483647.0)
  {
    p->altitude_ft = (int32)(v < 0 ? v - 0.5 : v + 0.5);
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_ALTITUDE_FT;
  }
  if (PAYLOAD_APP_GetDouble(doc, obj, "ground_speed_kt", &v))
  {
    p->ground_speed_kt = (float)v;
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_GROUND_SPEED_KT;
  }
  if (PAYLOAD_APP_GetDouble(doc, obj, "heading_deg", &v))
  {
    p->heading_deg = (float)v;
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_HEADING_DEG;
  }
  if (PAYLOAD_APP_GetDouble(doc, obj, "vertical_rate_fpm", &v))
  {
    p->vertical_rate_fpm = (float)v;
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_VERTICAL_RATE_FPM;
  }
  if (PAYLOAD_APP_GetString(doc, obj, "squawk", p->squawk, sizeof(p->squawk)))
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_SQUAWK;
  if (PAYLOAD_JSON_TokBool(doc, PAYLOAD_JSON_Find(doc, obj, "is_on_ground"), &b) == PAYLOAD_JSON_FIELD_VALID)
  {
    p->is_on_ground = (uint8)b;
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_IS_ON_GROUND;
  }
  if (PAYLOAD_APP_GetIso(doc, obj, "last_seen_utc", &p->last_seen_utc))
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_LAST_SEEN_UTC;
  if (PAYLOAD_APP_GetDouble(doc, obj, "age_s", &v) && v >= 0.0)
  {
    p->age_s   = (float)v;
    e->age_key = v;
    p->valid_flags |= PAYLOAD_APP_LIVE_VALID_AGE_S;
  }
  return true;
}

/* Keeps the PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ entries with the lowest age (stable for equal ages) */
static void PAYLOAD_APP_LiveKeep(const PAYLOAD_APP_LiveEntry_t *e, int *kept)
{
  int pos = *kept;

  while (pos > 0 && PAYLOAD_APP_LiveBest[pos - 1].age_key > e->age_key)
    pos--;
  if (pos >= PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ)
    return;
  if (*kept < PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ)
    (*kept)++;
  memmove(&PAYLOAD_APP_LiveBest[pos + 1], &PAYLOAD_APP_LiveBest[pos],
          (size_t)(*kept - 1 - pos) * sizeof(PAYLOAD_APP_LiveBest[0]));
  PAYLOAD_APP_LiveBest[pos] = *e;
}

bool PAYLOAD_APP_TlmLive(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id)
{
  PAYLOAD_APP_LiveEntry_t entry;
  uint32                  header_flags = 0;
  uint32                  timestamp    = 0;
  uint32                  ttl          = 0;
  uint32                  count;
  int                     arr;
  int                     item;
  int                     n;
  int                     kept    = 0;
  int                     skipped = 0;
  int                     i;

  arr = PAYLOAD_APP_ArrayOf(doc, data, "aircraft");
  if (arr < 0)
    return PAYLOAD_APP_JsonErr("GET_LIVE_STATE", request_id, "aircraft missing or not an array");

  if (PAYLOAD_APP_GetIso(doc, data, "timestamp_utc", &timestamp))
    header_flags |= PAYLOAD_APP_LIVE_VALID_TIMESTAMP_UTC;
  if (PAYLOAD_APP_GetU32(doc, data, "ttl_s", &ttl) && ttl <= 0xFFFF)
    header_flags |= PAYLOAD_APP_LIVE_VALID_TTL_S;
  if (!PAYLOAD_APP_GetU32(doc, data, "aircraft_count", &count))
    count = (uint32)doc->tok[arr].size;
  if (count > 0xFFFF)
    count = 0xFFFF;

  item = arr + 1;
  for (n = 0; n < doc->tok[arr].size; n++)
  {
    if (PAYLOAD_APP_ParseAircraft(doc, item, &entry))
      PAYLOAD_APP_LiveKeep(&entry, &kept);
    else
      skipped++;
    item = PAYLOAD_JSON_Next(doc, item);
  }

  PAYLOAD_APP_HkLock();
  PAYLOAD_APP_Global.hk_packet.payload.live_aircraft_count = (uint16)count;
  PAYLOAD_APP_HkUnlock();

  if (skipped > 0)
    CFE_EVS_SendEvent(PAYLOAD_APP_JSON_ERR_EID, CFE_EVS_EventType_ERROR,
                      "GET_LIVE_STATE rid=%lu: %d aircraft without a valid icao skipped", (unsigned long)request_id,
                      skipped);

  for (i = 0; i < kept; i++)
  {
    PAYLOAD_APP_LivePkt.payload                = PAYLOAD_APP_LiveBest[i].payload;
    PAYLOAD_APP_LivePkt.payload.request_id     = request_id;
    PAYLOAD_APP_LivePkt.payload.aircraft_index = (uint16)i;
    PAYLOAD_APP_LivePkt.payload.aircraft_count = (uint16)count;
    PAYLOAD_APP_LivePkt.payload.timestamp_utc  = timestamp;
    PAYLOAD_APP_LivePkt.payload.ttl_s          = (uint16)ttl;
    PAYLOAD_APP_LivePkt.payload.valid_flags |= header_flags;
    PAYLOAD_APP_Publish(&PAYLOAD_APP_LivePkt.telemetry_header, i + 1 < kept);
  }
  return true;
}

/* ---------------------------------------------------------------- sessions */

static void PAYLOAD_APP_ParseSession(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *fallback_id,
                                     PAYLOAD_APP_SessionTlm_Payload_t *p)
{
  double   v;
  uint64_t u;

  memset(p, 0, sizeof(*p));
  if (!PAYLOAD_APP_GetString(doc, obj, "session_id", p->session_id, sizeof(p->session_id)) && fallback_id != NULL)
    strncpy(p->session_id, fallback_id, sizeof(p->session_id) - 1);

  if (PAYLOAD_APP_GetIso(doc, obj, "start_utc", &p->start_utc))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_START_UTC;
  if (PAYLOAD_APP_GetIso(doc, obj, "end_utc", &p->end_utc))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_END_UTC;
  if (PAYLOAD_APP_GetU32(doc, obj, "duration_s", &p->duration_s))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_DURATION_S;
  if (PAYLOAD_APP_GetU32(doc, obj, "messages", &p->messages))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_MESSAGES;
  if (PAYLOAD_APP_GetU32(doc, obj, "unique_aircraft", &p->unique_aircraft))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_UNIQUE_AIRCRAFT;
  if (PAYLOAD_APP_GetDouble(doc, obj, "messages_per_minute", &v))
  {
    p->messages_per_minute = (float)v;
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_MESSAGES_PER_MINUTE;
  }
  if (PAYLOAD_JSON_TokU64(doc, PAYLOAD_JSON_Find(doc, obj, "file_size_bytes"), &u) == PAYLOAD_JSON_FIELD_VALID)
  {
    p->file_size_bytes = u;
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_FILE_SIZE_BYTES;
  }
  if (PAYLOAD_APP_GetU32(doc, obj, "reconnects", &p->reconnects))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_RECONNECTS;
  if (PAYLOAD_APP_GetString(doc, obj, "software_version", p->software_version, sizeof(p->software_version)))
    p->valid_flags |= PAYLOAD_APP_SESSION_VALID_SOFTWARE_VERSION;
}

bool PAYLOAD_APP_TlmSessionList(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id)
{
  uint32 count;
  int    arr;
  int    item;
  int    n;

  arr = PAYLOAD_APP_ArrayOf(doc, data, "sessions");
  if (arr < 0)
    return PAYLOAD_APP_JsonErr("LIST_SESSIONS", request_id, "sessions missing or not an array");
  if (!PAYLOAD_APP_GetU32(doc, data, "session_count", &count))
    count = (uint32)doc->tok[arr].size;
  if (count > 0xFFFF)
    count = 0xFFFF;

  if (doc->tok[arr].size == 0)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_LIST_EMPTY_EID, CFE_EVS_EventType_INFORMATION,
                      "LIST_SESSIONS rid=%lu: no stored sessions", (unsigned long)request_id);
    return true;
  }

  item = arr + 1;
  for (n = 0; n < doc->tok[arr].size; n++)
  {
    if (doc->tok[item].type == JSMN_OBJECT)
    {
      PAYLOAD_APP_ParseSession(doc, item, NULL, &PAYLOAD_APP_SessionPkt.payload);
      PAYLOAD_APP_SessionPkt.payload.request_id    = request_id;
      PAYLOAD_APP_SessionPkt.payload.session_index = (uint16)n;
      PAYLOAD_APP_SessionPkt.payload.session_count = (uint16)count;
      PAYLOAD_APP_SessionPkt.payload.source        = PAYLOAD_APP_SESSION_SOURCE_LIST;
      PAYLOAD_APP_Publish(&PAYLOAD_APP_SessionPkt.telemetry_header, n + 1 < doc->tok[arr].size);
    }
    item = PAYLOAD_JSON_Next(doc, item);
  }
  return true;
}

bool PAYLOAD_APP_TlmSessionInfo(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id, const char *session_id)
{
  PAYLOAD_APP_ParseSession(doc, data, session_id, &PAYLOAD_APP_SessionPkt.payload);
  PAYLOAD_APP_SessionPkt.payload.request_id    = request_id;
  PAYLOAD_APP_SessionPkt.payload.session_index = 0;
  PAYLOAD_APP_SessionPkt.payload.session_count = 1;
  PAYLOAD_APP_SessionPkt.payload.source        = PAYLOAD_APP_SESSION_SOURCE_INFO;
  PAYLOAD_APP_Publish(&PAYLOAD_APP_SessionPkt.telemetry_header, false);
  return true;
}

/* ---------------------------------------------------------------- chunks */

bool PAYLOAD_APP_TlmChunks(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id, const char *session_id,
                           uint32 offset)
{
  PAYLOAD_APP_ChunkTlm_Payload_t *c = &PAYLOAD_APP_ChunkPkt.payload;
  char                            encoding[16];
  char                            what[80];
  uint32                          returned;
  size_t                          decoded = 0;
  size_t                          pos     = 0;
  size_t                          n;
  int                             eof = 0;
  int                             idx;

  if (!PAYLOAD_APP_GetString(doc, data, "encoding", encoding, sizeof(encoding)) || strcmp(encoding, "base64") != 0)
    return PAYLOAD_APP_JsonErr("READ_SESSION_RANGE", request_id, "encoding is not base64");

  idx = PAYLOAD_JSON_Find(doc, data, "data");
  if (idx < 0 || doc->tok[idx].type != JSMN_STRING ||
      (size_t)(doc->tok[idx].end - doc->tok[idx].start) > PAYLOAD_APP_READ_B64_MAX)
    return PAYLOAD_APP_JsonErr("READ_SESSION_RANGE", request_id, "data missing, not a string or too long");
  PAYLOAD_JSON_TokString(doc, idx, PAYLOAD_APP_B64Text, sizeof(PAYLOAD_APP_B64Text));

  if (PAYLOAD_Base64Decode(PAYLOAD_APP_B64Text, strlen(PAYLOAD_APP_B64Text), PAYLOAD_APP_ReadBytes,
                           sizeof(PAYLOAD_APP_ReadBytes), &decoded) != PAYLOAD_BASE64_OK)
    return PAYLOAD_APP_JsonErr("READ_SESSION_RANGE", request_id, "invalid Base64 data");

  if (PAYLOAD_APP_GetU32(doc, data, "bytes_returned", &returned) && returned != decoded)
  {
    snprintf(what, sizeof(what), "bytes_returned %lu != %lu decoded bytes", (unsigned long)returned,
             (unsigned long)decoded);
    return PAYLOAD_APP_JsonErr("READ_SESSION_RANGE", request_id, what);
  }
  PAYLOAD_JSON_TokBool(doc, PAYLOAD_JSON_Find(doc, data, "eof"), &eof);

  /* Always at least one packet, so an empty read at end of file still reports eof */
  do
  {
    n = decoded - pos;
    if (n > PAYLOAD_APP_CHUNK_TLM_MAX)
      n = PAYLOAD_APP_CHUNK_TLM_MAX;

    memset(c, 0, sizeof(*c));
    c->request_id = request_id;
    c->n_bytes    = (uint16)n;
    c->offset     = (uint64)offset + pos;
    c->eof        = (uint8)(eof && pos + n >= decoded);
    strncpy(c->session_id, session_id, sizeof(c->session_id) - 1);
    memcpy(c->data, PAYLOAD_APP_ReadBytes + pos, n);
    pos += n;
    PAYLOAD_APP_Publish(&PAYLOAD_APP_ChunkPkt.telemetry_header, pos < decoded);
  } while (pos < decoded);

  if (eof)
    CFE_EVS_SendEvent(PAYLOAD_APP_READ_EOF_EID, CFE_EVS_EventType_INFORMATION,
                      "READ_SESSION_RANGE %s: eof at offset %lu", session_id,
                      (unsigned long)((uint64)offset + decoded));
  return true;
}
