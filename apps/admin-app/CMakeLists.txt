set(ADMIN_SOURCES
  AdminMainWindow.cpp
  AdminOverview.cpp
  AdminStations.cpp
  AdminPeople.cpp
  AdminForecasts.cpp
  AdminUi.cpp
  AdminMainWindow.h
  AdminWindowState.h
  AdminUi.h
)
qt_add_executable(charging-admin main.cpp ${ADMIN_SOURCES})
find_package(Qt6 6.2 REQUIRED COMPONENTS Charts)
target_link_libraries(charging-admin PRIVATE charging-client Qt6::Widgets Qt6::Charts)

if(BUILD_TESTING)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
  qt_add_executable(admin-ui-tests tests/AdminUiTest.cpp ${ADMIN_SOURCES})
  target_include_directories(admin-ui-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(admin-ui-tests PRIVATE charging-client Qt6::Widgets Qt6::Charts Qt6::Test)
  add_test(NAME admin-ui COMMAND admin-ui-tests)
  set_tests_properties(admin-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
