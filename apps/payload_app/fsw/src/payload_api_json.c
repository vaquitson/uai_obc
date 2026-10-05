#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAYLOAD_API_JSON_IMPL
#include "payload_api_json.h"

/* Longest numeric literal accepted (digits, sign, fraction, exponent) */
#define PAYLOAD_JSON_NUM_MAX 40

PAYLOAD_JSON_Result_t PAYLOAD_JSON_Parse(PAYLOAD_JSON_Doc_t *doc, const char *js, size_t len, jsmntok_t *tokens,
                                         unsigned int max_tokens)
{
  jsmn_parser parser;
  int         rc;

  doc->js    = js;
  doc->tok   = tokens;
  doc->count = 0;

  jsmn_init(&parser);
  rc = jsmn_parse(&parser, js, len, tokens, max_tokens);
  if (rc == JSMN_ERROR_NOMEM)
    return PAYLOAD_JSON_ERR_NOMEM;
  if (rc < 0)
    return PAYLOAD_JSON_ERR_INVAL;
  if (rc == 0 || tokens[0].type != JSMN_OBJECT)
    return PAYLOAD_JSON_ERR_NOT_OBJECT;
  /* Trailing data after the root object is not part of a valid document */
  if (PAYLOAD_JSON_Next(&(PAYLOAD_JSON_Doc_t){js, tokens, rc}, 0) != rc)
    return PAYLOAD_JSON_ERR_INVAL;

  doc->count = rc;
  return PAYLOAD_JSON_OK;
}

int PAYLOAD_JSON_Next(const PAYLOAD_JSON_Doc_t *doc, int idx)
{
  int end;
  int j;

  if (idx < 0 || idx >= doc->count)
    return doc->count;

  end = doc->tok[idx].end;
  for (j = idx + 1; j < doc->count && doc->tok[j].start < end; j++)
    ;
  return j;
}

static int PAYLOAD_JSON_TokLen(const PAYLOAD_JSON_Doc_t *doc, int idx)
{
  return doc->tok[idx].end - doc->tok[idx].start;
}

static int PAYLOAD_JSON_TokEquals(const PAYLOAD_JSON_Doc_t *doc, int idx, const char *text)
{
  size_t len = strlen(text);

  return (size_t)PAYLOAD_JSON_TokLen(doc, idx) == len && memcmp(doc->js + doc->tok[idx].start, text, len) == 0;
}

static int PAYLOAD_JSON_IsNull(const PAYLOAD_JSON_Doc_t *doc, int idx)
{
  return doc->tok[idx].type == JSMN_PRIMITIVE && PAYLOAD_JSON_TokEquals(doc, idx, "null");
}

int PAYLOAD_JSON_Find(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key)
{
  int i;
  int n;
  int pairs;

  if (obj < 0 || obj >= doc->count || doc->tok[obj].type != JSMN_OBJECT)
    return -1;

  pairs = doc->tok[obj].size;
  i     = obj + 1;
  for (n = 0; n < pairs && i + 1 < doc->count; n++)
  {
    if (doc->tok[i].type == JSMN_STRING && PAYLOAD_JSON_TokEquals(doc, i, key))
      return i + 1;
    i = PAYLOAD_JSON_Next(doc, i + 1);
  }
  return -1;
}

static PAYLOAD_JSON_Field_t PAYLOAD_JSON_Classify(const PAYLOAD_JSON_Doc_t *doc, int idx)
{
  if (idx < 0 || idx >= doc->count)
    return PAYLOAD_JSON_FIELD_MISSING;
  if (PAYLOAD_JSON_IsNull(doc, idx))
    return PAYLOAD_JSON_FIELD_NULL;
  return PAYLOAD_JSON_FIELD_VALID;
}

