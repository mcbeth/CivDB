cmake_minimum_required(VERSION 2.6)

PROJECT(db)

FIND_PACKAGE(Boost 1.37 COMPONENTS serialization system)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

SET(db_SRCS db.cpp dbUtils.cpp)
SET (boost_SRCS /usr/share/doc/libboost1.37-dev/examples/random_device.cpp)

ADD_LIBRARY(civdb STATIC ${db_SRCS})
LINK_DIRECTORIES(civdb)

#ADD_EXECUTABLE(create create.cpp)
#TARGET_LINK_LIBRARIES(create ${boost_LIBS} civdb)

#ADD_EXECUTABLE(draw draw.cpp)
#TARGET_LINK_LIBRARIES(draw ${boost_LIBS} civdb)

#ADD_EXECUTABLE(held held.cpp)
#TARGET_LINK_LIBRARIES(held ${boost_LIBS} civdb)

#ADD_EXECUTABLE(discard discard.cpp)
#TARGET_LINK_LIBRARIES(discard ${boost_LIBS} civdb)

#ADD_EXECUTABLE(export export.cpp)
#TARGET_LINK_LIBRARIES(export ${boost_LIBS} civdb)

#ADD_EXECUTABLE(setplayer setplayer.cpp)
#TARGET_LINK_LIBRARIES(setplayer ${boost_LIBS} civdb)

ADD_EXECUTABLE(shell shell.cpp ${boost_SRCS})
TARGET_LINK_LIBRARIES(shell ${Boost_LIBRARIES} civdb readline)

SET(CMAKE_BUILD_TYPE Debug)
