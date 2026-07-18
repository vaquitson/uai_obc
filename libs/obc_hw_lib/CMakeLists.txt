project(OBC_HW_LIIB C)

# Create the app module
add_cfe_app(obc_hw_lib 
  fsw/src/obc_hw_lib.c
)

target_include_directories(obc_hw_lib PUBLIC fsw/public_inc)

if(OBC_PLATFORM_LINUX)
  target_include_directories(obc_hw_lib PRIVATE fsw/src/linux)
  target_compile_definitions(obc_hw_lib PRIVATE OBC_PLATFORM_LINUX=1)
endif(OBC_PLATFORM_LINUX)

target_sources(obc_hw_lib PRIVATE fsw/src/linux/narwal_thermal_zones.c)
