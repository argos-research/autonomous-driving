INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "human")
SET(ROBOT_INTERFACE WELCOME)
SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp)

ADD_SDLIB_INCLUDEDIR(robottools tgfclient sensors)
# include build files of our protobuf messages
IF(OPTION_PARKING)
  FILE(GLOB PROTO ../../../../../tools/protobuf/*.proto)
  PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO})
ENDIF(OPTION_PARKING)

IF(OPTION_PARKING)
    # path where the protobuf generated *.pb.h files are stored
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ENDIF(OPTION_PARKING)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 1.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
           SOURCES ${ROBOT_SOURCES} ${PROTO_SRCS})

IF(OPTION_PARKING)
    # Add protobuf includes and library
    ADD_PROTOBUF_INCLUDEDIR()
    ADD_PROTOBUF_LIBRARY(${ROBOT_NAME})
ENDIF(OPTION_PARKING)

ADD_SDLIB_LIBRARY(${ROBOT_NAME} robottools tgfclient sensors)

# For data associated to the robot module, see data/drivers/human/CMakeLists.txt
