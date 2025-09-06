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
#QMAKE_CXXFLAGS += -mavx

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    device/adalm_pluto/pluto_sdr.cpp \
    device/adalm_pluto/pluto_sdr_rx.cpp \
    device/adalm_pluto/pluto_sdr_tx.cpp \
    device/device.cpp \
    higher_layer.cpp \
    ieee802_15_4/mac_sublayer/mac_sublayer.cpp \
    ieee802_15_4/oqpsk_demodulator.cpp \
    ieee802_15_4/oqpsk_modulator.cpp \
    ieee802_15_4/phy_layer/phy_layer.cpp \
    main.cpp \
    main_window.cpp \
    plot/plot.cpp \
    plot/qcustomplot.cpp \
    utils/timer_cb.cpp \
    utils/udp.cpp

HEADERS += \
    callback_higher_layer.h \
    callback_ui.h \
    device/adalm_pluto/pluto_sdr.h \
    device/adalm_pluto/pluto_sdr_rx.h \
    device/adalm_pluto/pluto_sdr_tx.h \
    device/callback_device.h \
    device/device.h \
    device/device_type.h \
    higher_layer.h \
    ieee802_15_4/callback_layer.h \
    ieee802_15_4/mac_sublayer/mac_sublayer.h \
    ieee802_15_4/mac_sublayer/mac_types.h \
    ieee802_15_4/oqpsk_demodulator.h \
    ieee802_15_4/oqpsk_modulator.h \
    ieee802_15_4/pan_information_base.hh \
    ieee802_15_4/phy_layer/phy_layer.h \
    ieee802_15_4/phy_layer/phy_types.h \
    ieee802_15_4/rf_ieee802_15_4_constants.h \
    main_window.h \
    plot/plot.h \
    plot/qcustomplot.h \
    utils/buffers.hh \
    utils/timer_cb.h \
    utils/udp.h \
    utils/zep.h

FORMS += \
    device/adalm_pluto/pluto_sdr.ui \
    main_window.ui

MOC_DIR = tmp
OBJECTS_DIR = obj
DESTDIR = ./bin
unix{
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
}

unix{
LIBS += -lusb-1.0
LIBS +=  -L"$${DESTDIR}" -liio
}
win32 {
LIBS += -L$$PWD/device/libusb-1.0/ -llibusb-1.0.dll
INCLUDEPATH += $$PWD/device/libusb-1.0
DEPENDPATH += $$PWD/device/libusb-1.0
LIBS += -L$$PWD/device/adalm_pluto/lib_iio/Windows-VS-2019-x64/ -llibiio
INCLUDEPATH += $$PWD/device/adalm_pluto/lib_iio/include
DEPENDPATH += $$PWD/device/adalm_pluto/lib_iio/include
LIBS += -lws2_32
}






