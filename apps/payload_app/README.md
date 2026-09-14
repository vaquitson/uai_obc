# PAYLOAD_APP

A cFS application that reads and parses a CSV file from the onboard
filesystem (the `/cf` volume).

## Behavior

- On startup, automatically opens and parses `PAYLOAD_APP_DEFAULT_FILE`
  (`/cf/payload_app_data.csv` by default, see `payload_app_platform_cfg.h`).
- Accepts a `PAYLOAD_APP_READ_FILE_CC` command carrying a file path, which
  (re)loads and parses that CSV file on demand.
- Non-numeric fields (e.g. a header row) are skipped while parsing.
- Parsed numeric values are stored internally as a `rows x cols` float
  matrix; row/column counts and sum/min/max of all parsed values are
  published in the app's housekeeping telemetry (`PAYLOAD_APP_TLM_MID`)
  once per cycle.

## Commands (`PAYLOAD_APP_CMD_MID`)

| Function code               | Value | Payload                          |
|------------------------------|-------|-----------------------------------|
| `PAYLOAD_APP_NOOP_CC`          | 0     | none                               |
| `PAYLOAD_APP_RESET_COUNTERS_CC`| 1     | none                               |
| `PAYLOAD_APP_READ_FILE_CC`     | 2     | `char file_name[PAYLOAD_APP_FILENAME_LEN]` |

## Telemetry (`PAYLOAD_APP_TLM_MID`)

`cmd_counter`, `err_counter`, `last_read_valid`, `rows_read`, `cols_read`,
`value_sum`, `value_min`, `value_max`, `last_file`.

## Limits

Configurable in `config/default_payload_app_platform_cfg.h`:
`PAYLOAD_APP_MAX_ROWS`, `PAYLOAD_APP_MAX_COLS`, `PAYLOAD_APP_FILE_BUF_SIZE`
(the whole file is read into memory in one shot, so it must fit).
