#
# Copyright (c) 2019 TOYOTA MOTOR CORPORATION
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
cmake_minimum_required( VERSION 2.8.5 )

project (gstwayland)

set(CMAKE_CXX_FLAGS "-Wall -fpermissive")

find_package(PkgConfig)
find_package (Threads)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)
pkg_check_modules(WAYLAND_CURSOR wayland-cursor REQUIRED)
pkg_check_modules(WALTHAM waltham REQUIRED)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(WAYLAND_EGL wayland-egl REQUIRED)
pkg_check_modules(ILMCONTROL REQUIRED ilmControl)
pkg_check_modules(ILMINPUT REQUIRED ilmInput)

pkg_search_module(EGL egl required)
pkg_search_module(GSTREAMER gstreamer-1.0 required)
pkg_search_module(GSTREAMERAPP gstreamer-app-1.0 required)
pkg_search_module(DRM libdrm required)
pkg_search_module(WEBSOCKET libwebsockets required)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
find_library(GST_ALLOCATOR NAMES gstallocators-1.0 PATHs /usr/lib64)
find_library(GST_VIDEO NAMES gstvideo-1.0 PATHs /usr/lib64)
find_library(GST_WAYLAND NAMES gstwayland-1.0 PATHs /usr/lib64)

add_custom_command(
    OUTPUT  ivi-application-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

add_custom_command(
    OUTPUT  ivi-application-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

include_directories(
    ${WAYLAND_CLIENT_INCLUDE_DIR}
    ${WAYLAND_CURSOR_INCLUDE_DIR}
    ${WALTHAM_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMERAPP_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${WAYLAND_EGL_INCLUDE_DIR}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/waltham-renderer
    "${ILMCONTROL_INCLUDE_DIRS}"
    "${ILMINPUT_INCLUDE_DIRS}"
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_CURSOR_LIBRARY_DIRS}
    ${WALTHAM_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_EGL_LIBRAY_DIRS}
    ${GLESv2_LIBRARY_DIRS}
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMERAPP_LIBRARY_DIRS}
)

SET(LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
    ${WALTHAM_LIBRARIES}
    ${EGL_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMERAPP_LIBRARIES}
    ${GST_ALLOCATOR}
    ${GST_VIDEO}
    ${GST_WAYLAND}
    libwindowmanager.so
    libjson-c.so
    libhomescreen.so
    libafbwsc.so
    systemd
    ${ILMCONTROL_LIBRARIES}
    ${ILMINPUT_LIBRARIES}
    ${WEBSOCKET_LIBRARIES}
)

SET(SRC_FILES
    src/sdl-protocol.c
    src/gst-wayland-demo.cpp
)

#add executable file
add_executable(${PROJECT_NAME} ${SRC_FILES})

#add link library
add_definitions(${EGL_CFLAGS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/package ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/bin
    COMMAND cp -rf ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ${PROJECT_BINARY_DIR}/package/root/bin
    COMMAND wgtpkg-pack -f -o ${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}.wgt ${PROJECT_BINARY_DIR}/package/root)


install (TARGETS ${PROJECT_NAME} DESTINATION bin)
