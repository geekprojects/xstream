cmake_minimum_required(VERSION 3.30)
project(xstream)

set(CMAKE_CXX_STANDARD 20)

include(FindOpenGL)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libpng REQUIRED libpng)
pkg_check_modules(yamlcpp REQUIRED yaml-cpp)
pkg_check_modules(gstreamer REQUIRED gstreamer-rtsp-1.0)

SET(FLAGS_COMMON "-Wall -Werror -DGL_SILENCE_DEPRECATION=1")
SET(CMAKE_CXX_FLAGS_DEBUG "${FLAGS_COMMON} -O0 -g -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
SET(CMAKE_CXX_FLAGS_RELEASE  "${FLAGS_COMMON} -O3")

add_definitions(
        -I/usr/local/include
        ${yamlcpp_CFLAGS}
        ${libpng_CFLAGS}
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

add_library(xstream SHARED xstreamplugin.cpp xstreamplugin.h
        videostream.cpp
        videostream.h
        displaymanager.cpp
        displaymanager.h
        logger.cpp
        logger.h
)
target_compile_definitions(xstream PUBLIC ${XPLM_CFLAGS})
target_include_directories(xstream PUBLIC ${XPLANE_INC})

target_link_libraries(
        xstream
        -Wl,-rpath -Wl,/usr/local/lib
        ${yamlcpp_LDFLAGS}
        ${libpng_LDFLAGS}
        ${gstreamer_LDFLAGS} -lgstrtspserver-1.0.0 -lgstapp-1.0.0
        ${OPENGL_LIBRARIES}
        ${XPLM_LDFLAGS}
)

#add_executable(test test/test.cpp)
#target_link_libraries(
#        test
#        -Wl,-rpath -Wl,/usr/local/lib
#        ${gstreamer_LDFLAGS} -lgstrtspserver-1.0.0 -lgstapp-1.0.0
#)
