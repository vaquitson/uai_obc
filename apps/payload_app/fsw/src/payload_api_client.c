#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "payload_api_client.h"

int PAYLOAD_API_IsConnectError(PAYLOAD_API_Result_t result)
{
  return result == PAYLOAD_API_ERR_ENOENT || result == PAYLOAD_API_ERR_ECONNREFUSED ||
         result == PAYLOAD_API_ERR_EACCES || result == PAYLOAD_API_ERR_CONNECT;
}

const char *PAYLOAD_API_ResultName(PAYLOAD_API_Result_t result)
{
  switch (result)
  {
    case PAYLOAD_API_OK:
      return "OK";
    case PAYLOAD_API_ERR_ENOENT:
      return "ENOENT";
    case PAYLOAD_API_ERR_ECONNREFUSED:
      return "ECONNREFUSED";
    case PAYLOAD_API_ERR_EACCES:
      return "EACCES";
    case PAYLOAD_API_ERR_CONNECT:
      return "CONNECT";
    case PAYLOAD_API_ERR_TIMEOUT:
      return "TIMEOUT";
    case PAYLOAD_API_ERR_HEADER:
      return "HEADER";
    case PAYLOAD_API_ERR_TOO_BIG:
      return "TOO_BIG";
    case PAYLOAD_API_ERR_CLOSED:
      return "CLOSED";
    case PAYLOAD_API_ERR_IO:
      return "IO";
    case PAYLOAD_API_ERR_ARG:
      return "ARG";
  }
  return "UNKNOWN";
}

