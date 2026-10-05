project(CFE_PAYLOAD_APP C)

set(APP_SRC_FILES
  fsw/src/payload_app.c
  fsw/src/payload_app_worker.c
  fsw/src/payload_api_client.c
  fsw/src/payload_api_json.c
  fsw/src/payload_base64.c
)

add_cfe_app(payload_app ${APP_SRC_FILES})

target_include_directories(payload_app PUBLIC fsw/inc)
target_include_directories(payload_app PRIVATE fsw/src/jsmn)
