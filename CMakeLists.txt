cmake_minimum_required(VERSION 3.16)
project(OrcStream VERSION 0.1.0 LANGUAGES C)

option(ORCSTREAM_BUILD_TESTS "Build OrcStream API and wire protocol tests" ON)
option(ORCSTREAM_BUILD_EXAMPLES "Build standalone IQ capture/replay examples" ON)

add_library(orcstream STATIC src/orcstream.c)
add_library(OrcStream::orcstream ALIAS orcstream)
target_include_directories(orcstream PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(orcstream PUBLIC c_std_99)
set_target_properties(orcstream PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
if(MSVC)
  target_compile_options(orcstream PRIVATE /W4)
else()
  target_compile_options(orcstream PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

include(GNUInstallDirs)
install(TARGETS orcstream ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/orcstream DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(ORCSTREAM_BUILD_TESTS)
  enable_testing()
  add_executable(orcstream_tests tests/test_orcstream.c)
  target_link_libraries(orcstream_tests PRIVATE OrcStream::orcstream)
  add_test(NAME orcstream_unit COMMAND orcstream_tests)
endif()

if(ORCSTREAM_BUILD_EXAMPLES)
  add_executable(orcstream_demo examples/iq_file_demo.c)
  target_link_libraries(orcstream_demo PRIVATE OrcStream::orcstream)
  add_executable(orcstream_udp_demo examples/udp_loopback.c)
  target_link_libraries(orcstream_udp_demo PRIVATE OrcStream::orcstream)
  if(WIN32)
    target_link_libraries(orcstream_udp_demo PRIVATE ws2_32)
  endif()
  if(ORCSTREAM_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
      add_test(NAME orcstream_udp_loopback
               COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/udp_loopback.py
                       $<TARGET_FILE:orcstream_udp_demo>)
    endif()
    add_test(NAME orcstream_file_roundtrip
             COMMAND ${CMAKE_COMMAND} -DDEMO=$<TARGET_FILE:orcstream_demo>
             -DOUT=${CMAKE_CURRENT_BINARY_DIR}/synthetic_iq.orcs
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/file_roundtrip.cmake)
  endif()
endif()