uint64_t PAYLOAD_API_NowMs(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static int PAYLOAD_API_RemainingMs(uint64_t deadline_ms)
{
  uint64_t now = PAYLOAD_API_NowMs();

  if (now >= deadline_ms)
    return 0;
  if (deadline_ms - now > (uint64_t)INT_MAX)
    return INT_MAX;
  return (int)(deadline_ms - now);
}

static void PAYLOAD_API_SetErrno(int *err_no, int value)
{
  if (err_no != NULL)
    *err_no = value;
}

/* Waits until fd is ready for 'events' or the deadline expires; EINTR resumes with the remaining time */
static PAYLOAD_API_Result_t PAYLOAD_API_WaitFd(int fd, short events, uint64_t deadline_ms, int *err_no)
{
  struct pollfd pfd;
  int           rc;
  int           remaining;

  for (;;)
  {
    remaining = PAYLOAD_API_RemainingMs(deadline_ms);
    if (remaining == 0)
      return PAYLOAD_API_ERR_TIMEOUT;

    pfd.fd      = fd;
    pfd.events  = events;
    pfd.revents = 0;

    rc = poll(&pfd, 1, remaining);
    if (rc > 0)
      return PAYLOAD_API_OK; /* readiness, HUP or ERR: the following recv/send reports the outcome */
    if (rc < 0 && errno != EINTR)
    {
      PAYLOAD_API_SetErrno(err_no, errno);
      return PAYLOAD_API_ERR_IO;
    }
  }
}

static PAYLOAD_API_Result_t PAYLOAD_API_MapIoErrno(int e, int *err_no)
{
  PAYLOAD_API_SetErrno(err_no, e);
  if (e == EPIPE || e == ECONNRESET)
    return PAYLOAD_API_ERR_CLOSED;
  return PAYLOAD_API_ERR_IO;
}

static PAYLOAD_API_Result_t PAYLOAD_API_SendAll(int fd, const char *data, size_t len, uint64_t deadline_ms,
                                                int *err_no)
{
  PAYLOAD_API_Result_t result;
  ssize_t              n;
  size_t               sent = 0;

  while (sent < len)
  {
    result = PAYLOAD_API_WaitFd(fd, POLLOUT, deadline_ms, err_no);
    if (result != PAYLOAD_API_OK)
      return result;

    /* MSG_NOSIGNAL: a peer that already closed must not raise SIGPIPE in the cFE process */
    n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
    if (n > 0)
    {
      sent += (size_t)n;
      continue;
    }
    if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
      continue;
    return PAYLOAD_API_MapIoErrno(n < 0 ? errno : EPIPE, err_no);
  }

  return PAYLOAD_API_OK;
}

/* Receives exactly len bytes (or one byte when len == 1 for the header) */
static PAYLOAD_API_Result_t PAYLOAD_API_RecvAll(int fd, char *buf, size_t len, uint64_t deadline_ms, int *err_no)
{
  PAYLOAD_API_Result_t result;
  ssize_t              n;
  size_t               got = 0;

  while (got < len)
  {
    result = PAYLOAD_API_WaitFd(fd, POLLIN, deadline_ms, err_no);
    if (result != PAYLOAD_API_OK)
      return result;

    n = recv(fd, buf + got, len - got, 0);
    if (n > 0)
    {
      got += (size_t)n;
      continue;
    }
    if (n == 0)
      return PAYLOAD_API_ERR_CLOSED;
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
      continue;
    return PAYLOAD_API_MapIoErrno(errno, err_no);
  }

  return PAYLOAD_API_OK;
}

PAYLOAD_API_Result_t PAYLOAD_API_SendFrame(int fd, const char *body, size_t body_len, uint64_t deadline_ms,
                                           int *err_no)
{
  char                 header[PAYLOAD_API_MAX_HEADER + 1];
  int                  header_len;
  PAYLOAD_API_Result_t result;

  if (body == NULL || body_len == 0)
    return PAYLOAD_API_ERR_ARG;

  header_len = snprintf(header, sizeof(header), "SIZE:%lu\n", (unsigned long)body_len);
  if (header_len <= 0 || (size_t)header_len >= sizeof(header))
    return PAYLOAD_API_ERR_ARG;

  result = PAYLOAD_API_SendAll(fd, header, (size_t)header_len, deadline_ms, err_no);
  if (result == PAYLOAD_API_OK)
    result = PAYLOAD_API_SendAll(fd, body, body_len, deadline_ms, err_no);
  return result;
}

static void PAYLOAD_API_CopyHeader(char *header_out, const char *header, size_t len)
{
  size_t i;

  if (header_out == NULL)
    return;
  for (i = 0; i < len && i < PAYLOAD_API_MAX_HEADER; i++)
    header_out[i] = (header[i] >= 0x20 && header[i] < 0x7F) ? header[i] : '?';
  header_out[i] = '\0';
}

PAYLOAD_API_Result_t PAYLOAD_API_RecvFrame(int fd, char *buf, size_t buf_size, size_t *body_len,
                                           uint64_t deadline_ms, int *err_no, char *header_out)
{
  char                 header[PAYLOAD_API_MAX_HEADER];
  size_t               header_len = 0;
  size_t               i;
  size_t               n          = 0;
  char                 c;
  PAYLOAD_API_Result_t result;

  if (buf == NULL || buf_size < 2 || body_len == NULL)
    return PAYLOAD_API_ERR_ARG;
  *body_len = 0;

  /* Header byte by byte, so nothing past '\n' is consumed */
  for (;;)
  {
    result = PAYLOAD_API_RecvAll(fd, &c, 1, deadline_ms, err_no);
    if (result != PAYLOAD_API_OK)
      return result;
    if (c == '\n')
      break;
    if (header_len >= sizeof(header))
    {
      PAYLOAD_API_CopyHeader(header_out, header, header_len);
      return PAYLOAD_API_ERR_HEADER;
    }
    header[header_len++] = c;
  }

  if (header_len < 6 || memcmp(header, "SIZE:", 5) != 0)
  {
    PAYLOAD_API_CopyHeader(header_out, header, header_len);
    return PAYLOAD_API_ERR_HEADER;
  }
  for (i = 5; i < header_len; i++)
  {
    if (header[i] < '0' || header[i] > '9' || n > ((size_t)-1 - 9) / 10)
    {
      PAYLOAD_API_CopyHeader(header_out, header, header_len);
      return PAYLOAD_API_ERR_HEADER;
    }
    n = n * 10 + (size_t)(header[i] - '0');
  }
  if (n == 0)
  {
    PAYLOAD_API_CopyHeader(header_out, header, header_len);
    return PAYLOAD_API_ERR_HEADER;
  }
  if (n > buf_size - 1)
  {
    PAYLOAD_API_CopyHeader(header_out, header, header_len);
    return PAYLOAD_API_ERR_TOO_BIG;
  }

  result = PAYLOAD_API_RecvAll(fd, buf, n, deadline_ms, err_no);
  if (result != PAYLOAD_API_OK)
    return result;

  buf[n]    = '\0';
  *body_len = n;
  return PAYLOAD_API_OK;
}

static PAYLOAD_API_Result_t PAYLOAD_API_Connect(int fd, const char *socket_path, uint64_t deadline_ms, int *err_no)
{
  struct sockaddr_un addr;
  struct timeval     tv;
  int                remaining;
  int                so_error = 0;
  socklen_t          so_len   = sizeof(so_error);
  int                e;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, socket_path, strlen(socket_path) + 1);

  /* A blocking AF_UNIX connect() waits on a full backlog bounded by SO_SNDTIMEO */
  remaining  = PAYLOAD_API_RemainingMs(deadline_ms);
  tv.tv_sec  = remaining / 1000;
  tv.tv_usec = (remaining % 1000) * 1000;
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
    return PAYLOAD_API_OK;

  e = errno;
  if (e == EINTR)
  {
    /* The connection continues asynchronously after EINTR; wait for it */
    if (PAYLOAD_API_WaitFd(fd, POLLOUT, deadline_ms, err_no) != PAYLOAD_API_OK)
      return PAYLOAD_API_ERR_TIMEOUT;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) == 0 && so_error == 0)
      return PAYLOAD_API_OK;
    e = so_error;
  }

  PAYLOAD_API_SetErrno(err_no, e);
  switch (e)
  {
    case ENOENT:
      return PAYLOAD_API_ERR_ENOENT;
    case ECONNREFUSED:
      return PAYLOAD_API_ERR_ECONNREFUSED;
    case EACCES:
    case EPERM:
      return PAYLOAD_API_ERR_EACCES;
    case EAGAIN:
    case EINPROGRESS:
      return PAYLOAD_API_ERR_TIMEOUT;
    default:
      return PAYLOAD_API_ERR_CONNECT;
  }
}

