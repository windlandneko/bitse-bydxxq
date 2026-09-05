find_package(Qt6 6.2 REQUIRED COMPONENTS QuickWidgets QuickControls2 WebEngineWidgets)

qt_add_executable(charging-user
  main.cpp
  MobileController.cpp
  MobileController.h
  UserMainWindow.cpp
  UserMainWindow.h
  resources.qrc
)
target_link_libraries(charging-user PRIVATE
  charging-client
  Qt6::Widgets
  Qt6::QuickWidgets
  Qt6::QuickControls2
  Qt6::WebEngineWidgets
)

option(BUILD_MOBILE_FLOW_TESTS "Build mobile UI/service integration test" ${BUILD_TESTING})
if(BUILD_MOBILE_FLOW_TESTS)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
  qt_add_executable(charging-user-flow-test
    tests/MobileFlowTest.cpp
    MobileController.cpp
    MobileController.h
    UserMainWindow.cpp
    UserMainWindow.h
    resources.qrc
  )
  if(BUILD_TESTING AND BUILD_SERVER)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME mobile-flow
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/run_mobile_flow.py
        --server $<TARGET_FILE:charging-server>
        --client $<TARGET_FILE:charging-user-flow-test>
    )
    set_tests_properties(mobile-flow PROPERTIES TIMEOUT 100)
  endif()
  target_include_directories(charging-user-flow-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(charging-user-flow-test PRIVATE
    charging-client Qt6::Test Qt6::Widgets Qt6::QuickWidgets
    Qt6::QuickControls2 Qt6::WebEngineWidgets
  )
endif()

if(BUILD_TESTING)
  add_test(NAME mobile-qml-smoke
    COMMAND ${CMAKE_COMMAND} -E env
      QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=1
      CHARGING_SERVER_URL=http://127.0.0.1:1
      $<TARGET_FILE:charging-user> --smoke-test
  )
  set_tests_properties(mobile-qml-smoke PROPERTIES TIMEOUT 20)
endif()
