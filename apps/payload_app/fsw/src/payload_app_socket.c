/*
** Unix domain socket client for the Payload API contract:
**   SIZE:<N>\n
**   <N bytes of JSON UTF-8>
**
** OSAL's socket abstraction (osapi-sockets.h) only supports AF_INET/AF_INET6,
** so this talks to the Unix socket with raw POSIX calls, the same style
** libs/obc_hw_lib/fsw/src/linux/narwal_cpu.c uses for /proc/stat. This
** assumes a POSIX/Linux host, which matches this project's only PSP
** (psp/fsw/pc-linux).
*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "cfe_evs.h"

#include "payload_app_platform_cfg.h"
#include "payload_app_eventids.h"
#include "payload_app_socket.h"

static int  PAYLOAD_APP_SocketFd        = -1;
static bool PAYLOAD_APP_SocketConnected = false;

static void PAYLOAD_APP_SocketMkdir(const char *path)
{
  if (mkdir(path, 0775) != 0 && errno != EEXIST)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_CONNECT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: failed to create socket dir '%s', errno=%d (%s)", path, errno,
                       strerror(errno));
  }
}

void PAYLOAD_APP_SocketDirEnsure(void)
{
  /* PAYLOAD_APP_SOCKET_DIR_REL is "../../../run/payload": the ".." parents
  ** already exist (core-cpu1's own build tree), only "run" and "run/payload"
  ** are new, so a two-step mkdir is enough - no need for a general
  ** recursive mkdir -p. */
  PAYLOAD_APP_SocketMkdir("../../../run");
  PAYLOAD_APP_SocketMkdir(PAYLOAD_APP_SOCKET_DIR_REL);
}

bool PAYLOAD_APP_SocketIsConnected(void)
{
  return PAYLOAD_APP_SocketConnected;
}

void PAYLOAD_APP_SocketClose(void)
{
  if (PAYLOAD_APP_SocketFd >= 0)
  {
    close(PAYLOAD_APP_SocketFd);
    PAYLOAD_APP_SocketFd = -1;
  }

  if (PAYLOAD_APP_SocketConnected)
  {
    PAYLOAD_APP_SocketConnected = false;
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_DISCONNECTED_EID, CFE_EVS_EventType_INFORMATION,
                       "PAYLOAD_APP: disconnected from Payload socket");
  }
}

void PAYLOAD_APP_SocketConnect(void)
{
  struct sockaddr_un addr;
  struct timeval     recv_timeout;
  char               sock_path[sizeof(addr.sun_path)];
  int                fd;

  if (PAYLOAD_APP_SocketConnected)
    return;

  snprintf(sock_path, sizeof(sock_path), "%s/%s", PAYLOAD_APP_SOCKET_DIR_REL, PAYLOAD_APP_SOCKET_NAME);

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_CONNECT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: socket() failed, errno=%d (%s)", errno, strerror(errno));
    return;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
  {
    /* Payload isn't up yet (ENOENT/ECONNREFUSED) - not fatal, retry next cycle */
    close(fd);
    return;
  }

  recv_timeout.tv_sec  = PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS / 1000;
  recv_timeout.tv_usec = (PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS % 1000) * 1000;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

  PAYLOAD_APP_SocketFd        = fd;
  PAYLOAD_APP_SocketConnected = true;

  CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_CONNECTED_EID, CFE_EVS_EventType_INFORMATION,
                     "PAYLOAD_APP: connected to Payload socket '%s'", sock_path);
}

/* Reads one '\n'-terminated line (the "SIZE:<N>" header). *got_any_byte tells
** the caller whether any byte at all had arrived before a timeout - used to
** tell "no message this cycle" apart from "timed out mid-frame". */
static int32 PAYLOAD_APP_SocketRecvLine(char *line_buf, size_t line_buf_size, bool *got_any_byte)
{
  size_t  len = 0;
  char    c;
  ssize_t n;

  *got_any_byte = false;

  while (len + 1 < line_buf_size)
  {
    n = recv(PAYLOAD_APP_SocketFd, &c, 1, 0);

    if (n == 1)
    {
      *got_any_byte = true;

      if (c == '\n')
      {
        line_buf[len] = '\0';
        return PAYLOAD_APP_SOCK_FRAME_OK;
      }

      line_buf[len++] = c;
      continue;
    }

    if (n == 0)
      return PAYLOAD_APP_SOCK_FRAME_ERROR; /* peer closed */

    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      if (*got_any_byte)
        continue; /* mid-frame already, keep waiting for the rest of the header */
      return PAYLOAD_APP_SOCK_FRAME_NONE;
    }

    return PAYLOAD_APP_SOCK_FRAME_ERROR;
  }

  return PAYLOAD_APP_SOCK_FRAME_ERROR; /* header too long, malformed */
}

static int32 PAYLOAD_APP_SocketRecvExact(char *buf, size_t len)
{
  size_t  received = 0;
  ssize_t n;

  while (received < len)
  {
    n = recv(PAYLOAD_APP_SocketFd, buf + received, len - received, 0);

    if (n > 0)
    {
      received += (size_t)n;
      continue;
    }

    if (n == 0)
      return PAYLOAD_APP_SOCK_FRAME_ERROR;

    if (errno == EAGAIN || errno == EWOULDBLOCK)
      continue; /* already mid-frame (header was received), keep waiting for the body */

    return PAYLOAD_APP_SOCK_FRAME_ERROR;
  }

  return PAYLOAD_APP_SOCK_FRAME_OK;
}

int32 PAYLOAD_APP_SocketReadFrame(char *out_buf, size_t out_buf_size, size_t *out_len)
{
  char  size_line[32];
  bool  got_any_byte;
  int32 status;
  long  body_len;
  char *end = NULL;

  *out_len = 0;

  if (!PAYLOAD_APP_SocketConnected)
    return PAYLOAD_APP_SOCK_FRAME_ERROR;

  status = PAYLOAD_APP_SocketRecvLine(size_line, sizeof(size_line), &got_any_byte);
  if (status != PAYLOAD_APP_SOCK_FRAME_OK)
  {
    if (status == PAYLOAD_APP_SOCK_FRAME_ERROR)
      PAYLOAD_APP_SocketClose();
    return status;
  }

  if (strncmp(size_line, "SIZE:", 5) != 0)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_PARSE_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: malformed frame header '%s'", size_line);
    PAYLOAD_APP_SocketClose();
    return PAYLOAD_APP_SOCK_FRAME_ERROR;
  }

  body_len = strtol(size_line + 5, &end, 10);
  if (end == size_line + 5 || body_len <= 0 || (size_t)body_len >= out_buf_size ||
      (size_t)body_len > PAYLOAD_APP_SOCK_MAX_MSG_SIZE)
  {
    CFE_EVS_SendEvent(PAYLOAD_APP_SOCKET_PARSE_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: invalid frame size '%s'", size_line);
    PAYLOAD_APP_SocketClose();
    return PAYLOAD_APP_SOCK_FRAME_ERROR;
  }

  status = PAYLOAD_APP_SocketRecvExact(out_buf, (size_t)body_len);
  if (status != PAYLOAD_APP_SOCK_FRAME_OK)
  {
    PAYLOAD_APP_SocketClose();
    return status;
  }

  out_buf[body_len] = '\0';
  *out_len           = (size_t)body_len;

  return PAYLOAD_APP_SOCK_FRAME_OK;
}
