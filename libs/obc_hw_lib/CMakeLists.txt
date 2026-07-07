project(OBC_HW_LIIB C)

# Create the app module
add_cfe_app(obc_hw_lib 
  fsw/src/obc_hw_lib.c
)

target_include_directories(obc_hw_lib PUBLIC fsw/public_inc)


