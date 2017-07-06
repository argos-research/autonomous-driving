cmake_minimum_required (VERSION 2.6)
project(SimCoupler)

find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

if(PROTOBUF_FOUND)
  include_directories(${PROTOBUF_INCLUDE_DIRS})
  PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS protobuf-messages/state.proto protobuf-messages/vehicle.proto protobuf-messages/position.proto protobuf-messages/setup.proto)
endif()

if(BOOST_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set (CMAKE_CXX_STANDARD 11)

# VREP
# FIXME adjust VREP_PATH if necessary
add_library(libvrep STATIC "$ENV{VREP_PATH}/programming/remoteApi/extApi.c" "$ENV{VREP_PATH}/programming/remoteApi/extApiPlatform.c")
set_target_properties(libvrep PROPERTIES COMPILE_FLAGS "-DNON_MATLAB_PARSING -DMAX_EXT_API_CONNECTIONS=255 -D__linux")
target_include_directories(libvrep PUBLIC $ENV{VREP_PATH}/programming/remoteApi)
target_include_directories(libvrep PUBLIC $ENV{VREP_PATH}/programming/include)

# SimCoupler
link_directories($ENV{SUMO_HOME}/src/utils/traci $ENV{SUMO_HOME}/src/foreign/tcpip $ENV{SUMO_HOME}/src/utils/common)
include_directories($ENV{SUMO_HOME}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # protobuf *.o files
include_directories(inc)
AUX_SOURCE_DIRECTORY(src SRC_CC)
add_executable(main "$ENV{VREP_PATH}/programming/remoteApi/extApi.c" "$ENV{VREP_PATH}/programming/remoteApi/extApiPlatform.c" ${SRC_VREP} ${SRC_CC} ${PROTO_SRCS} ${PROTO_HDRS})
set_target_properties(main PROPERTIES COMPILE_FLAGS "-DNON_MATLAB_PARSING -DMAX_EXT_API_CONNECTIONS=255 -D__linux")
# includes
target_include_directories(main PUBLIC $ENV{VREP_PATH}/programming/remoteApi $ENV{VREP_PATH}/programming/include) # VREP

# -------
# linking
# -------
target_link_libraries(main libvrep) # VREP
target_link_libraries(main common traci tcpip protobuf) # SUMO
target_link_libraries(main ${PROTOBUF_LIBRARIES}) # Protobuf
target_link_libraries(main ${Boost_LIBRARIES}) # Boost
