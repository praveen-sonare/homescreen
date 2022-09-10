# Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
# Copyright (c) 2017 TOYOTA MOTOR CORPORATION
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

TEMPLATE = app
TARGET = homescreen
QT = qml quick gui-private
CONFIG += c++11 link_pkgconfig wayland-scanner
PKGCONFIG += wayland-client
PKGCONFIG += qtappfw-weather qtappfw-network qtappfw-bt qtappfw-vehicle-signals qtappfw-applauncher

SOURCES += \
    src/main.cpp \
    src/shell.cpp \
    src/statusbarmodel.cpp \
    src/statusbarserver.cpp \
    src/applicationlauncher.cpp \
    src/mastervolume.cpp \
    src/homescreenhandler.cpp

HEADERS  += \
    src/shell.h \
    src/constants.h \
    src/statusbarmodel.h \
    src/statusbarserver.h \
    src/applicationlauncher.h \
    src/mastervolume.h \
    src/homescreenhandler.h

OTHER_FILES += \
    README.md

RESOURCES += \
    qml/images/MediaPlayer/mediaplayer.qrc \
    qml/images/MediaMusic/mediamusic.qrc \
    qml/images/Weather/weather.qrc \
    qml/images/Shortcut/shortcut.qrc \
    qml/images/Status/status.qrc \
    qml/images/images.qrc \
    qml/qml.qrc

AGL_SHELL_PATH = $$system(pkg-config --variable=pkgdatadir agl-compositor-0.0.20-protocols)
WAYLANDCLIENTSOURCES += $$AGL_SHELL_PATH/agl-shell.xml $$AGL_SHELL_PATH/agl-shell-desktop.xml

target.path = $${PREFIX}/usr/bin
target.files += $${OUT_PWD}/$${TARGET}
target.CONFIG = no_check_exist executable

INSTALLS += target
