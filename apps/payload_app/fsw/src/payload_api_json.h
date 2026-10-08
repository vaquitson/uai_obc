#ifndef PAYLOAD_API_JSON_H
#define PAYLOAD_API_JSON_H

/*
** Payload API JSON helpers on top of jsmn, independent of cFE (no malloc).
** Fields are looked up by path, unknown keys are ignored, and every getter
** reports whether the value was valid, null, missing or of the wrong type.
*/

#include <stddef.h>
#include <stdint.h>

/* jsmn is vendored once (payload_api_json.c); keep its public symbols out of the global namespace */
#define jsmn_init  PAYLOAD_JSMN_Init
#define jsmn_parse PAYLOAD_JSMN_Parse
#define JSMN_STRICT
#ifndef PAYLOAD_API_JSON_IMPL
#define JSMN_HEADER
#endif
#include "jsmn.h"

/** Max length (incl. NUL) kept from data.message */
#define PAYLOAD_JSON_MESSAGE_MAX 128

/** Max length (incl. NUL) of interface_version kept for comparison */
#define PAYLOAD_JSON_VERSION_MAX 16

/** Length of a session_id "YYYYMMDDTHHMMSSZ" plus NUL */
#define PAYLOAD_JSON_SESSION_ID_LEN 17

/** Highest status_code defined by the API */
#define PAYLOAD_JSON_STATUS_CODE_MAX 8

typedef enum {
  PAYLOAD_JSON_OK = 0,
  PAYLOAD_JSON_ERR_NOMEM,      /* more tokens than the token buffer holds */
  PAYLOAD_JSON_ERR_INVAL,      /* not valid JSON */
  PAYLOAD_JSON_ERR_NOT_OBJECT, /* root is not an object */
  PAYLOAD_JSON_ERR_FIELD       /* envelope field missing, null or of the wrong type */
} PAYLOAD_JSON_Result_t;

typedef enum {
  PAYLOAD_JSON_FIELD_VALID = 0,
  PAYLOAD_JSON_FIELD_NULL,
  PAYLOAD_JSON_FIELD_MISSING,
  PAYLOAD_JSON_FIELD_BADTYPE
} PAYLOAD_JSON_Field_t;

/* status_code values */
typedef enum {
  PAYLOAD_API_STATUS_OK                   = 0,
  PAYLOAD_API_STATUS_INVALID_REQUEST      = 1,
  PAYLOAD_API_STATUS_INVALID_ARGUMENT     = 2,
  PAYLOAD_API_STATUS_NOT_FOUND            = 3,
  PAYLOAD_API_STATUS_BUSY                 = 4,
  PAYLOAD_API_STATUS_RECEIVER_UNAVAILABLE = 5,
  PAYLOAD_API_STATUS_LOGGER_UNAVAILABLE   = 6,
  PAYLOAD_API_STATUS_STORAGE_ERROR        = 7,
  PAYLOAD_API_STATUS_INTERNAL_ERROR       = 8
} PAYLOAD_API_StatusCode_t;

typedef struct {
  const char      *js;
  const jsmntok_t *tok;
  int              count;
} PAYLOAD_JSON_Doc_t;

typedef struct {
  uint32_t request_id;
  int      ok;          /* envelope "status" == "OK" */
  uint8_t  status_code;
  int      data;        /* token index of the "data" object, -1 if absent or null */
  char     message[PAYLOAD_JSON_MESSAGE_MAX]; /* data.message when it is a string, else "" */
} PAYLOAD_JSON_Envelope_t;

/* PAYLOAD_JSON_Status_t.valid bits */
#define PAYLOAD_JSON_STATUS_VERSION        0x01u
#define PAYLOAD_JSON_STATUS_PAYLOAD_STATE  0x02u
#define PAYLOAD_JSON_STATUS_RECEIVER_STATE 0x04u
#define PAYLOAD_JSON_STATUS_LOGGER_STATE   0x08u
#define PAYLOAD_JSON_STATUS_ACTIVE_SESSION 0x10u /* set for a string or an explicit null */
#define PAYLOAD_JSON_STATUS_STORAGE_FREE   0x20u
#define PAYLOAD_JSON_STATUS_LIVE_CONSUMER  0x40u
#define PAYLOAD_JSON_STATUS_LIVE_COUNT     0x80u

