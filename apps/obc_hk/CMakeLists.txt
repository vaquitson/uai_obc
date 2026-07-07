project(CFE_OBC_HK C)

set(APP_SRC_FILES
  fsw/src/obc_hk.c
)

add_cfe_app(obc_hk ${APP_SRC_FILES})

target_include_directories(obc_hk PUBLIC fsw/inc)

add_cfe_app_dependency(obc_hk obc_hw_lib)

