project(CFE_TELECOM C)

set(APP_SRC_FILES
    fsw/src/telecom_app.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/telecom_eds_encode.c)
else()
  list(APPEND APP_SRC_FILES
  fsw/src/telecom_passthru_encode.c)
endif(CFE_EDS_ENABLED)


if (COMMUNICATION_LORA)
  list(APPEND APP_SRC_FILES
  fsw/src/telecom_lora_controller.c
  fsw/src/telecom_serial.c
  fsw/src/telecom_lora_tlm.c)
else() 
  list(APPEND APP_SRC_FILES
  fsw/src/telecom_ip_tlm.c)
endif(COMMUNICATION_LORA)

add_cfe_app(telecom ${APP_SRC_FILES})

target_include_directories(telecom PUBLIC fsw/inc)

if (COMMUNICATION_LORA)
  target_compile_definitions(telecom PRIVATE COMMUNICATION_LORA)
endif(COMMUNICATION_LORA)

