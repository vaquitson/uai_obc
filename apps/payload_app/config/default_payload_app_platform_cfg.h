/**
 * @file
 *   PAYLOAD_APP Application Platform Configuration
 */
#ifndef DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H
#define DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H

/** Depth of the command pipe */
#define PAYLOAD_APP_PIPE_DEPTH 16

/** Timeout (ms) used while waiting for a command on the pipe, between HK sends */
#define PAYLOAD_APP_SB_TIMEOUT_MS 1000

/** Max length (incl. null) of an aircraft ICAO24 hex identifier field */
#define PAYLOAD_APP_ICAO24_LEN 8

/** Max length (incl. null) of an aircraft callsign field */
#define PAYLOAD_APP_CALLSIGN_LEN 16

/** Max length (incl. null) of the envelope "status" string ("OK"/"ERROR") */
#define PAYLOAD_APP_STATUS_LEN 8

/*
** core-cpu1 runs with its CWD set to <repo_root>/build/exe/cpu1 (see the
** repo README build/run instructions). This mirrors how the PSP maps the
** "/cf" virtual volume relative to that same CWD (OS_FileSysAddFixedMap
** "./cf" in psp/fsw/pc-linux/src/cfe_psp_start.c). The socket directory
** lives at <repo_root>/run/payload, so from that CWD it is 3 levels up.
*/
/** Directory (relative to core-cpu1's CWD) where the Payload Unix socket lives */
#define PAYLOAD_APP_SOCKET_DIR_REL "../../../run/payload"

/** File name of the Unix domain socket exposed by the Payload process */
#define PAYLOAD_APP_SOCKET_NAME "payload.sock"

/** Max size (bytes) accepted for a single framed JSON message body */
#define PAYLOAD_APP_SOCK_MAX_MSG_SIZE 4096

/** Timeout (ms) applied to each socket recv() so the AppMain loop is never blocked indefinitely */
#define PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS 200

#endif
