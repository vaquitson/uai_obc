# PAYLOAD_APP

cFS client of the Payload API v0.4.0 (Unix domain socket, request/response).

## Behavior

- **Transport:** `SIZE:<N>\n` + N bytes of compact UTF-8 JSON, identical for request and response. One
  request per connection: `connect -> send -> read response -> close`, bounded by one total deadline
  (`poll()`; EINTR resumes with the remaining time). Header read byte by byte (max 32 chars); malformed
  headers, `N = 0` and frames larger than `PAYLOAD_APP_RX_BUF_SIZE - 1` are rejected with an event and
  the connection is closed. `send()` uses `MSG_NOSIGNAL`.
- **Tasks:** the main task only handles commands and HK. Socket I/O runs in the `PAYLOAD_WORKER`
  child task, fed by an OSAL queue (`PAYLOAD_APP_REQ_QUEUE_DEPTH`, one request in flight). A full
  queue drops the command with `QUEUE_FULL` and `err_counter++`. HK goes out every
  `PAYLOAD_APP_HK_PERIOD_MS` regardless of command load or a request in progress.
- **Timeouts:** `PAYLOAD_APP_QUERY_TIMEOUT_MS` (5 s) for queries, `PAYLOAD_APP_ACQ_TIMEOUT_MS` (40 s)
  for START/STOP.
- **Service unavailable** (`ENOENT`, `ECONNREFUSED`, `EACCES`): an operator command is retried with
  exponential backoff (`PAYLOAD_APP_CONNECT_BACKOFF_MIN_MS` .. `_MAX_MS`); once the next attempt would
  exceed `PAYLOAD_APP_CONNECT_RETRY_MAX_MS` it is discarded (`WORKER_ERR "... discarded ..., not resent"`,
  `req_err_count++`) and **never resent automatically**: the operator decides. One
  `SERVICE_UNAVAILABLE` event per outage and per errno change, `service_available = 0` while it lasts,
  `SERVICE_RECOVERED` (INFO) when it comes back. The app never creates the socket directory nor starts
  the service.
- **Automatic probing:** after a discard, and when the startup GET_STATUS cannot connect, the worker
  probes the service with GET_STATUS (one attempt each, same backoff, capped at
  `PAYLOAD_APP_CONNECT_BACKOFF_MAX_MS`). Failed probes do not increment `req_err_count`. The backoff
  wait is a queue wait, so an operator command arriving meanwhile is served immediately; if it connects
  it counts as the recovery. After any recovery a GET_STATUS re-checks `interface_version` exactly like
  at startup (`VERSION_OK`/`VERSION_MISMATCH`, `version_ok` in HK).
- **Responses:** `request_id` is an incremental uint32; a response with another `request_id` is
  discarded (`RID_MISMATCH`). `status_code` 1..8 raise `STATUS_ERR` with `data.message` (truncated to
  the EVS limit). `BUSY` (4) is retried up to `PAYLOAD_APP_BUSY_MAX_ATTEMPTS` times with a doubling
  `PAYLOAD_APP_BUSY_BACKOFF_MS`, each attempt with a new `request_id`.
- **Startup:** a GET_STATUS is queued automatically; `interface_version` must equal
  `PAYLOAD_APP_INTERFACE_VERSION` ("0.4.0") -> `VERSION_OK` / `VERSION_MISMATCH` and `version_ok` in HK.
- **Local validation:** GET_SESSION_INFO / READ_SESSION_RANGE are rejected before anything is sent when
  `session_id` is not `YYYYMMDDTHHMMSSZ` or `length` is outside 1..4096 (`VALIDATION_ERR`,
  `err_counter++`). `offset` is a uint32, so `offset >= 0` always holds.
- **TLM bursts:** multi-packet responses (LIVE, LIST, READ) are spaced by `PAYLOAD_APP_TLM_BURST_DELAY_MS`
  so to_lab's pipe is not overrun.
