qt_add_executable(charging-server
  main.cpp Database.cpp Database.h Service.cpp Service.h Accounts.cpp Stations.cpp
  Orders.cpp Statistics.cpp HttpServer.cpp HttpServer.h resources.qrc
)
target_link_libraries(charging-server PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql)
target_compile_definitions(charging-server PRIVATE CHARGING_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
