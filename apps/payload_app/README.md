# PAYLOAD_APP

A cFS application that connects, as a Unix domain socket client, to the
Payload process's API and republishes what it receives on the Software Bus.

## Behavior

- On init, ensures `run/payload` exists (relative to `core-cpu1`'s working
  directory, i.e. `<repo_root>/run/payload`, see
  `PAYLOAD_APP_SOCKET_DIR_REL` in `payload_app_platform_cfg.h`) and attempts
  to `connect()` to `payload.sock` inside it. Payload is expected to create
  and listen on that socket; this app is only the client side.
- If Payload isn't up yet, the connect attempt just fails silently and is
  retried every AppMain cycle - this never blocks or crashes the app.
- Once connected, each cycle tries to read one framed response following the
  documented contract:
  ```
  SIZE:<N>\n
  <N bytes of JSON UTF-8>
  ```
  with envelope `{"request_id": N, "status": "OK"|"ERROR", "status_code": N,
  "data": {...}}`. Reads are bounded by
  `PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS` so a quiet socket never stalls the app's
  command pipe.
- The `data` object is parsed into a provisional, typed ADS-B-like schema
  (`icao24`, `callsign`, `latitude`, `longitude`, `altitude`, `ground_speed`,
  `heading`, `vertical_rate`, `timestamp`) - there's no official schema from
  Payload yet, so this is expected to change.
- This app currently only reads; it does not yet send any request/command to
  Payload (no LIVE/START/STOP support). That's a planned follow-up once the
  request side of the contract is implemented.
- Every parsed message is published as `PAYLOAD_APP_DATA_TLM_MID` and printed
  to stdout (`PAYLOAD_APP_PrintDataMsg`) immediately after, from the exact
  same struct instance that was transmitted - so the printed fields and the
  SB message are always identical.

There is no JSON library in this repo, so JSON field extraction is a small
hand-rolled, bounded-scope reader (see `payload_app_data.c`) rather than a
general parser - it does not support nested arrays or escaped strings.

## Commands (`PAYLOAD_APP_CMD_MID`)

| Function code                  | Value | Payload |
|---------------------------------|-------|---------|
| `PAYLOAD_APP_NOOP_CC`           | 0     | none    |
| `PAYLOAD_APP_RESET_COUNTERS_CC` | 1     | none    |

## Telemetry

- `PAYLOAD_APP_TLM_MID` (housekeeping): `cmd_counter`, `err_counter`,
  `socket_connected`, `msgs_received`, `parse_errors`, `last_request_id`,
  `last_status_code`.
- `PAYLOAD_APP_DATA_TLM_MID` (one per Payload message): `request_id`,
  `status_code`, `status`, `icao24`, `callsign`, `latitude`, `longitude`,
  `altitude`, `ground_speed`, `heading`, `vertical_rate`, `timestamp`.

## Config

`config/default_payload_app_platform_cfg.h`: `PAYLOAD_APP_SOCKET_DIR_REL`,
`PAYLOAD_APP_SOCKET_NAME`, `PAYLOAD_APP_SOCK_MAX_MSG_SIZE`,
`PAYLOAD_APP_SOCK_RECV_TIMEOUT_MS`.
