cmake_minimum_required(VERSION 3.11)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

set(POW_SRC
    beamHash.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/crypto/equihashR_impl.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/arith_uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/utilstrencodings.cpp
)

if(UNIX)
    set_source_files_properties(impl/crypto/equihashR_impl.cpp PROPERTIES COMPILE_FLAGS -O2)
endif()

add_library(pow STATIC ${POW_SRC})

target_compile_definitions(pow PRIVATE ENABLE_MINING)

target_link_libraries(pow PRIVATE Boost::boost blake2b)

add_library(external_pow STATIC
    external_pow_stub.cpp
    stratum.cpp
    stratum_server.cpp
)

target_link_libraries(external_pow PRIVATE p2p pow core OpenSSL::SSL)

if (NOT ANDROID AND NOT IOS)
    find_package(Boost REQUIRED program_options)
    add_executable(miner_client miner_client.cpp)
    target_link_libraries(miner_client PRIVATE external_pow Boost::boost Boost::program_options)
  #  add_executable(miner_client miner_client.cpp ../core/block_crypt.cpp) # ???????????????????????????
  #  if (Boost_FOUND)
 #       find_package(Boost COMPONENTS program_options)
  #      add_dependencies(miner_client external_pow  Boost::program_options)
  #      target_link_libraries(miner_client external_pow Boost::program_options)
  #  else()
  #      add_dependencies(miner_client external_pow boost_program_options)
  #      target_link_libraries(miner_client external_pow boost_program_options)
  #  endif()

    add_subdirectory(unittests)
endif()