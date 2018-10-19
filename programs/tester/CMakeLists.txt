add_executable( ${TEST_STORE_EXECUTABLE_NAME} main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling ${TEST_STORE_EXECUTABLE_NAME} with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_link_libraries( ${TEST_STORE_EXECUTABLE_NAME}
        PRIVATE appbase
        PRIVATE creator_plugin
        PRIVATE http_plugin
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories(${TEST_STORE_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

mas_sign(${TEST_STORE_EXECUTABLE_NAME})

install( TARGETS
   ${TEST_STORE_EXECUTABLE_NAME}

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