static int PAYLOAD_JSON_HexDigit(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokString(const PAYLOAD_JSON_Doc_t *doc, int idx, char *out, size_t out_size)
{
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_Classify(doc, idx);
  const char          *p;
  const char          *end;
  size_t               n = 0;
  char                 c;
  int                  cp;
  int                  k;

  if (out_size > 0)
    out[0] = '\0';
  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  if (doc->tok[idx].type != JSMN_STRING)
    return PAYLOAD_JSON_FIELD_BADTYPE;
  if (out_size == 0)
    return PAYLOAD_JSON_FIELD_VALID;

  p   = doc->js + doc->tok[idx].start;
  end = doc->js + doc->tok[idx].end;
  while (p < end && n + 1 < out_size)
  {
    c = *p++;
    if (c == '\\' && p < end)
    {
      c = *p++;
      switch (c)
      {
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'u':
          cp = 0;
          for (k = 0; k < 4 && p < end; k++)
            cp = cp * 16 + PAYLOAD_JSON_HexDigit(*p++);
          c = (cp >= 0x20 && cp < 0x7F) ? (char)cp : '?'; /* non-ASCII is not needed by any field */
          break;
        default:
          break; /* \" \\ \/ map to themselves */
      }
    }
    out[n++] = c;
  }
  out[n] = '\0';
  return PAYLOAD_JSON_FIELD_VALID;
}

/* Copies a numeric primitive (up to PAYLOAD_JSON_NUM_MAX - 1 chars) into buf */
static PAYLOAD_JSON_Field_t PAYLOAD_JSON_NumText(const PAYLOAD_JSON_Doc_t *doc, int idx, char *buf)
{
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_Classify(doc, idx);
  int                  len;

  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  len = PAYLOAD_JSON_TokLen(doc, idx);
  if (doc->tok[idx].type != JSMN_PRIMITIVE || len <= 0 || len >= PAYLOAD_JSON_NUM_MAX)
    return PAYLOAD_JSON_FIELD_BADTYPE;
  memcpy(buf, doc->js + doc->tok[idx].start, (size_t)len);
  buf[len] = '\0';
  if (buf[0] != '-' && (buf[0] < '0' || buf[0] > '9'))
    return PAYLOAD_JSON_FIELD_BADTYPE;
  return PAYLOAD_JSON_FIELD_VALID;
}

PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokI64(const PAYLOAD_JSON_Doc_t *doc, int idx, int64_t *out)
{
  char                 buf[PAYLOAD_JSON_NUM_MAX];
  char                *end = NULL;
  long long            v;
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_NumText(doc, idx, buf);

  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  if (strpbrk(buf, ".eE") != NULL)
    return PAYLOAD_JSON_FIELD_BADTYPE;
  errno = 0;
  v     = strtoll(buf, &end, 10);
  if (errno != 0 || end == buf || *end != '\0')
    return PAYLOAD_JSON_FIELD_BADTYPE;
  *out = (int64_t)v;
  return PAYLOAD_JSON_FIELD_VALID;
}

PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokU64(const PAYLOAD_JSON_Doc_t *doc, int idx, uint64_t *out)
{
  char                 buf[PAYLOAD_JSON_NUM_MAX];
  char                *end = NULL;
  unsigned long long   v;
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_NumText(doc, idx, buf);

  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  if (buf[0] == '-' || strpbrk(buf, ".eE") != NULL)
    return PAYLOAD_JSON_FIELD_BADTYPE;
  errno = 0;
  v     = strtoull(buf, &end, 10);
  if (errno != 0 || end == buf || *end != '\0')
    return PAYLOAD_JSON_FIELD_BADTYPE;
  *out = (uint64_t)v;
  return PAYLOAD_JSON_FIELD_VALID;
}

PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokDouble(const PAYLOAD_JSON_Doc_t *doc, int idx, double *out)
{
  char                 buf[PAYLOAD_JSON_NUM_MAX];
  char                *end = NULL;
  double               v;
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_NumText(doc, idx, buf);

  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  errno = 0;
  v     = strtod(buf, &end);
  if (errno != 0 || end == buf || *end != '\0')
    return PAYLOAD_JSON_FIELD_BADTYPE;
  *out = v;
  return PAYLOAD_JSON_FIELD_VALID;
}

PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokBool(const PAYLOAD_JSON_Doc_t *doc, int idx, int *out)
{
  PAYLOAD_JSON_Field_t field = PAYLOAD_JSON_Classify(doc, idx);

  if (field != PAYLOAD_JSON_FIELD_VALID)
    return field;
  if (doc->tok[idx].type == JSMN_PRIMITIVE && PAYLOAD_JSON_TokEquals(doc, idx, "true"))
    *out = 1;
  else if (doc->tok[idx].type == JSMN_PRIMITIVE && PAYLOAD_JSON_TokEquals(doc, idx, "false"))
    *out = 0;
  else
    return PAYLOAD_JSON_FIELD_BADTYPE;
  return PAYLOAD_JSON_FIELD_VALID;
}

static PAYLOAD_JSON_Result_t PAYLOAD_JSON_Fail(char *why, size_t why_size, const char *text)
{
  if (why != NULL && why_size > 0)
    snprintf(why, why_size, "%s", text);
  return PAYLOAD_JSON_ERR_FIELD;
}

PAYLOAD_JSON_Result_t PAYLOAD_JSON_ParseEnvelope(const PAYLOAD_JSON_Doc_t *doc, PAYLOAD_JSON_Envelope_t *env,
                                                 char *why, size_t why_size)
{
  uint64_t rid;
  int64_t  code;
  char     status[8];
  int      data;

  memset(env, 0, sizeof(*env));
  env->data = -1;
  if (why != NULL && why_size > 0)
    why[0] = '\0';

  if (PAYLOAD_JSON_TokU64(doc, PAYLOAD_JSON_Find(doc, 0, "request_id"), &rid) != PAYLOAD_JSON_FIELD_VALID ||
      rid > 0xFFFFFFFFu)
    return PAYLOAD_JSON_Fail(why, why_size, "request_id missing or not a uint32");
  env->request_id = (uint32_t)rid;

  if (PAYLOAD_JSON_TokString(doc, PAYLOAD_JSON_Find(doc, 0, "status"), status, sizeof(status)) !=
      PAYLOAD_JSON_FIELD_VALID)
    return PAYLOAD_JSON_Fail(why, why_size, "status missing or not a string");
  if (strcmp(status, "OK") == 0)
    env->ok = 1;
  else if (strcmp(status, "ERROR") != 0)
    return PAYLOAD_JSON_Fail(why, why_size, "status is neither OK nor ERROR");

  if (PAYLOAD_JSON_TokI64(doc, PAYLOAD_JSON_Find(doc, 0, "status_code"), &code) != PAYLOAD_JSON_FIELD_VALID ||
      code < 0 || code > PAYLOAD_JSON_STATUS_CODE_MAX)
    return PAYLOAD_JSON_Fail(why, why_size, "status_code missing or outside 0..8");
  env->status_code = (uint8_t)code;
  if (env->ok != (code == PAYLOAD_API_STATUS_OK))
    return PAYLOAD_JSON_Fail(why, why_size, "status and status_code disagree");

  data = PAYLOAD_JSON_Find(doc, 0, "data");
  if (data >= 0 && !PAYLOAD_JSON_IsNull(doc, data))
  {
    if (doc->tok[data].type != JSMN_OBJECT)
      return PAYLOAD_JSON_Fail(why, why_size, "data is not an object");
    env->data = data;
    PAYLOAD_JSON_TokString(doc, PAYLOAD_JSON_Find(doc, data, "message"), env->message, sizeof(env->message));
  }
  if (env->ok && env->data < 0)
    return PAYLOAD_JSON_Fail(why, why_size, "data missing in OK response");

  return PAYLOAD_JSON_OK;
}

static int PAYLOAD_JSON_GetEnum(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key, const char *zero,
                                const char *one, uint8_t *out)
{
  char text[16];

  if (PAYLOAD_JSON_TokString(doc, PAYLOAD_JSON_Find(doc, obj, key), text, sizeof(text)) != PAYLOAD_JSON_FIELD_VALID)
    return 0;
  if (strcmp(text, zero) == 0)
    *out = 0;
  else if (strcmp(text, one) == 0)
    *out = 1;
  else
    return 0;
  return 1;
}

void PAYLOAD_JSON_ParseStatus(const PAYLOAD_JSON_Doc_t *doc, int obj, PAYLOAD_JSON_Status_t *status)
{
  uint64_t             u;
  int                  b;
  int                  idx;
  PAYLOAD_JSON_Field_t field;

  memset(status, 0, sizeof(*status));
  if (obj < 0 || obj >= doc->count || doc->tok[obj].type != JSMN_OBJECT)
    return;

  if (PAYLOAD_JSON_TokString(doc, PAYLOAD_JSON_Find(doc, obj, "interface_version"), status->interface_version,
                             sizeof(status->interface_version)) == PAYLOAD_JSON_FIELD_VALID)
    status->valid |= PAYLOAD_JSON_STATUS_VERSION;
  if (PAYLOAD_JSON_GetEnum(doc, obj, "payload_state", "READY", "ACQUIRING", &status->payload_state))
    status->valid |= PAYLOAD_JSON_STATUS_PAYLOAD_STATE;
  if (PAYLOAD_JSON_GetEnum(doc, obj, "receiver_state", "INACTIVE", "ACTIVE", &status->receiver_state))
    status->valid |= PAYLOAD_JSON_STATUS_RECEIVER_STATE;
  if (PAYLOAD_JSON_GetEnum(doc, obj, "logger_state", "INACTIVE", "ACTIVE", &status->logger_state))
    status->valid |= PAYLOAD_JSON_STATUS_LOGGER_STATE;

  idx   = PAYLOAD_JSON_Find(doc, obj, "active_session_id");
  field = PAYLOAD_JSON_TokString(doc, idx, status->active_session_id, sizeof(status->active_session_id));
  if (field == PAYLOAD_JSON_FIELD_VALID || field == PAYLOAD_JSON_FIELD_NULL)
    status->valid |= PAYLOAD_JSON_STATUS_ACTIVE_SESSION;

  if (PAYLOAD_JSON_TokU64(doc, PAYLOAD_JSON_Find(doc, obj, "storage_free_bytes"), &u) == PAYLOAD_JSON_FIELD_VALID)
  {
    status->storage_free_bytes = u;
    status->valid |= PAYLOAD_JSON_STATUS_STORAGE_FREE;
  }
  if (PAYLOAD_JSON_TokBool(doc, PAYLOAD_JSON_Find(doc, obj, "live_consumer_connected"), &b) ==
      PAYLOAD_JSON_FIELD_VALID)
  {
    status->live_consumer_connected = (uint8_t)b;
    status->valid |= PAYLOAD_JSON_STATUS_LIVE_CONSUMER;
  }
  if (PAYLOAD_JSON_TokU64(doc, PAYLOAD_JSON_Find(doc, obj, "live_aircraft_count"), &u) == PAYLOAD_JSON_FIELD_VALID)
  {
    status->live_aircraft_count = (uint16_t)(u > 0xFFFFu ? 0xFFFFu : u);
    status->valid |= PAYLOAD_JSON_STATUS_LIVE_COUNT;
  }
}

int PAYLOAD_JSON_BuildRequest(char *buf, size_t buf_size, uint32_t request_id, const char *op, const char *args_json)
{
  int n;

  if (buf == NULL || op == NULL || args_json == NULL)
    return -1;
  n = snprintf(buf, buf_size, "{\"request_id\":%lu,\"op\":\"%s\",\"args\":%s}", (unsigned long)request_id, op,
               args_json);
  if (n <= 0 || (size_t)n >= buf_size)
    return -1;
  return n;
}

const char *PAYLOAD_JSON_StatusCodeName(uint32_t status_code)
{
  static const char *const names[] = {"OK",
                                      "INVALID_REQUEST",
                                      "INVALID_ARGUMENT",
                                      "NOT_FOUND",
                                      "BUSY",
                                      "RECEIVER_UNAVAILABLE",
                                      "LOGGER_UNAVAILABLE",
                                      "STORAGE_ERROR",
                                      "INTERNAL_ERROR"};

  if (status_code <= PAYLOAD_JSON_STATUS_CODE_MAX)
    return names[status_code];
  return "UNKNOWN";
}

static int PAYLOAD_JSON_Digits(const char *s, int count, int *out)
{
  int i;
  int v = 0;

  for (i = 0; i < count; i++)
  {
    if (s[i] < '0' || s[i] > '9')
      return 0;
    v = v * 10 + (s[i] - '0');
  }
  *out = v;
  return 1;
}

int PAYLOAD_JSON_IsValidSessionId(const char *s)
{
  int dummy;

  return s != NULL && strlen(s) == 16 && PAYLOAD_JSON_Digits(s, 8, &dummy) && s[8] == 'T' &&
         PAYLOAD_JSON_Digits(s + 9, 6, &dummy) && s[15] == 'Z';
}

/* Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's days_from_civil) */
static long PAYLOAD_JSON_DaysFromCivil(int y, int m, int d)
{
  long era;
  long yoe;
  long doy;
  long doe;

  y -= m <= 2;
  era = (y >= 0 ? y : y - 399) / 400;
  yoe = y - era * 400;
  doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

int PAYLOAD_JSON_IsoToEpoch(const char *s, uint32_t *out)
{
  static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int              y, mo, d, h, mi, sec, dim;
  const char      *p;
  long long        t;

  if (s == NULL || strlen(s) < 20)
    return 0;
  if (!PAYLOAD_JSON_Digits(s, 4, &y) || s[4] != '-' || !PAYLOAD_JSON_Digits(s + 5, 2, &mo) || s[7] != '-' ||
      !PAYLOAD_JSON_Digits(s + 8, 2, &d) || s[10] != 'T' || !PAYLOAD_JSON_Digits(s + 11, 2, &h) || s[13] != ':' ||
      !PAYLOAD_JSON_Digits(s + 14, 2, &mi) || s[16] != ':' || !PAYLOAD_JSON_Digits(s + 17, 2, &sec))
    return 0;

  p = s + 19;
  if (*p == '.')
  {
    p++;
    if (*p < '0' || *p > '9')
      return 0;
    while (*p >= '0' && *p <= '9')
      p++;
  }
  if (p[0] != 'Z' || p[1] != '\0')
    return 0;

  if (mo < 1 || mo > 12 || h > 23 || mi > 59 || sec > 60 || y < 1970)
    return 0;
  dim = mdays[mo - 1] + (mo == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0));
  if (d < 1 || d > dim)
    return 0;

  t = (long long)PAYLOAD_JSON_DaysFromCivil(y, mo, d) * 86400LL + h * 3600LL + mi * 60LL + sec;
  if (t < 0 || t > 0xFFFFFFFFLL)
    return 0;
  *out = (uint32_t)t;
  return 1;
}
