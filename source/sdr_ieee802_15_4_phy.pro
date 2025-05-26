#  Copyright 2025 Oleg Malyutin.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

TEMPLATE = app
TARGET = sdr_ieee802_15_4_phy

QT += core gui
QT += network
QT += printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += thread

QMAKE_CXXFLAGS += -Ofast
QMAKE_CXXFLAGS += -mavx

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    device/adalm_pluto/device_rx.cpp \
    device/adalm_pluto/device_tx.cpp \
    device/adalm_pluto/pluto_sdr.cpp \
    device/device.cpp \
    ieee802_15_4/ieee802_15_4.cpp \
    ieee802_15_4/mac_sublayer.cpp \
    main.cpp \
    main_window.cpp \
    plot/plot.cpp \
    plot/qcustomplot.cpp

HEADERS += \
    device/adalm_pluto/device_rx.h \
    device/adalm_pluto/device_tx.h \
    device/adalm_pluto/pluto_sdr.h \
    device/device.h \
    device/device_type.h \
    ieee802_15_4/ieee802_15_4.h \
    ieee802_15_4/mac_sublayer.h \
    main_window.h \
    plot/plot.h \
    plot/qcustomplot.h \
    utils/buffers.hh

FORMS += \
    device/adalm_pluto/pluto_sdr.ui \
    main_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

MOC_DIR = tmp
OBJECTS_DIR = obj
#DESTDIR = ../../bin
DESTDIR = ./

win32 {
LIBS +=  -L"$${DESTDIR}" -llibfftw3-3
LIBS += -L"$${DESTDIR}" -liio
}
else {
LIBS +=  -L"$${DESTDIR}" -lfftw3
LIBS +=  -L"$${DESTDIR}" -liio
}


