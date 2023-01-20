project(antler-run)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/antler-run.cpp ${CMAKE_BINARY_DIR}/antler-run.cpp)
find_package(eos-vm)

set(TOOLS_DIR ${CMAKE_SOURCE_DIR})
set(LIB_DIR ${CMAKE_SOURCE_DIR}/../libraries)
set(CDT_NO_START TRUE)

include_directories(${LIB_DIR})

add_tool(antler-run)
target_compile_options(${PROJECT_NAME} PUBLIC -ldl -fpermissive)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND APPLE)
    target_compile_options( ${TARGET} PUBLIC -g )
    find_program ( name NAMES "dsymutil" )
    if ( name )
        add_custom_command( TARGET ${TARGET} POST_BUILD COMMAND dsymutil ${BINOUTPUT}/${TARGET} )
    endif()
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,\"\\$ORIGIN/../lib\"")

target_include_directories(${PROJECT_NAME} PUBLIC ${TOOLS_DIR}/external/eos-vm/include
                                                  ${LIB_DIR}/eosiolib/contracts
                                                  ${LIB_DIR}/eosiolib/core
                                                  ${LIB_DIR}/eosiolib/capi
                                                  ${LIB_DIR}/eosiolib/native
                                                  ${LIB_DIR}/native
                                                  ${LIB_DIR}/meta_refl/include
                                                  ${LIB_DIR}/boost/include)
target_link_libraries(${PROJECT_NAME} native_eosio native)

# target_link_libraries(${PROJECT_NAME} eos-vm EOSIOLIB)