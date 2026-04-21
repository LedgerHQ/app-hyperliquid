cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
	      DESCRIPTION "Unit tests for Ledger Nano application"
        LANGUAGES C)


# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
add_definitions(-DPRINTF=)
# OS_UNIT_TEST makes PIC(x) a simple cast instead of a hardware call
add_compile_definitions(OS_UNIT_TEST)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -g -O0 --coverage")

set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

add_compile_definitions(TEST)

# ── Include directories ──────────────────────────────────────────────────────
# stubs/ must come first to intercept hardware-specific SDK headers
include_directories(stubs)
include_directories(.)
include_directories(../src)
include_directories(../src/transaction)
include_directories($ENV{BOLOS_SDK}/lib_standard_app)
include_directories($ENV{BOLOS_SDK}/include)
include_directories($ENV{BOLOS_SDK}/lib_tlv)
include_directories(../cmp)

# ── SDK shared libraries ─────────────────────────────────────────────────────
add_library(base58  SHARED $ENV{BOLOS_SDK}/lib_standard_app/base58.c)
add_library(bip32   SHARED $ENV{BOLOS_SDK}/lib_standard_app/bip32.c)
add_library(buffer  SHARED $ENV{BOLOS_SDK}/lib_standard_app/buffer.c)
add_library(read    SHARED $ENV{BOLOS_SDK}/lib_standard_app/read.c)
add_library(write   SHARED $ENV{BOLOS_SDK}/lib_standard_app/write.c)
add_library(format  SHARED $ENV{BOLOS_SDK}/lib_standard_app/format.c)
add_library(varint  SHARED $ENV{BOLOS_SDK}/lib_standard_app/varint.c)
add_library(apdu_parser SHARED $ENV{BOLOS_SDK}/lib_standard_app/parser.c)
add_library(stubs_os_utils SHARED stubs/os_utils_stubs.c)
add_library(tlv_library SHARED $ENV{BOLOS_SDK}/lib_tlv/tlv_library.c)
target_link_libraries(tlv_library PUBLIC stubs_os_utils)
add_library(cmp_lib SHARED ../cmp/cmp.c)

# ── App shared libraries ─────────────────────────────────────────────────────
add_library(hl_context     SHARED ../src/hl_context.c)
add_library(approve_builder_fee SHARED ../src/approve_builder_fee.c)
add_library(update_leverage SHARED ../src/update_leverage.c)
add_library(update_isolated_margin SHARED ../src/update_isolated_margin.c)
add_library(bulk_cancel    SHARED ../src/bulk_cancel.c)
add_library(order_request  SHARED ../src/order_request.c)
add_library(bulk_modify    SHARED ../src/bulk_modify.c)
add_library(bulk_order     SHARED ../src/bulk_order.c)
add_library(stubs_cx       SHARED stubs/cx_stubs.c)
add_library(stubs_eip712   SHARED stubs/eip712_stubs.c)
add_library(action         SHARED ../src/action.c)

# Common link set for tests that use TLV parsing
set(COMMON_LIBS buffer bip32 read write varint format tlv_library cmocka gcov)
set(APP_LIBS    hl_context cmp_lib)

# ── test_hl_context ──────────────────────────────────────────────────────────
add_executable(test_hl_context test_hl_context.c)
target_link_libraries(test_hl_context PUBLIC hl_context cmocka gcov)
add_test(test_hl_context test_hl_context)

# ── test_approve_builder_fee ─────────────────────────────────────────────────
add_executable(test_approve_builder_fee test_approve_builder_fee.c)
target_link_libraries(test_approve_builder_fee PUBLIC
    approve_builder_fee ${COMMON_LIBS})
add_test(test_approve_builder_fee test_approve_builder_fee)

# ── test_update_leverage ─────────────────────────────────────────────────────
add_executable(test_update_leverage test_update_leverage.c)
target_link_libraries(test_update_leverage PUBLIC
    update_leverage ${APP_LIBS} ${COMMON_LIBS})
add_test(test_update_leverage test_update_leverage)

# ── test_update_isolated_margin ──────────────────────────────────────────────
add_executable(test_update_isolated_margin test_update_isolated_margin.c)
target_link_libraries(test_update_isolated_margin PUBLIC
    update_isolated_margin ${APP_LIBS} ${COMMON_LIBS})
add_test(test_update_isolated_margin test_update_isolated_margin)

# ── test_bulk_cancel ─────────────────────────────────────────────────────────
add_executable(test_bulk_cancel test_bulk_cancel.c)
target_link_libraries(test_bulk_cancel PUBLIC
    bulk_cancel ${APP_LIBS} ${COMMON_LIBS})
add_test(test_bulk_cancel test_bulk_cancel)

# ── test_order_request ───────────────────────────────────────────────────────
add_executable(test_order_request test_order_request.c)
target_link_libraries(test_order_request PUBLIC
    order_request ${APP_LIBS} ${COMMON_LIBS})
add_test(test_order_request test_order_request)

# ── test_bulk_modify ─────────────────────────────────────────────────────────
add_executable(test_bulk_modify test_bulk_modify.c)
target_link_libraries(test_bulk_modify PUBLIC
    bulk_modify order_request ${APP_LIBS} ${COMMON_LIBS})
add_test(test_bulk_modify test_bulk_modify)

# ── test_bulk_order ──────────────────────────────────────────────────────────
add_executable(test_bulk_order test_bulk_order.c)
target_link_libraries(test_bulk_order PUBLIC
    bulk_order order_request stubs_os_utils ${APP_LIBS} ${COMMON_LIBS})
add_test(test_bulk_order test_bulk_order)

# ── test_action ──────────────────────────────────────────────────────────────
add_executable(test_action test_action.c)
target_link_libraries(test_action PUBLIC
    action bulk_cancel bulk_order bulk_modify order_request
    update_leverage update_isolated_margin approve_builder_fee
    stubs_cx stubs_eip712 stubs_os_utils ${APP_LIBS} ${COMMON_LIBS} write)
add_test(test_action test_action)
