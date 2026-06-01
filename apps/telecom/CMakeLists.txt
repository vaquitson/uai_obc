project(CFE_TELECOM C)

set(APP_SRC_FILES
    fsw/src/telecom_app.c
)


add_cfe_app(telecom ${APP_SRC_FILES})

target_include_directories(telecom PUBLIC fsw/inc)
