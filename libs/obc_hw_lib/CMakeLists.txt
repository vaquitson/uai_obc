project(OBC_HW_LIB C)

set(LINUX_SRC_DIR PRIVATE fsw/src/linux)

# Create the app module
add_cfe_app(obc_hw_lib 
  fsw/src/obc_hw_lib.c
)

target_include_directories(obc_hw_lib PUBLIC fsw/public_inc)

if(OBC_PLATFORM_LINUX)
  target_compile_definitions(obc_hw_lib PRIVATE OBC_PLATFORM_LINUX=1)
  target_include_directories(obc_hw_lib PRIVATE fsw/src/linux)
endif(OBC_PLATFORM_LINUX)

target_sources(obc_hw_lib PRIVATE 
  ${LINUX_SRC_DIR}/narwal_thermal_zones.c
  ${LINUX_SRC_DIR}/narwal_ram.c
)
