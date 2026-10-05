#ifndef PAYLOAD_BASE64_H
#define PAYLOAD_BASE64_H

#include <stddef.h>
#include <stdint.h>

#define PAYLOAD_BASE64_OK          0
#define PAYLOAD_BASE64_ERR_INVALID -1 /* bad length, character or padding */
#define PAYLOAD_BASE64_ERR_SPACE   -2 /* decoded data does not fit out_size */

/*
** Strict standard-alphabet Base64 decoder (RFC 4648, padded, no whitespace),
** independent of cFE. Never writes more than out_size bytes.
*/
int PAYLOAD_Base64Decode(const char *in, size_t in_len, uint8_t *out, size_t out_size, size_t *out_len);

#endif
