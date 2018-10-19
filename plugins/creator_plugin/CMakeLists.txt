file(GLOB HEADERS "include/eosio/creator_plugin/*.hpp")
add_library( creator_plugin
             creator_plugin.cpp
             creator_plugin_impl.cpp
             httpc.cpp
             ${HEADERS} )

target_link_libraries( creator_plugin appbase fc http_plugin chain_plugin wallet_plugin )
target_include_directories( creator_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
