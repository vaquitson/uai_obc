#ifndef PAYLOAD_APP_SOCKET_H
#define PAYLOAD_APP_SOCKET_H

#include <stddef.h>
#include "common_types.h"

/* PAYLOAD_APP_SocketReadFrame() outcomes */
#define PAYLOAD_APP_SOCK_FRAME_OK    0  /* a full framed JSON message was read into out_buf */
#define PAYLOAD_APP_SOCK_FRAME_NONE  1  /* recv timed out, nothing new this cycle */
#define PAYLOAD_APP_SOCK_FRAME_ERROR 2  /* socket closed or errored, caller should reconnect */

/* Creates run/payload (relative to core-cpu1's CWD) if it doesn't exist yet */
void PAYLOAD_APP_SocketDirEnsure(void);

/* True if a connection to Payload's socket is currently established */
bool PAYLOAD_APP_SocketIsConnected(void);

/* Attempts a single (non-retrying) connect() to Payload's Unix socket. Safe to
** call every AppMain cycle: does nothing if already connected. */
void PAYLOAD_APP_SocketConnect(void);

/* Reads one SIZE:<N>\n + N-byte JSON frame, bounded by
** PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS per recv() call so the caller's loop is
** never blocked indefinitely. On PAYLOAD_APP_SOCK_FRAME_OK, *out_len holds
** the JSON body length written into out_buf (NUL-terminated). On
** PAYLOAD_APP_SOCK_FRAME_ERROR the connection has already been closed
** internally; the caller should retry PAYLOAD_APP_SocketConnect() later. */
int32 PAYLOAD_APP_SocketReadFrame(char *out_buf, size_t out_buf_size, size_t *out_len);

/* Closes the connection, if any */
void PAYLOAD_APP_SocketClose(void);

#endif