PAYLOAD_API_Result_t PAYLOAD_API_Transact(const char *socket_path, const char *req, size_t req_len, char *resp,
                                          size_t resp_size, size_t *resp_len, uint32_t timeout_ms, int *err_no,
                                          char *header_out)
{
  struct sockaddr_un   addr;
  uint64_t             deadline_ms;
  int                  fd;
  int                  flags;
  PAYLOAD_API_Result_t result;

  PAYLOAD_API_SetErrno(err_no, 0);
  if (header_out != NULL)
    header_out[0] = '\0';
  if (socket_path == NULL || strlen(socket_path) == 0 || strlen(socket_path) >= sizeof(addr.sun_path) ||
      req == NULL || req_len == 0 || resp == NULL || resp_size < 2 || resp_len == NULL)
    return PAYLOAD_API_ERR_ARG;
  *resp_len = 0;

  deadline_ms = PAYLOAD_API_NowMs() + timeout_ms;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
  {
    PAYLOAD_API_SetErrno(err_no, errno);
    return PAYLOAD_API_ERR_CONNECT;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);

  result = PAYLOAD_API_Connect(fd, socket_path, deadline_ms, err_no);
  if (result == PAYLOAD_API_OK)
  {
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      PAYLOAD_API_SetErrno(err_no, errno);
      result = PAYLOAD_API_ERR_IO;
    }
  }
  if (result == PAYLOAD_API_OK)
    result = PAYLOAD_API_SendFrame(fd, req, req_len, deadline_ms, err_no);
  if (result == PAYLOAD_API_OK)
    result = PAYLOAD_API_RecvFrame(fd, resp, resp_size, resp_len, deadline_ms, err_no, header_out);

  close(fd);
  return result;
}
