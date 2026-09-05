add_library(charging-client STATIC ApiClient.cpp ApiClient.h)
target_include_directories(charging-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(charging-client PUBLIC Qt6::Core Qt6::Network)
