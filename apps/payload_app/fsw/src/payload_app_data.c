/*
** Minimal, bounded-scope JSON field extraction for the Payload response
** envelope: {"request_id": N, "status": "OK"|"ERROR", "status_code": N,
** "data": {...}}. There is no JSON library anywhere in this repo, so rather
** than vendor one for a single flat envelope this hand-rolls just enough:
** find a key by name within a given substring, read the scalar that follows
** it. It does not handle arbitrary nesting, arrays, or escaped strings -
** if Payload's "data" schema grows beyond flat scalars this should be
** replaced with a real parser (e.g. cJSON).
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_msg.h"

#include "payload_app_eventids.h"
#include "payload_app_data.h"
#include "payload_app.h"

static const char *PAYLOAD_APP_JsonFindKey(const char *buf, size_t buf_len, const char *key)
{
  char   pattern[64];
  int    pattern_len_i;
  size_t pattern_len;
  size_t i;

  pattern_len_i = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  if (pattern_len_i <= 0)
    return NULL;

  pattern_len = (size_t)pattern_len_i;
  if (pattern_len >= buf_len)
    return NULL;

  for (i = 0; i + pattern_len <= buf_len; i++)
  {
    if (memcmp(buf + i, pattern, pattern_len) == 0)
      return buf + i + pattern_len;
  }

  return NULL;
}

/* Skips whitespace and the ':' following a JSON key, landing on the first
** character of the value. Returns NULL if malformed (no ':' found). */
static const char *PAYLOAD_APP_JsonSkipToValue(const char *p, const char *buf_end)
{
  while (p < buf_end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;

  if (p >= buf_end || *p != ':')
    return NULL;
  p++;

  while (p < buf_end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;

  return (p < buf_end) ? p : NULL;
}

static bool PAYLOAD_APP_JsonGetValuePtr(const char *buf, size_t buf_len, const char *key, const char **out_val,
                                         const char **out_end)
{
  const char *key_end = PAYLOAD_APP_JsonFindKey(buf, buf_len, key);
  const char *buf_end = buf + buf_len;
  const char *value;

  if (key_end == NULL)
    return false;

  value = PAYLOAD_APP_JsonSkipToValue(key_end, buf_end);
  if (value == NULL)
    return false;

  *out_val = value;
  *out_end = buf_end;
  return true;
}

/* Finds "key": { ... } within buf[0..buf_len) and returns a pointer to the
** opening '{', with *out_len spanning through the matching closing '}'. */
static const char *PAYLOAD_APP_JsonFindObject(const char *buf, size_t buf_len, const char *key, size_t *out_len)
{
  const char *key_end = PAYLOAD_APP_JsonFindKey(buf, buf_len, key);
  const char *buf_end = buf + buf_len;
  const char *value;
  const char *p;
  int         depth     = 0;
  bool        in_string = false;

  *out_len = 0;

  if (key_end == NULL)
    return NULL;

  value = PAYLOAD_APP_JsonSkipToValue(key_end, buf_end);
  if (value == NULL || *value != '{')
    return NULL;

  for (p = value; p < buf_end; p++)
  {
    if (in_string)
    {
      if (*p == '\\')
        p++;
      else if (*p == '"')
        in_string = false;
      continue;
    }

    if (*p == '"')
      in_string = true;
    else if (*p == '{')
      depth++;
    else if (*p == '}')
    {
      depth--;
      if (depth == 0)
      {
        *out_len = (size_t)(p - value) + 1;
        return value;
      }
    }
  }

  return NULL; /* unbalanced braces */
}

static bool PAYLOAD_APP_JsonGetInt(const char *buf, size_t buf_len, const char *key, int32 *out_val)
{
  const char *value;
  const char *buf_end;
  char       *end = NULL;
  long        parsed;

  if (!PAYLOAD_APP_JsonGetValuePtr(buf, buf_len, key, &value, &buf_end))
    return false;

  parsed = strtol(value, &end, 10);
  if (end == value)
    return false;

  *out_val = (int32)parsed;
  return true;
}

static bool PAYLOAD_APP_JsonGetDouble(const char *buf, size_t buf_len, const char *key, double *out_val)
{
  const char *value;
  const char *buf_end;
  char       *end = NULL;
  double      parsed;

  if (!PAYLOAD_APP_JsonGetValuePtr(buf, buf_len, key, &value, &buf_end))
    return false;

  parsed = strtod(value, &end);
  if (end == value)
    return false;

  *out_val = parsed;
  return true;
}

static bool PAYLOAD_APP_JsonGetString(const char *buf, size_t buf_len, const char *key, char *out_str,
                                       size_t out_str_size)
{
  const char *value;
  const char *buf_end;
  const char *p;
  size_t      len = 0;

  if (!PAYLOAD_APP_JsonGetValuePtr(buf, buf_len, key, &value, &buf_end))
    return false;

  if (value >= buf_end || *value != '"')
    return false;

  for (p = value + 1; p < buf_end && *p != '"' && len + 1 < out_str_size; p++)
    out_str[len++] = *p; /* no escape-sequence decoding - values are plain identifiers/callsigns */

  out_str[len] = '\0';
  return true;
}

void PAYLOAD_APP_PrintDataMsg(const PAYLOAD_APP_DataMsg_t *msg)
{
  printf("PAYLOAD_APP: data_tlm request_id=%d status=%s status_code=%u\n", (int)msg->request_id, msg->status,
         (unsigned int)msg->status_code);
  printf("  icao24=%s callsign=%s\n", msg->icao24, msg->callsign);
  printf("  latitude=%.6f longitude=%.6f altitude=%.2f\n", msg->latitude, msg->longitude, (double)msg->altitude);
  printf("  ground_speed=%.2f heading=%.2f vertical_rate=%.2f timestamp=%.3f\n", (double)msg->ground_speed,
         (double)msg->heading, (double)msg->vertical_rate, msg->timestamp);
}

void PAYLOAD_APP_ParseAndPublish(const char *json, size_t len)
{
  PAYLOAD_APP_DataMsg_t *msg = &PAYLOAD_APP_Global.data_msg;
  const char             *data_obj;
  size_t                  data_len = 0;
  int32                   request_id  = 0;
  int32                   status_code = 0;
  char                    status[PAYLOAD_APP_STATUS_LEN];
  double                  tmp_d;
  int32                   tlm_status;

  if (!PAYLOAD_APP_JsonGetInt(json, len, "request_id", &request_id))
    request_id = 0;

  if (!PAYLOAD_APP_JsonGetInt(json, len, "status_code", &status_code))
    status_code = 0;

  if (!PAYLOAD_APP_JsonGetString(json, len, "status", status, sizeof(status)))
    strncpy(status, "ERROR", sizeof(status) - 1);
  status[sizeof(status) - 1] = '\0';

  msg->request_id  = request_id;
  msg->status_code = (uint8)status_code;
  strncpy(msg->status, status, sizeof(msg->status) - 1);
  msg->status[sizeof(msg->status) - 1] = '\0';

  msg->icao24[0]   = '\0';
  msg->callsign[0] = '\0';
  msg->latitude = msg->longitude = 0.0;
  msg->altitude = msg->ground_speed = msg->heading = msg->vertical_rate = 0.0f;
  msg->timestamp = 0.0;

  data_obj = PAYLOAD_APP_JsonFindObject(json, len, "data", &data_len);
  if (data_obj != NULL)
  {
    PAYLOAD_APP_JsonGetString(data_obj, data_len, "icao24", msg->icao24, sizeof(msg->icao24));
    PAYLOAD_APP_JsonGetString(data_obj, data_len, "callsign", msg->callsign, sizeof(msg->callsign));

    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "latitude", &tmp_d))
      msg->latitude = tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "longitude", &tmp_d))
      msg->longitude = tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "altitude", &tmp_d))
      msg->altitude = (float)tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "ground_speed", &tmp_d))
      msg->ground_speed = (float)tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "heading", &tmp_d))
      msg->heading = (float)tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "vertical_rate", &tmp_d))
      msg->vertical_rate = (float)tmp_d;
    if (PAYLOAD_APP_JsonGetDouble(data_obj, data_len, "timestamp", &tmp_d))
      msg->timestamp = tmp_d;
  }
  else
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_PARSE_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: no 'data' object found in Payload message");
    PAYLOAD_APP_Global.hk_packet.parse_errors++;
  }

  tlm_status = CFE_SB_TransmitMsg(CFE_MSG_PTR(msg->telemetry_header), true);
  if (tlm_status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_DATA_TLM_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: data TLM transmit error, RC = 0x%08lX", (unsigned long)tlm_status);

  PAYLOAD_APP_Global.hk_packet.msgs_received++;
  PAYLOAD_APP_Global.hk_packet.last_request_id   = request_id;
  PAYLOAD_APP_Global.hk_packet.last_status_code  = (uint8)status_code;

  PAYLOAD_APP_PrintDataMsg(msg);
}
