project(CFE_CSV_READER C)

set(APP_SRC_FILES
  fsw/src/csv_reader.c
)

add_cfe_app(csv_reader ${APP_SRC_FILES})

target_include_directories(csv_reader PUBLIC fsw/inc)
