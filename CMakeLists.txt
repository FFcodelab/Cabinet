cmake_minimum_required(VERSION 3.16)
project(CabinetController VERSION 1.0.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(CTest)
add_library(cabinet_core STATIC
    src/protocol/checksum.c
    src/protocol/modbus.c
    src/protocol/stream.c
    src/domain/sample.c
    src/domain/interlock.c
    src/domain/power_allocator.c
    src/domain/exchange.c
    src/storage/config_codec.c
    src/storage/config_store.c
    src/services/client.c
    src/services/event_queue.c
    src/services/fault_journal.c
    src/services/image_update.c
    src/app/controller.c)
target_include_directories(cabinet_core PUBLIC include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cabinet_core PRIVATE -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow)
endif()
add_library(cabinet_host STATIC ports/host/host_port.c)
target_include_directories(cabinet_host PUBLIC ports/host)
target_link_libraries(cabinet_host PUBLIC cabinet_core)
add_executable(cabinet_sim apps/simulator/main.c apps/simulator/report.c)
target_link_libraries(cabinet_sim PRIVATE cabinet_host)
if(BUILD_TESTING)
    foreach(suite protocol domain storage_update controller)
        add_executable(test_${suite} tests/test_${suite}.c)
        target_link_libraries(test_${suite} PRIVATE cabinet_host)
        if(MSVC)
            target_compile_options(test_${suite} PRIVATE /UNDEBUG)
        else()
            target_compile_options(test_${suite} PRIVATE -UNDEBUG)
        endif()
        add_test(NAME ${suite} COMMAND test_${suite})
    endforeach()
    add_test(NAME scenario_normal COMMAND cabinet_sim normal ${CMAKE_BINARY_DIR}/normal.json)
    add_test(NAME scenario_fault COMMAND cabinet_sim fault ${CMAKE_BINARY_DIR}/fault.json)
endif()
option(CABINET_FREERTOS_ADAPTER "Build adapter against caller-provided FreeRTOS target" OFF)
if(CABINET_FREERTOS_ADAPTER)
    if(NOT TARGET freertos_kernel)
        message(FATAL_ERROR "Provide freertos_kernel and its board configuration first")
    endif()
    add_library(cabinet_freertos STATIC ports/freertos/cabinet_task.c)
    target_include_directories(cabinet_freertos PUBLIC ports/freertos)
    target_link_libraries(cabinet_freertos PUBLIC cabinet_core freertos_kernel)
endif()
