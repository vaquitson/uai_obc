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
- **Service unavailable** (`ENOENT`, `ECONNREFUSED`, `EACCES`): exponential backoff between
  `PAYLOAD_APP_CONNECT_BACKOFF_MIN_MS` and `_MAX_MS`. A request is discarded after
  `PAYLOAD_APP_CONNECT_RETRY_MAX_MS`; the startup GET_STATUS retries forever. One
  `SERVICE_UNAVAILABLE` event per outage and per errno change, `SERVICE_RECOVERED` when it comes back,
  `service_available` in HK. The app never creates the socket directory nor starts the service.
- **Responses:** `request_id` is an incremental uint32; a response with another `request_id` is
  discarded (`RID_MISMATCH`). `status_code` 1..8 raise `STATUS_ERR` with `data.message` (truncated to
  the EVS limit). `BUSY` (4) is retried up to `PAYLOAD_APP_BUSY_MAX_ATTEMPTS` times with a doubling
  `PAYLOAD_APP_BUSY_BACKOFF_MS`, each attempt with a new `request_id`.
- **Startup:** a GET_STATUS is queued automatically; `interface_version` must equal
  `PAYLOAD_APP_INTERFACE_VERSION` ("0.4.0") -> `VERSION_OK` / `VERSION_MISMATCH` and `version_ok` in HK.
- **JSON:** [jsmn](https://github.com/zserge/jsmn) v1.1.0 (MIT, `fsw/src/jsmn/`), strict mode, no
  malloc. Fields are read by path, unknown keys are ignored, null/missing values are tracked.
- Static buffers only (no malloc): 64 KiB response + 4096 jsmn tokens in the worker module.

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

| CC | Command | Payload | Status |
|----|---------|---------|--------|
| 0 | NOOP | - | |
| 1 | RESET_COUNTERS | - (cmd, err, req_ok, req_err) | |
| 2 | GET_STATUS | - | implemented |
| 3 | START_ACQUISITION | - | accepted, `NOT_IMPLEMENTED` (stage B) |
| 4 | STOP_ACQUISITION | - | accepted, `NOT_IMPLEMENTED` (stage B) |
| 5 | GET_LIVE_STATE | - | accepted, `NOT_IMPLEMENTED` (stage B) |
| 6 | LIST_SESSIONS | - | accepted, `NOT_IMPLEMENTED` (stage B) |
| 7 | GET_SESSION_INFO | `char session_id[17]; u8 spare[3]` (20 B) | accepted, `NOT_IMPLEMENTED` (stage B) |
| 8 | READ_SESSION_RANGE | `char session_id[17]; u8 spare[3]; u32 offset; u16 length; u16 spare` (28 B) | accepted, `NOT_IMPLEMENTED` (stage B) |

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

`PAYLOAD_APP_DATA_TLM_MID` (0x088E) and the session/chunk packets are defined in stage B.

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
| 18 | ERR | VALIDATION_ERR | command arguments rejected before sending (stage B) |
| 19 | ERR | QUEUE_FULL | |
| 20 | INFO | ACQ_INFO | START/STOP `data.message` (stage B) |
| 21 | INFO | LIST_EMPTY | (stage B) |
| 22 | INFO | READ_EOF | (stage B) |
| 23 | ERR | NOT_IMPLEMENTED | command accepted but not implemented yet |
| 24 | ERR | JSON_ERR | invalid JSON / envelope |
| 25 | ERR | TLM_TRANSMIT_ERR | (stage B) |
| 26 | ERR | CMD_LEN_ERR | |
| 27 | INFO | BUSY_RETRY | |
| 28 | ERR | WORKER_ERR | queue error, request discarded after connect retries |

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
