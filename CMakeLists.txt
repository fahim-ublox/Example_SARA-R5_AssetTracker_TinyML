cmake_minimum_required(VERSION 3.16)
project(target-cortexm LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE ON)
include(CMakePrintHelpers)

option(USE_RTOS "Enable RTOS support" OFF)

message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

add_compile_options(
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    $<$<CONFIG:DEBUG>:-g3>
    $<$<CONFIG:DEBUG>:-Og>
    $<$<CONFIG:RELEASE>:-O3>  
)

add_compile_definitions(
  $<$<CONFIG:DEBUG>:DEBUG>
  $<$<BOOL:${USE_RTOS}>:RTOS>
)

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/ubxlib)
set(UBXLIB_BASE ${CMAKE_SOURCE_DIR}/ubxlib)
# Set the ubxlib features to compile (all needs to be enabled at the moment)
set(UBXLIB_FEATURES short_range cell gnss)

# From this line we will get back:
# - UBXLIB_SRC
# - UBXLIB_INC
# - UBXLIB_PRIVATE_INC
# - UBXLIB_TEST_SRC
# - UBXLIB_TEST_INC
include(${UBXLIB_BASE}/port/ubxlib.cmake)

message(STATUS "Including /port/ubxlib.cmake")

set(PLATFORM_PATH ${UBXLIB_BASE}/port/platform/stm32cube)
set(FREERTOS_PATH  ${CMAKE_SOURCE_DIR}/FreeRTOS)
set(MBEDTLS_PATH ${CMAKE_SOURCE_DIR}/mbedTLS)

# Ubxlib port
set (UBXLIB_SRC_PLATFORM 
	${UBXLIB_BASE}/port/clib/u_port_clib_mktime64.c  
	${UBXLIB_BASE}/port/platform/common/mbedtls/u_port_crypto.c 
	${PLATFORM_PATH}/src/u_port_debug.c 
	${PLATFORM_PATH}/src/u_port_gpio.c 
	${PLATFORM_PATH}/src/u_port_os.c 
	${PLATFORM_PATH}/src/u_port_private.c 
	${PLATFORM_PATH}/src/u_port_uart.c 
	${PLATFORM_PATH}/src/u_port_i2c.c 
	${PLATFORM_PATH}/src/u_port.c 
	${UBXLIB_BASE}/port/platform/common/heap_check/u_heap_check.c 
	${PLATFORM_PATH}/src/heap_useNewlib.c
	)

#set(UBXLIB_ASM ${PLATFORM_PATH}/src/startup_stm32f437vgtx.s)

set (UBXLIB_INC_PLATFORM 
	${UBXLIB_PRIVATE_INC}
	${UBXLIB_BASE}/port/clib 
	${PLATFORM_PATH}/src 
	${PLATFORM_PATH}/mcu/stm32f4/cfg
	${UBXLIB_BASE}/port/platform/common/heap_check
	${UBXLIB_BASE}/port/platform/common/debug_utils/api
	${PLATFORM_PATH}
	${UBXLIB_BASE}/common/error/api
	${UBXLIB_BASE}/common/device/api
	${UBXLIB_BASE}/common/assert/api
	${UBXLIB_BASE}/common/location/api
	${UBXLIB_BASE}/common/at_client/api
	${UBXLIB_BASE}/common/security/api
	${UBXLIB_BASE}/common/sock/api
	${UBXLIB_BASE}/common/spartn/api
	${UBXLIB_BASE}/common/short_range/api
	${UBXLIB_BASE}/common/mqtt_client/api
	${UBXLIB_BASE}/common/network/api
	${UBXLIB_BASE}/common/utils/api
	${UBXLIB_BASE}/common/ubx_protocol/api
	${UBXLIB_BASE}/cell/api
	${UBXLIB_BASE}/gnss/api
	${UBXLIB_BASE}/ble/api
	${UBXLIB_BASE}/port/api
	${UBXLIB_BASE}/wifi/api
	${UBXLIB_BASE}/cfg
	) 

# FreeRTOS
set(FreeRTOS_FW_SRC
	${FREERTOS_PATH}/Source/croutine.c 
	${FREERTOS_PATH}/Source/list.c 
	${FREERTOS_PATH}/Source/queue.c 
	${FREERTOS_PATH}/Source/tasks.c 
	${FREERTOS_PATH}/Source/timers.c 
	${FREERTOS_PATH}/Source/CMSIS_RTOS/cmsis_os.c 
	${FREERTOS_PATH}/Source/portable/GCC/ARM_CM4F/port.c
	)

