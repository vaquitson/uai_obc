project(CFE_TELECOM C)

set(APP_SRC_FILES
    fsw/src/telecom_app.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/telecom_eds_encode.c)
else()
  list(APPEND APP_SRC_FILES,
  fsw/src/telecom_passthru_encode.c)
endif()

add_cfe_app(telecom ${APP_SRC_FILES})

target_include_directories(telecom PUBLIC fsw/inc)
