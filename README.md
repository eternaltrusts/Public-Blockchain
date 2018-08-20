# Public-Blockchain

To connect to your wallet you need to replace

target_link_libraries( keosd
        PRIVATE appbase
        PRIVATE wallet_api_plugin wallet_plugin
        PRIVATE http_plugin
        PRIVATE chain_plugin
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

on 

target_link_libraries( keosd
        PRIVATE appbase
        PRIVATE wallet_api_plugin wallet_plugin
        PRIVATE http_plugin
        PRIVATE chain_plugin
        PRIVATE -Wl,${whole_archive_flag} monitor_api_plugin        -Wl,${no_whole_archive_flag}
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )


