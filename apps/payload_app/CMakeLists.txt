project(CFE_PAYLOAD_APP C)

set(APP_SRC_FILES
  fsw/src/payload_app.c
)

add_cfe_app(payload_app ${APP_SRC_FILES})

target_include_directories(payload_app PUBLIC fsw/inc)
