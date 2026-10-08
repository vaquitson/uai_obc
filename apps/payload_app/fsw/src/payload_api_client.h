#ifndef PAYLOAD_API_CLIENT_H
#define PAYLOAD_API_CLIENT_H

/*
** Payload API transport, independent of cFE (POSIX only) so it can be unit
** tested on the host. Framing, identical for request and response:
**   "SIZE:<N>\n" followed by N bytes of compact UTF-8 JSON.
** One request per connection: connect -> send -> read response -> close,
** bounded by a single total deadline.
*/

#include <stddef.h>
#include <stdint.h>

/** Max header line length before '\n' ("SIZE:" + digits) */
#define PAYLOAD_API_MAX_HEADER 32

typedef enum {
  PAYLOAD_API_OK = 0,
  PAYLOAD_API_ERR_ENOENT,       /* socket file missing: service down */
  PAYLOAD_API_ERR_ECONNREFUSED, /* nobody listening: service restarting */
  PAYLOAD_API_ERR_EACCES,       /* no permission on the socket */
  PAYLOAD_API_ERR_CONNECT,      /* any other connect()/socket() failure */
  PAYLOAD_API_ERR_TIMEOUT,      /* total deadline expired */
  PAYLOAD_API_ERR_HEADER,       /* malformed "SIZE:<N>" header */
  PAYLOAD_API_ERR_TOO_BIG,      /* N does not fit the response buffer */
  PAYLOAD_API_ERR_CLOSED,       /* peer closed or reset the connection */
  PAYLOAD_API_ERR_IO,           /* unexpected send/recv/poll error */
  PAYLOAD_API_ERR_ARG           /* invalid arguments */
} PAYLOAD_API_Result_t;

/** True for the results that mean "could not reach the service" */
int PAYLOAD_API_IsConnectError(PAYLOAD_API_Result_t result);

/** Short constant name of a result, for events and logs */
const char *PAYLOAD_API_ResultName(PAYLOAD_API_Result_t result);

/** Monotonic clock in milliseconds */
uint64_t PAYLOAD_API_NowMs(void);

/*
** Full request/response exchange. On success resp holds the NUL-terminated
** JSON body (resp_size must leave room for the NUL) and *resp_len its length.
** *err_no receives the errno behind a failure (0 if none) and may be NULL.
** header_out (may be NULL, >= PAYLOAD_API_MAX_HEADER + 1 bytes) receives the
** offending header text on PAYLOAD_API_ERR_HEADER/TOO_BIG.
*/
PAYLOAD_API_Result_t PAYLOAD_API_Transact(const char *socket_path, const char *req, size_t req_len, char *resp,
                                          size_t resp_size, size_t *resp_len, uint32_t timeout_ms, int *err_no,
                                          char *header_out);

/* Framing primitives on an already connected fd, exposed for unit tests */
PAYLOAD_API_Result_t PAYLOAD_API_SendFrame(int fd, const char *body, size_t body_len, uint64_t deadline_ms,
                                           int *err_no);
PAYLOAD_API_Result_t PAYLOAD_API_RecvFrame(int fd, char *buf, size_t buf_size, size_t *body_len,
                                           uint64_t deadline_ms, int *err_no, char *header_out);

#endif
