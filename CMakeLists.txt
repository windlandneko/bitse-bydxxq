cmake_minimum_required(VERSION 3.16)
project(charging_platform VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_USER_APP "Build the Qt Quick mobile user application" ON)
option(BUILD_ADMIN_APP "Build the Qt Widgets operations application" ON)
option(BUILD_SERVER "Build the unified C++/Qt service" ON)
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network)
include(CTest)
add_subdirectory(shared)
if(BUILD_SERVER)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Gui Sql)
  add_subdirectory(apps/server)
endif()
if(BUILD_USER_APP OR BUILD_ADMIN_APP)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
endif()
if(BUILD_USER_APP)
  add_subdirectory(apps/user-app)
endif()
if(BUILD_ADMIN_APP)
  add_subdirectory(apps/admin-app)
endif()
add_custom_target(format
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_custom_target(format-check
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh" --check
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


if(BUILD_TESTING AND BUILD_SERVER)
  find_program(UV_EXECUTABLE uv)
  if(UV_EXECUTABLE)
    add_test(NAME service-integration
      COMMAND ${CMAKE_COMMAND} -E env CHARGING_SERVER_BIN=$<TARGET_FILE:charging-server>
      ${UV_EXECUTABLE} run --project ${PROJECT_SOURCE_DIR}/ml pytest ${PROJECT_SOURCE_DIR}/tests -q)
    set_tests_properties(service-integration PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} TIMEOUT 180)
    add_test(NAME forecast-unit COMMAND ${UV_EXECUTABLE} run --project ${PROJECT_SOURCE_DIR}/ml
      pytest ${PROJECT_SOURCE_DIR}/ml/tests -q)
    set_tests_properties(forecast-unit PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} TIMEOUT 180)
  endif()
endif()