- **JSON:** [jsmn](https://github.com/zserge/jsmn) v1.1.0 (MIT, `fsw/src/jsmn/`), strict mode, no
  malloc. Fields are read by path, unknown keys are ignored, null/missing values are tracked.
- Static buffers only (no malloc): 64 KiB response + 4096 jsmn tokens in the worker module, plus the
  Base64 text/bytes of one READ_SESSION_RANGE and the live aircraft selection in the TLM module.

## Socket path

Default `/run/payload/payload.sock` (`PAYLOAD_APP_SOCKET_PATH_DEFAULT`). Override at runtime with the
environment variable `PAYLOAD_APP_SOCKET_PATH` (max 107 chars). The effective path is reported by the
`SOCKET_PATH` event at startup. `sudo` drops the environment, so pass it explicitly:

```
cd build/exe/cpu1
sudo PAYLOAD_APP_SOCKET_PATH=<repo>/run/payload/payload.sock ./core-cpu1
```

## Commands (`PAYLOAD_APP_CMD_MID` 0x188C)

Command payloads follow the 8-byte command header; lengths are verified (`CMD_LEN_ERR`).

| CC | Command | Payload | Result |
|----|---------|---------|--------|
| 0 | NOOP | - | event |
| 1 | RESET_COUNTERS | - | resets cmd, err, req_ok, req_err |
| 2 | GET_STATUS | - | HK status fields |
| 3 | START_ACQUISITION | - | `ACQ_INFO` with `data.message`, HK from `data.status` (40 s timeout) |
| 4 | STOP_ACQUISITION | - | `ACQ_INFO` with `data.message`, HK from `data.status` (40 s timeout) |
| 5 | GET_LIVE_STATE | - | LIVE packets (0x088E), HK `live_aircraft_count` |
| 6 | LIST_SESSIONS | - | SESSION packets (0x088F, source 0) or `LIST_EMPTY` |
| 7 | GET_SESSION_INFO | `char session_id[17]; u8 spare[3]` (20 B) | one SESSION packet (source 1) |
| 8 | READ_SESSION_RANGE | `char session_id[17]; u8 spare[3]; u32 offset; u16 length; u16 spare` (28 B) | CHUNK packets (0x0890), `READ_EOF` at end of file |

READ_SESSION_RANGE makes exactly one request per command; downloading a whole session (advancing
`offset = next_offset` until `eof`) is up to the operator for now.

## Telemetry

All payloads use fixed-width types, natural alignment and sizes multiple of 8 (no implicit padding on
i686, x86-64 or aarch64; checked at compile time). Offsets below are relative to the end of the 16-byte
telemetry header.

### HK (`PAYLOAD_APP_TLM_MID` 0x088D, 64 B payload, `tlm_layout_version` = 1)

| Off | Type | Field |
|-----|------|-------|
| 0 | u8 | cmd_counter |
| 1 | u8 | err_counter |
| 2 | u8 | payload_state (0 READY, 1 ACQUIRING, 0xFF unknown) |
| 3 | u8 | receiver_state (0 INACTIVE, 1 ACTIVE, 0xFF unknown) |
| 4 | u8 | logger_state (0 INACTIVE, 1 ACTIVE, 0xFF unknown) |
| 5 | u8 | live_consumer_connected (0/1, 0xFF unknown) |
| 6 | u8 | last_status_code |
| 7 | u8 | version_ok (0/1, 0xFF not checked yet) |
| 8 | u32 | req_ok_count |
| 12 | u32 | req_err_count |
| 16 | u32 | last_request_id |
| 20 | u16 | live_aircraft_count |
| 22 | u8 | service_available (0/1) |
| 23 | u8 | tlm_layout_version |
| 24 | u64 | storage_free_bytes |
| 32 | char[17] | active_session_id ("" = null) |
| 49 | char[8] | interface_version |
| 57 | u8[7] | spare |

### LIVE (`PAYLOAD_APP_DATA_TLM_MID` 0x088E, 80 B payload, one per aircraft)

At most `PAYLOAD_APP_LIVE_MAX_AIRCRAFT_PER_REQ` (20) packets, lowest `age_s` first (null `age_s` last).
`aircraft_count` (and HK `live_aircraft_count`) is the total reported by Payload. `aircraft_count = 0`
publishes nothing and raises no event. Aircraft without a valid `icao` are skipped (`JSON_ERR`).

| Off | Type | Field |
|-----|------|-------|
| 0 | u32 | request_id |
| 4 | u16 | aircraft_index |
| 6 | u16 | aircraft_count |
| 8 | u32 | valid_flags (bits below) |
| 12 | u32 | timestamp_utc (epoch s) |
| 16 | f64 | latitude |
| 24 | f64 | longitude |
| 32 | i32 | altitude_ft |
| 36 | f32 | ground_speed_kt |
| 40 | f32 | heading_deg |
| 44 | f32 | vertical_rate_fpm |
| 48 | f32 | age_s |
| 52 | u32 | last_seen_utc (epoch s) |
| 56 | u16 | ttl_s |
| 58 | u8 | is_on_ground |
| 59 | char[7] | icao |
| 66 | char[9] | callsign |
| 75 | char[5] | squawk |

`valid_flags` (a clear bit means null, missing or invalid): 0 callsign, 1 latitude, 2 longitude,
3 altitude_ft, 4 ground_speed_kt, 5 heading_deg, 6 vertical_rate_fpm, 7 squawk, 8 is_on_ground,
9 last_seen_utc, 10 age_s, 11 timestamp_utc, 12 ttl_s.

### SESSION (`PAYLOAD_APP_SESSION_TLM_MID` 0x088F, 80 B payload)

One per LIST_SESSIONS entry (`source` 0, most recent first, as returned by Payload) or one for
GET_SESSION_INFO (`source` 1; missing fields only clear their bit, the requested `session_id` is used if
the response has none).

| Off | Type | Field |
|-----|------|-------|
| 0 | u32 | request_id |
| 4 | u16 | session_index |
| 6 | u16 | session_count |
| 8 | u32 | valid_flags (bits below) |
| 12 | u8 | source (0 LIST, 1 INFO) |
| 13 | char[17] | session_id |
| 30 | u16 | spare |
| 32 | u32 | start_utc (epoch s) |
| 36 | u32 | end_utc (epoch s) |
| 40 | u32 | duration_s |
| 44 | u32 | messages |
| 48 | u32 | unique_aircraft |
| 52 | f32 | messages_per_minute |
| 56 | u64 | file_size_bytes |
| 64 | u32 | reconnects |
| 68 | char[12] | software_version |

`valid_flags`: 0 start_utc, 1 end_utc, 2 duration_s, 3 messages, 4 unique_aircraft,
5 messages_per_minute, 6 file_size_bytes, 7 reconnects, 8 software_version.

### CHUNK (`PAYLOAD_APP_CHUNK_TLM_MID` 0x0890, 40 B + `PAYLOAD_APP_CHUNK_TLM_MAX` payload)

The Base64 `data` of a READ_SESSION_RANGE response is decoded and split into pieces of
`PAYLOAD_APP_CHUNK_TLM_MAX` bytes (default 176: 16 + 40 + 176 = 232 B per packet, LoRa friendly;
must be a multiple of 8). A 4096-byte read gives 24 packets. A read at end of file with 0 bytes still
publishes one packet (`n_bytes` 0, `eof` 1). `bytes_returned` must match the decoded length.

| Off | Type | Field |
|-----|------|-------|
| 0 | u32 | request_id |
| 4 | u16 | n_bytes |
| 6 | u8 | eof (only on the last chunk of a response with `eof` true) |
| 7 | u8 | spare |
| 8 | u64 | offset (absolute, of data[0]) |
| 16 | char[17] | session_id |
| 33 | u8[7] | spare |
| 40 | u8[176] | data |

## Events

| EID | Type | Name | Meaning |
|-----|------|------|---------|
| 1 | ERR | INIT_FAILURE | init step failed / env path too long |
| 2 | INFO | INIT_SUCCESSFUL | |
| 3 | ERR | HK_TRANSMIT_ERR | |
| 4 | INFO | NOOP | |
| 5 | INFO | RESET_COUNTERS | |
| 6 | ERR | INVALID_MID | |
| 7 | ERR | INVALID_CC | |
| 8 | INFO | SOCKET_PATH | effective socket path and source (env/default) |
| 9 | ERR | SERVICE_UNAVAILABLE | connect() failed: ENOENT/ECONNREFUSED/EACCES (throttled) |
| 10 | INFO | SERVICE_RECOVERED | |
| 11 | ERR | REQ_TIMEOUT | request deadline expired |
| 12 | ERR | FRAME_ERR | malformed header, oversized frame, peer closed |
| 13 | ERR | RID_MISMATCH | response discarded |
| 14 | ERR | STATUS_ERR | `<OP> rid=N NAME(code): data.message` |
| 15 | ERR | BUSY_EXHAUSTED | BUSY on every attempt |
| 16 | ERR | VERSION_MISMATCH | |
| 17 | INFO | VERSION_OK | |
| 18 | ERR | VALIDATION_ERR | command arguments rejected before sending |
| 19 | ERR | QUEUE_FULL | |
| 20 | INFO | ACQ_INFO | START/STOP `data.message` |
| 21 | INFO | LIST_EMPTY | LIST_SESSIONS returned no sessions |
| 22 | INFO | READ_EOF | READ_SESSION_RANGE reached end of file |
| 23 | ERR | NOT_IMPLEMENTED | internal: response handler missing for an op |
| 24 | ERR | JSON_ERR | invalid JSON / envelope / unusable `data` |
| 25 | ERR | TLM_TRANSMIT_ERR | data TLM transmit failed |
| 26 | ERR | CMD_LEN_ERR | |
| 27 | INFO | BUSY_RETRY | |
| 28 | ERR | WORKER_ERR | queue error, or command discarded after connect retries (not resent) |

## Assumptions to confirm with the real Payload

The development mock fixed these points that the API contract does not specify:

- START_ACQUISITION while already ACQUIRING returns `4 BUSY` (so the app retries it up to
  `PAYLOAD_APP_BUSY_MAX_ATTEMPTS` times before reporting `BUSY_EXHAUSTED`).
- STOP_ACQUISITION while not acquiring returns OK (idempotent).
- GET_SESSION_INFO / READ_SESSION_RANGE on the active session return `3 NOT_FOUND`.
- READ_SESSION_RANGE with `offset > size` returns `2`; `offset == size` returns 0 bytes with `eof`.
- GET_LIVE_STATE returns aircraft only while ACQUIRING.
- Exact shape of the GET_SESSION_INFO response.
- Presence and types of the session fields `messages_per_minute`, `reconnects`, `software_version`.
- `EACCES` handling is not verified locally: core-cpu1 runs as root, which bypasses socket permissions.

## Known issues outside this app

- `ci_lab` and `cmd_hand` both bind UDP 1234; in practice `cmd_hand` receives (and forwards) the
  ground commands.
- `sch_lab` has no HK schedule for this app; HK is generated by the app's own main loop.
