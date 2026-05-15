include_guard()
cmake_minimum_required(VERSION 3.14)

project(
  HyperliquidAppFuzzer
  VERSION 1.0
  DESCRIPTION "Ledger Hyperliquid App Fuzzer"
  LANGUAGES C)

if(NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK must be defined")
endif()

include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
ledger_fuzz_setup()

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)

file(GLOB_RECURSE C_SOURCES
  "${APP_SOURCE_DIR}/src/*.c"
  "${APP_SOURCE_DIR}/src_msgpack/*.c"
  "${CMAKE_SOURCE_DIR}/mock/*.c"
)
list(REMOVE_ITEM C_SOURCES "${APP_SOURCE_DIR}/src/app_main.c")

# Hyperliquid provides its own DER-aware tlv_custom_mutate() so we do NOT
# link ${LEDGER_FUZZ_TLV_MUTATOR_SOURCE} (which would short-form-encode every
# tag and confuse lib_tlv's DER parser). See harness/fuzz_tlv_config.c.
set(FUZZ_SUPPORT_SOURCES
    "${CMAKE_SOURCE_DIR}/harness/fuzz_tlv_config.c"
)

ledger_fuzz_add_app_target(
    SOURCES
        ${C_SOURCES}
        ${FUZZ_SUPPORT_SOURCES}
    INCLUDE_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/harness/
        ${APP_SOURCE_DIR}/src/
        ${APP_SOURCE_DIR}/src/apdu/
        ${APP_SOURCE_DIR}/src/handler/
        ${APP_SOURCE_DIR}/src/ui/
        ${APP_SOURCE_DIR}/src_msgpack/
        ${CMAKE_SOURCE_DIR}/mock/
        ${CMAKE_SOURCE_DIR}
    COMPILE_DEFINITIONS
        APPNAME="Hyperliquid"
)
