file(GLOB HEADERS "include/eosio/monitor_api_plugin/*.hpp")
add_library( monitor_api_plugin
             monitor_api_plugin.cpp
             monitor_api_plugin_impl.cpp
             httpc.cpp
             ${HEADERS} )

target_link_libraries( monitor_api_plugin appbase fc http_plugin )
target_include_directories( monitor_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
