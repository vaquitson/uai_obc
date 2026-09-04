# CSV_READER

A cFS application that reads and parses a CSV file from the onboard
filesystem (the `/cf` volume).

## Behavior

- On startup, automatically opens and parses `CSV_READER_DEFAULT_FILE`
  (`/cf/csv_reader_data.csv` by default, see `csv_reader_platform_cfg.h`).
- Accepts a `CSV_READER_READ_FILE_CC` command carrying a file path, which
  (re)loads and parses that CSV file on demand.
- Non-numeric fields (e.g. a header row) are skipped while parsing.
- Parsed numeric values are stored internally as a `rows x cols` float
  matrix; row/column counts and sum/min/max of all parsed values are
  published in the app's housekeeping telemetry (`CSV_READER_TLM_MID`)
  once per cycle.

## Commands (`CSV_READER_CMD_MID`)

| Function code               | Value | Payload                          |
|------------------------------|-------|-----------------------------------|
| `CSV_READER_NOOP_CC`          | 0     | none                               |
| `CSV_READER_RESET_COUNTERS_CC`| 1     | none                               |
| `CSV_READER_READ_FILE_CC`     | 2     | `char file_name[CSV_READER_FILENAME_LEN]` |

## Telemetry (`CSV_READER_TLM_MID`)

`cmd_counter`, `err_counter`, `last_read_valid`, `rows_read`, `cols_read`,
`value_sum`, `value_min`, `value_max`, `last_file`.

## Limits

Configurable in `config/default_csv_reader_platform_cfg.h`:
`CSV_READER_MAX_ROWS`, `CSV_READER_MAX_COLS`, `CSV_READER_FILE_BUF_SIZE`
(the whole file is read into memory in one shot, so it must fit).
