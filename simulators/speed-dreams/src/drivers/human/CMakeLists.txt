INCLUDE(../../../cmake/macros.cmake)

SET(ROBOT_NAME "human")
SET(ROBOT_INTERFACE WELCOME)
SET(ROBOT_SOURCES ${ROBOT_NAME}.cpp)

ADD_SDLIB_INCLUDEDIR(robottools tgfclient sensors)
# include build files of our protobuf messages
INCLUDE_DIRECTORIES(../../../../../tools/protobuf/build)
FILE(GLOB PROTO ../../../../../tools/protobuf/build/*.cc)

ROBOT_MODULE(NAME ${ROBOT_NAME} VERSION 1.0.0 SOVERSION 1.0.0
             INTERFACE ${ROBOT_INTERFACE}
           SOURCES ${ROBOT_SOURCES} ${PROTO})

# Add protobuf includes and library
ADD_PROTOBUF_INCLUDEDIR()
ADD_PROTOBUF_LIBRARY(${ROBOT_NAME})

ADD_SDLIB_LIBRARY(${ROBOT_NAME} robottools tgfclient sensors)

# For data associated to the robot module, see data/drivers/human/CMakeLists.txt
