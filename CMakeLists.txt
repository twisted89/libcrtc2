cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_ASAN OFF)

project(libcrtc)

set(WEBRTC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/3dparty/webrtc/src)
message(${WEBRTC_SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${WEBRTC_SRC})
include_directories(SYSTEM ${WEBRTC_SRC}/buildtools/third_party/libc++/trunk/include)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/perfetto/buildtools/libcxx_config)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/abseil-cpp/)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/jsoncpp/source/include/)

add_library(crtc SHARED
	include/crtc.h
	src/promise.h
	src/arraybuffer.cc src/arraybuffer.h
	src/async.cc 
	src/atomic.cc
	src/audiobuffer.cc src/audiobuffer.h
	src/audiosource.cc src/audiosource.h
	src/error.cc src/error.h
	src/event.cc src/event.h
	src/imagebuffer.cc src/imagebuffer.h
	src/mediadevices.cc src/mediadevices.h
	src/mediastream.cc src/mediastream.h
	src/mediastreamtrack.cc src/mediastreamtrack.h
	src/module.cc src/module.h
	src/rtcdatachannel.cc src/rtcdatachannel.h
	src/rtcpeerconnection.cc src/rtcpeerconnection.h
	src/string.cc
	src/time.cc
	src/videoframe.cc src/videoframe.h
 "src/promise.h")
  
target_link_libraries(crtc
        PRIVATE
        ${WEBRTC_SRC}/out/Debug/obj/webrtc.lib
        )

if(WIN32)
	target_compile_definitions(crtc PRIVATE CRTC_EXPORT NOMINMAX )
endif()