set(FreeRTOS_FW_INC 
	${FREERTOS_PATH}/Source/include 
	${FREERTOS_PATH}/Source/portable/GCC/ARM_CM4F 
	${FREERTOS_PATH}/Source/CMSIS_RTOS
	)

# mbedTLS
set(mbedTLS_FW_SRC
	${MBEDTLS_PATH}/library/sha256.c 
	${MBEDTLS_PATH}/library/md.c 
	${MBEDTLS_PATH}/library/md_wrap.c 
	${MBEDTLS_PATH}/library/md5.c 
	${MBEDTLS_PATH}/library/aes.c 
	${MBEDTLS_PATH}/library/platform_util.c
	)

set(mbedTLS_FW_INC ${MBEDTLS_PATH}/include)
endif()

add_executable(Application 
    ${CMAKE_SOURCE_DIR}/Core/Src/main.cpp
    ${CMAKE_SOURCE_DIR}/Core/Src/model_tflite.cc
	${CMAKE_SOURCE_DIR}/Core/Src/debug_log.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
    ${CMAKE_SOURCE_DIR}/Core/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/Core/Src/sysmem.c
    ${CMAKE_SOURCE_DIR}/Core/Src/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Core/Startup/startup_stm32f437vgtx.s
	${UBXLIB_SRC} ${UBXLIB_SRC_PLATFORM} ${FreeRTOS_FW_SRC} ${mbedTLS_FW_SRC}
)

target_include_directories(Application INTERFACE ${UBXLIB_INC} ${FreeRTOS_FW_INC} ${mbedTLS_FW_INC})

target_include_directories(Application PRIVATE
  ${CMAKE_SOURCE_DIR}/Core/Src
  ${CMAKE_SOURCE_DIR}/Core/Inc
  ${UBXLIB_PRIVATE_INC} ${UBXLIB_INC_PLATFORM} ${FreeRTOS_FW_INC} ${mbedTLS_FW_INC}
)

set(LINKER_SCRIPTS 
    ${CMAKE_SOURCE_DIR}/STM32F437VGTX_FLASH.ld
    #${CMAKE_SOURCE_DIR}/STM32F437VGTX_RAM.ld
)

set_target_properties(Application PROPERTIES
    SUFFIX .elf
    LINK_DEPENDS "${LINKER_SCRIPTS}"
)

target_link_options(Application PRIVATE
  -T${CMAKE_SOURCE_DIR}/STM32F437VGTX_FLASH.ld
  #-T${CMAKE_SOURCE_DIR}/STM32F437VGTX_RAM.ld
  LINKER:--print-memory-usage
  LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/Application.map
)

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/Drivers)
  add_subdirectory(${CMAKE_SOURCE_DIR}/Drivers)
  target_link_libraries(Application PRIVATE Drivers)
endif()

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/tensorflow)
  add_subdirectory(${CMAKE_SOURCE_DIR}/tensorflow)
  target_link_libraries(Application PRIVATE tensorflow)
endif()

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/ubxlib)
#  add_subdirectory(${CMAKE_SOURCE_DIR}/ubxlib)
#  target_link_libraries(Application PRIVATE ubxlib)
endif()


if (EXISTS ${TOOLCHAIN_COPY})
  add_custom_command(
    TARGET Application
    POST_BUILD
    COMMAND ${TOOLCHAIN_COPY} -O ihex $<TARGET_FILE:Application> 
            ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_NAME:Application>.hex
  )
else()
  message(STATUS "'objcopy' not found: cannot generate .hex file")
endif()

if (EXISTS ${TOOLCHAIN_SIZE})
  add_custom_command(
    TARGET Application
    POST_BUILD
    COMMAND ${TOOLCHAIN_SIZE} $<TARGET_FILE:Application> 
  )
else()
  message(STATUS "'objcopy' not found: cannot generate .hex file")
endif()

if (EXISTS "${TOOLCHAIN_SIZE}")
  add_custom_command(
    TARGET Application
    POST_BUILD
    COMMAND ${TOOLCHAIN_SIZE} --format=berkeley $<TARGET_FILE:Application> 
            >${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_NAME:Application>.bsz
  )

  add_custom_command(
    TARGET Application
    POST_BUILD
    COMMAND ${TOOLCHAIN_SIZE} --format=sysv -x $<TARGET_FILE:Application> 
            >${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_NAME:Application>.ssz
  )
else()
  message(STATUS "'size' not found: cannot generate .[bs]sz files")
endif()
