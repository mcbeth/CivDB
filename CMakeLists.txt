cmake_minimum_required(VERSION 2.6)

PROJECT(db)

FIND_PACKAGE(Boost 1.38 COMPONENTS serialization system)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SET(db_SRCS db.cpp dbUtils.cpp parser.cpp)
SET (boost_SRCS /usr/share/doc/libboost1.38-dev/examples/random_device.cpp)
SET(CMAKE_CXX_FLAGS "-std=gnu++0x -m32 -static-libgcc")

ADD_LIBRARY(civdb STATIC ${db_SRCS} ${Boost_LIBRARIES})
LINK_DIRECTORIES(civdb)

ADD_EXECUTABLE(shell shell.cpp ${boost_SRCS})
ADD_EXECUTABLE(value value.cpp ${boost_SRCS})
ADD_EXECUTABLE(listCards listCards.cpp ${boost_SRCS})
TARGET_LINK_LIBRARIES(shell civdb readline ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(value civdb ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(listCards civdb ${Boost_LIBRARIES})

SET(CMAKE_BUILD_TYPE Debug)
