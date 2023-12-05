cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_ASAN OFF)

project(libcrtc)

set(WEBRTC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/3dparty/webrtc/src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${WEBRTC_SRC})
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/libyuv/include)
include_directories(SYSTEM ${WEBRTC_SRC}/buildtools/third_party/libc++/trunk/include)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/perfetto/buildtools/libcxx_config)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/abseil-cpp/)
include_directories(SYSTEM ${WEBRTC_SRC}/third_party/jsoncpp/source/include/)

add_library(crtc SHARED
	include/crtc.h
	src/arraybuffer.cc src/arraybuffer.h
	src/atomic.cc
	src/audiobuffer.cc src/audiobuffer.h
	src/audiosource.cc src/audiosource.h
	src/customvideodecoder.cc src/customvideodecoder.h
	src/customaudiodecoder.cc src/customaudiodecoder.h
	src/customaudiofactory.cc src/customaudiofactory.h
	src/customvideofactory.cc src/customvideofactory.h
	src/error.cc src/error.h
	src/event.cc src/event.h
	src/fakeaudiodevice.cc src/fakeaudiodevice.h
	#src/imagebuffer.cc src/imagebuffer.h
	#src/mediadevices.cc src/mediadevices.h
	src/mediastream.cc src/mediastream.h
	src/mediastreamtrack.cc src/mediastreamtrack.h
	src/module.cc src/module.h
	src/promise.h
	src/rtcdatachannel.cc src/rtcdatachannel.h
	src/rtcpeerconnection.cc src/rtcpeerconnection.h
	src/string.cc
	src/time.cc
	src/videoframe.cc src/videoframe.h
	)
  
	if(WIN32)
		set(CXXFLAGS "${CXXFLAGS} /permissive-")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /permissive-")
		target_compile_definitions(crtc PRIVATE CRTC_EXPORTS NOMINMAX WEBRTC_WIN)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/win32/x64")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/win32/x86")
		endif()
	elseif(ANDROID)
		target_compile_definitions(crtc PRIVATE WEBRTC_POSIX)
		target_link_libraries(crtc PRIVATE android log)
		if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "armeabi-v7a")
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/android/arm")
		elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a")
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/android/arm64")
		elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86")
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/android/x86")
		elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86_64")
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/android/x64")
		endif()
	elseif(LINUX)
		target_compile_definitions(crtc PRIVATE WEBRTC_POSIX)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/x64")
		elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
			target_link_directories(crtc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/x86")
		endif()
	endif()

	target_link_libraries(crtc PRIVATE webrtc)