cmake_minimum_required(VERSION 3.22)
project(BinanceLogger)

set(CMAKE_CXX_STANDARD 20)

FIND_PACKAGE( Boost 1.75 COMPONENTS program_options json REQUIRED )
FIND_PACKAGE( OpenSSL REQUIRED )
#find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
#link_libraries(${JSONCPP_LIBRARIES})
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_executable(BinanceLogger src/main.cpp src/colormod.h src/main.h src/main.h)
TARGET_LINK_LIBRARIES( BinanceLogger LINK_PUBLIC ${Boost_LIBRARIES} )
TARGET_LINK_LIBRARIES( BinanceLogger LINK_PUBLIC OpenSSL::SSL)
#TARGET_LINK_LIBRARIES(U3 LINK_PUBLIC ${JSONCPP_LIBRARIES})