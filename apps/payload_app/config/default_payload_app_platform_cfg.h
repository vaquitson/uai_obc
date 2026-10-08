/**
 * @file
 *   PAYLOAD_APP Application Platform Configuration
 */
#ifndef DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H
#define DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H

/** Depth of the command pipe */
#define PAYLOAD_APP_PIPE_DEPTH 16

/** Housekeeping period (ms); the main loop never blocks longer than this */
#define PAYLOAD_APP_HK_PERIOD_MS 1000

/** Absolute path of the Payload API Unix socket (Raspberry Pi deployment) */
#define PAYLOAD_APP_SOCKET_PATH_DEFAULT "/run/payload/payload.sock"

/** Environment variable that overrides the socket path at runtime (development) */
#define PAYLOAD_APP_SOCKET_PATH_ENV "PAYLOAD_APP_SOCKET_PATH"

/** interface_version the app was written against */
#define PAYLOAD_APP_INTERFACE_VERSION "0.4.0"

/*
** Request queue between the main task and the socket worker child task.
** Without root, POSIX message queues are capped by /proc/sys/fs/mqueue/msg_max (10).
*/
#define PAYLOAD_APP_REQ_QUEUE_DEPTH 4

/** Per-request total deadline (ms) for queries */
#define PAYLOAD_APP_QUERY_TIMEOUT_MS 5000

/** Per-request total deadline (ms) for START_ACQUISITION/STOP_ACQUISITION (systemd units) */
#define PAYLOAD_APP_ACQ_TIMEOUT_MS 40000

/** Response buffer: READ_SESSION_RANGE (4096 B as Base64) and LIST_SESSIONS with many sessions */
#define PAYLOAD_APP_RX_BUF_SIZE 65536

/** Max JSON tokens per response (jsmn) */
#define PAYLOAD_APP_JSON_MAX_TOKENS 4096

/** BUSY (status_code 4) handling: total attempts and initial backoff, doubled on each retry */
#define PAYLOAD_APP_BUSY_MAX_ATTEMPTS 3
#define PAYLOAD_APP_BUSY_BACKOFF_MS   1000

/** connect() failure backoff: exponential between these bounds (ms) */
#define PAYLOAD_APP_CONNECT_BACKOFF_MIN_MS 1000
#define PAYLOAD_APP_CONNECT_BACKOFF_MAX_MS 30000

/** A queued request is discarded after this long without being able to connect (startup GET_STATUS never is) */
#define PAYLOAD_APP_CONNECT_RETRY_MAX_MS 30000

/** Max raw bytes per READ_SESSION_RANGE chunk TLM packet (multiple of 8; 16+40+176 = 232 B fits LoRa) */
#define PAYLOAD_APP_CHUNK_TLM_MAX 176

/** Max aircraft packets published per GET_LIVE_STATE (lowest age_s first) */
#define PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ 20

/** Delay (ms) between packets of a TLM burst so to_lab's pipe is not overrun */
#define PAYLOAD_APP_TLM_BURST_DELAY_MS 10

/** Socket worker child task */
#define PAYLOAD_APP_WORKER_PRIORITY 95
#define PAYLOAD_APP_WORKER_STACK    16384

#endif