/* GET_STATUS data / START+STOP data.status */
typedef struct {
  uint32_t valid;
  char     interface_version[PAYLOAD_JSON_VERSION_MAX];
  uint8_t  payload_state;  /* 0 READY, 1 ACQUIRING */
  uint8_t  receiver_state; /* 0 INACTIVE, 1 ACTIVE */
  uint8_t  logger_state;   /* 0 INACTIVE, 1 ACTIVE */
  char     active_session_id[PAYLOAD_JSON_SESSION_ID_LEN]; /* "" when null */
  uint64_t storage_free_bytes;
  uint8_t  live_consumer_connected;
  uint16_t live_aircraft_count; /* clamped to 65535 */
} PAYLOAD_JSON_Status_t;

/** Tokenizes js (len bytes); the root must be an object */
PAYLOAD_JSON_Result_t PAYLOAD_JSON_Parse(PAYLOAD_JSON_Doc_t *doc, const char *js, size_t len, jsmntok_t *tokens,
                                         unsigned int max_tokens);

/** Token index of the value for key in object obj, -1 if obj is not an object or key is absent */
int PAYLOAD_JSON_Find(const PAYLOAD_JSON_Doc_t *doc, int obj, const char *key);

/** Index of the token following the whole subtree rooted at idx */
int PAYLOAD_JSON_Next(const PAYLOAD_JSON_Doc_t *doc, int idx);

/* Typed getters on a token index (idx < 0 means missing). Strings are unescaped and always NUL-terminated
** (truncated to out_size - 1); integers reject fractions/exponents; U64 rejects negatives. */
PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokString(const PAYLOAD_JSON_Doc_t *doc, int idx, char *out, size_t out_size);
PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokI64(const PAYLOAD_JSON_Doc_t *doc, int idx, int64_t *out);
PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokU64(const PAYLOAD_JSON_Doc_t *doc, int idx, uint64_t *out);
PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokDouble(const PAYLOAD_JSON_Doc_t *doc, int idx, double *out);
PAYLOAD_JSON_Field_t PAYLOAD_JSON_TokBool(const PAYLOAD_JSON_Doc_t *doc, int idx, int *out);

/** Validates the response envelope; on failure why (may be NULL) describes the problem */
PAYLOAD_JSON_Result_t PAYLOAD_JSON_ParseEnvelope(const PAYLOAD_JSON_Doc_t *doc, PAYLOAD_JSON_Envelope_t *env,
                                                 char *why, size_t why_size);

/** Parses a status object (GET_STATUS data or START/STOP data.status); missing/null/invalid fields stay unset */
void PAYLOAD_JSON_ParseStatus(const PAYLOAD_JSON_Doc_t *doc, int obj, PAYLOAD_JSON_Status_t *status);

/** Builds {"request_id":N,"op":"OP","args":ARGS}; args_json must already be a JSON object. Returns length or -1 */
int PAYLOAD_JSON_BuildRequest(char *buf, size_t buf_size, uint32_t request_id, const char *op, const char *args_json);

/** Constant name of a status_code ("BUSY", ...), "UNKNOWN" outside 0..8 */
const char *PAYLOAD_JSON_StatusCodeName(uint32_t status_code);

/** 1 if s has the exact form YYYYMMDDTHHMMSSZ */
int PAYLOAD_JSON_IsValidSessionId(const char *s);

/** "YYYY-MM-DDTHH:MM:SS[.frac]Z" -> seconds since 1970 (fraction dropped). Returns 1 on success */
int PAYLOAD_JSON_IsoToEpoch(const char *s, uint32_t *out);

#endif
