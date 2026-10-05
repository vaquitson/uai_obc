#include "payload_base64.h"

static int PAYLOAD_Base64Value(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

int PAYLOAD_Base64Decode(const char *in, size_t in_len, uint8_t *out, size_t out_size, size_t *out_len)
{
  size_t   i;
  size_t   pad = 0;
  size_t   n;
  size_t   o = 0;
  size_t   bytes;
  uint32_t quad;
  int      last;
  int      k;
  int      v;

  if (out_len == NULL || (in == NULL && in_len > 0))
    return PAYLOAD_BASE64_ERR_INVALID;
  *out_len = 0;
  if (in_len % 4 != 0)
    return PAYLOAD_BASE64_ERR_INVALID;
  if (in_len == 0)
    return PAYLOAD_BASE64_OK;

  if (in[in_len - 1] == '=')
    pad = (in[in_len - 2] == '=') ? 2 : 1;
  n = in_len / 4 * 3 - pad;
  if (n > out_size)
    return PAYLOAD_BASE64_ERR_SPACE;

  for (i = 0; i < in_len; i += 4)
  {
    last  = (i + 4 == in_len);
    quad  = 0;
    for (k = 0; k < 4; k++)
    {
      if (last && k >= 4 - (int)pad)
        v = 0; /* '=' already verified by the pad count */
      else if ((v = PAYLOAD_Base64Value(in[i + k])) < 0)
        return PAYLOAD_BASE64_ERR_INVALID;
      quad = (quad << 6) | (uint32_t)v;
    }
    bytes = last ? 3 - pad : 3;
    out[o++] = (uint8_t)(quad >> 16);
    if (bytes > 1)
      out[o++] = (uint8_t)(quad >> 8);
    if (bytes > 2)
      out[o++] = (uint8_t)quad;
  }

  *out_len = n;
  return PAYLOAD_BASE64_OK;
}
