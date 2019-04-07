cmake_minimum_required(VERSION 3.2)

project(lsmonitor VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

configure_file(version.h.in version.h)

find_program(CONAN conan)
if(NOT EXISTS ${CONAN})
  message(FATAL_ERROR "conan not found.")
endif()

set(CONANFILE ${CMAKE_SOURCE_DIR}/conanfile.txt)
set(CONANFILE_CMAKE ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
execute_process(
  COMMAND ${CONAN} install --update --remote conan-center --build=missing -s os=${CMAKE_HOST_SYSTEM_NAME} -s compiler.libcxx=libstdc++11 ${CMAKE_SOURCE_DIR} RESULT_VARIABLE CONAN_RETURN_CODE
  )

if(NOT ${CONAN_RETURN_CODE} EQUAL 0)
  message(FATAL_ERROR "conan install command failed.")
endif()


include(${CONANFILE_CMAKE})
conan_basic_setup(TARGETS)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lspredicate
  ${CMAKE_CURRENT_SOURCE_DIR}/lsmonitor
  ${CMAKE_CURRENT_SOURCE_DIR}/lsprobe
  ${CMAKE_CURRENT_SOURCE_DIR}/stlab
  ${CONAN_INCLUDE_DIRS_ARGH}
  ${CONAN_INCLUDE_DIRS_SPDLOG}
  ${CONAN_INCLUDE_DIRS_FMT}
  ${CONAN_INCLUDE_DIRS_BOOST}
  )

link_directories(
  ${CONAN_LIB_DIRS}
  ${CONAN_LIB_DIRS_BOOST}
  )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/stlab)

add_library(lspredicate STATIC
  lspredicate/printer.cpp
  lspredicate/cmdl_expression.cpp
  )

add_library(file_event STATIC
  file_event/fanotify_event.cpp
  file_event/fanotify_reader.cpp
  file_event/lsprobe_event.cpp
  file_event/lsprobe_reader.cpp
  )

set_target_properties(lspredicate file_event PROPERTIES
  COMPILE_OPTIONS "-std=c++17;-Wpedantic;-Wall;-Wextra"
  )

add_executable(lsmonitor
  lsmonitor/main.cpp
  lsmonitor/utility.cpp
  lsmonitor/broadcast.cpp
  )

set_target_properties(lsmonitor PROPERTIES
  COMPILE_OPTIONS "-std=c++17;-Wpedantic;-Wall;-Wextra"
  LINK_FLAGS "-static-libstdc++"
  )

target_link_libraries(lsmonitor lspredicate file_event pthread stdc++fs ${CONAN_LIBS_BOOST})


install(TARGETS lsmonitor
  RUNTIME DESTINATION bin
  )

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_CONTACT "ad.beregovoy@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A toy Linux security events monitor.")
set(CPACK_PACKAGE_DESCRIPTION "An utility that reads data exposed by lsprobe, Linux security events probe.")

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_SECTION "Development")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Anton Beregovoy <${CPACK_PACKAGE_CONTACT}>")

include(CPack)

