#ifndef PAYLOAD_APP_TLM_H
#define PAYLOAD_APP_TLM_H

#include "cfe.h"
#include "payload_api_json.h"

/*
** Maps Payload API responses to TLM packets and publishes them (called from the
** worker task). Each function returns false after its own JSON_ERR event when the
** response data cannot be used.
*/

void PAYLOAD_APP_TlmInit(void);

/* GET_LIVE_STATE: one DATA packet per aircraft (lowest age_s first, capped); updates HK live_aircraft_count */
bool PAYLOAD_APP_TlmLive(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id);

/* LIST_SESSIONS: one SESSION packet per session, LIST_EMPTY event when there are none */
bool PAYLOAD_APP_TlmSessionList(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id);

/* GET_SESSION_INFO: one SESSION packet; missing fields are flagged, not fatal */
bool PAYLOAD_APP_TlmSessionInfo(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id, const char *session_id);

/* READ_SESSION_RANGE: decodes the Base64 data and publishes it as CHUNK packets */
bool PAYLOAD_APP_TlmChunks(const PAYLOAD_JSON_Doc_t *doc, int data, uint32 request_id, const char *session_id,
                           uint32 offset);

#endif
