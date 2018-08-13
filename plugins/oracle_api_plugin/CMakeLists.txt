file(GLOB HEADERS "include/eosio/oracle_api_plugin/*.hpp")
add_library( oracle_api_plugin
             oracle_api_plugin.cpp
             oracle_api_plugin_impl.cpp
             ${HEADERS} )

target_link_libraries( oracle_api_plugin appbase fc http_plugin wallet_plugin )
target_include_directories( oracle_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
