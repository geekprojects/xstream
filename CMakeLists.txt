cmake_minimum_required(VERSION 3.30)
project(xscreen)

set(CMAKE_CXX_STANDARD 20)

include(FindOpenGL)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libufc REQUIRED libufc)
pkg_check_modules(gstreamer REQUIRED gstreamer-rtsp-1.0)

SET(FLAGS_COMMON "-Wall -Werror -DGL_SILENCE_DEPRECATION=1")
SET(CMAKE_CXX_FLAGS_DEBUG "${FLAGS_COMMON} -O0 -g -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
SET(CMAKE_CXX_FLAGS_RELEASE  "${FLAGS_COMMON} -O3")

add_definitions(
        -I/usr/local/include
        ${libufc_CFLAGS}
        ${gstreamer_CFLAGS}
)

message("Building XPlane plugin: $ENV{XPLANE_SDK}")

if(UNIX AND APPLE)
set(XPLM_LDFLAGS $ENV{XPLANE_SDK}/Libraries/Mac/XPLM.framework)
set(XPLM_CFLAGS -DAPL=1)
elseif (UNIX AND NOT APPLE)
# TODO!
endif()

set(XPLANE_INC $ENV{XPLANE_SDK}/CHeaders/XPLM)

message("XPLM_LDFLAGS: ${XPLM_LDFLAGS}")
message("XPLANE_INC: ${XPLANE_INC}")

include_directories(include src)

add_library(xscreen SHARED xstreamplugin.cpp xstreamplugin.h
        videostream.cpp
        videostream.h
        displaymanager.cpp
        displaymanager.h)
target_compile_definitions(xscreen PUBLIC ${XPLM_CFLAGS})
target_include_directories(xscreen PUBLIC ${XPLANE_INC})

target_link_libraries(
        xscreen
        -Wl,-rpath -Wl,/usr/local/lib
        ${libufc_LDFLAGS}
        ${gstreamer_LDFLAGS} -lgstrtspserver-1.0.0 -lgstapp-1.0.0
        ${OPENGL_LIBRARIES}
        ${XPLM_LDFLAGS}
)
