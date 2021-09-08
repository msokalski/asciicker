cmake_minimum_required(VERSION 3.16)   

option(ASSUME_APPLE "Force compile for apple" OFF)
option(COMPILE_SERVER "Compile server" ON)
if(APPLE OR ASSUME_APPLE)
  option(COMPILE_ASCIICKER "Compile asciicker" OFF)
  option(COMPILE_ASCIICKER_TERM "Compile asciicker_term" OFF)
  option(COMPILE_ASCIIID "Compile asciiid" OFF)
  option(COMPILE_ASCIICKER_MAC "Compile asciicker_sdl mac" ON)
  option(COMPILE_ASCIICKER_TERM_MAC "Compile asciicker_term mac" ON)
  option(COMPILE_ASCIIID_MAC "Compile asciiid mac" ON)
else()
  option(COMPILE_ASCIICKER "Compile asciicker" ON)
  option(COMPILE_ASCIICKER_TERM "Compile asciicker_term" ON)
  option(COMPILE_ASCIIID "Compile asciiid" ON)
  option(COMPILE_ASCIICKER_MAC "Compile asciicker_sdl mac" OFF)
  option(COMPILE_ASCIICKER_TERM_MAC "Compile asciicker_term mac" OFF)
  option(COMPILE_ASCIIID_MAC "Compile asciiid mac" OFF)
endif()

project("asciicker_binaries") 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_GENERATOR)
	set(CMAKE_GENERATOR "Unix Makefiles")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE EQUAL Release)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

if(COMPILE_ASCIIID)
  include(CMake/make_asciiid.cmake)
endif()

if(COMPILE_ASCIIID_MAC)
  include(CMake/make_asciiid_mac.cmake)
endif()

if(COMPILE_ASCIICKER)
  include(CMake/make_asciicker.cmake)
endif()

if(COMPILE_ASCIICKER_MAC)
  include(CMake/make_asciicker_mac.cmake)
endif()

if(COMPILE_ASCIICKER_TERM)
  include(CMake/make_asciicker_term.cmake)
endif()

if(COMPILE_ASCIICKER_TERM_MAC)
  include(CMake/make_asciicker_term_mac.cmake)
endif()

if(COMPILE_SERVER)
  include(CMake/make_asciicker_server.cmake)
endif()