# set CMAKE_SYSTEM_NAME to define build as CMAKE_CROSSCOMPILING
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION Cortex-M4-STM32F437)
set(CMAKE_SYSTEM_PROCESSOR arm)

# setting either of the compilers builds the absolute paths for the other tools:
#   ar, nm, objcopy, objdump, ranlib, readelf -- but not as, ld, size
# if the compiler cannot be found the try_compile() function will fail the build
# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

find_program(CROSS_GCC_PATH "arm-none-eabi-gcc.exe")
get_filename_component(TOOLCHAIN ${CROSS_GCC_PATH} PATH)

set(CMAKE_C_COMPILER ${TOOLCHAIN}/arm-none-eabi-gcc.exe)
set(CMAKE_Cxx_COMPILER ${TOOLCHAIN}/arm-none-eabi-g++.exe)
set(TOOLCHAIN_AS ${TOOLCHAIN}/arm-none-eabi-as.exe CACHE STRING "arm-none-eabi-as")
set(TOOLCHAIN_LD ${TOOLCHAIN}/arm-none-eabi-ld.exe CACHE STRING "arm-none-eabi-ld")
set(TOOLCHAIN_SIZE ${TOOLCHAIN}/arm-none-eabi-size.exe CACHE STRING "arm-none-eabi-size")
set(TOOLCHAIN_COPY ${TOOLCHAIN}/arm-none-eabi-objcopy.exe CACHE STRING "arm-none-eabi-objcopy")

# --specs=nano.specs is both a compiler and linker option
set(ARM_OPTIONS -std=gnu11 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

add_compile_options(
  ${ARM_OPTIONS}
  -fmessage-length=0
  -funsigned-char
  -ffunction-sections
  -fdata-sections
  -fstack-usage
  -fno-rtti
  -fno-exceptions
  -g3 -O0 -Wall
  -MMD
  -MP)

add_compile_definitions(
  STM32F437xx
  USE_HAL_DRIVER
  USE_FULL_LL_DRIVER
#  HAL_DRIVERS_ONLY
  USE_FULL_ASSERT
  CMSIS_NN
  MBEDTLS_USER_CONFIG_FILE=\"user_mbedtls_config.h\"
  $<$<CONFIG:DEBUG>:OS_USE_TRACE_SEMIHOSTING_STDOUT>
  $<$<CONFIG:DEBUG>:OS_USE_SEMIHOSTING>
)

# use this to avoid running the linker during test compilation
# set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# use these options to verify the linker can create an ELF file
# when not doing a static link

add_link_options(
  ${ARM_OPTIONS}
  -Wl,--wrap=malloc -Wl,--wrap=_malloc_r -Wl,--wrap=calloc -Wl,--wrap=_calloc_r -Wl,--wrap=realloc -Wl,--wrap=_realloc_r
#  $<$<CONFIG:DEBUG>:--specs=rdimon.specs>
#  $<$<CONFIG:RELEASE>:--specs=nosys.specs>
  $<$<CONFIG:DEBUG>:-u_printf_float>
  $<$<CONFIG:DEBUG>:-u_scanf_float>
  --specs=nosys.specs
  --specs=nano.specs
#  -nostartfiles
  LINKER:--gc-sections
  LINKER:--build-id)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
