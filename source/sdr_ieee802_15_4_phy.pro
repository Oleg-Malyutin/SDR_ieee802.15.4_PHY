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
QT += printsupport # for customplot
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += thread

QMAKE_CXXFLAGS += -Ofast
QMAKE_CXXFLAGS += -ftree-vectorize

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

MOC_DIR = tmp
OBJECTS_DIR = obj
DESTDIR = ./bin
unix{
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
}

# selection of devices (1-on, 0-off)
plutosdr = 1
hackrf = 1
limesdr = 0

equals(plutosdr,1):{
    QMAKE_CXXFLAGS += -DUSE_PLUTOSDR
    SOURCES += \
        device/adalm_pluto/pluto_sdr.cpp \
        device/adalm_pluto/pluto_sdr_rx.cpp \
        device/adalm_pluto/pluto_sdr_tx.cpp \
        device/adalm_pluto/rx_usb_plutosdr.cpp \
        device/adalm_pluto/sdrusbgadget.cpp \
        device/adalm_pluto/tx_usb_plutosdr.cpp \
        device/adalm_pluto/upload_sdrusbgadget.cpp
    HEADERS += \
        device/adalm_pluto/pluto_sdr.h \
        device/adalm_pluto/pluto_sdr_rx.h \
        device/adalm_pluto/pluto_sdr_tx.h \
        device/adalm_pluto/rx_usb_plutosdr.h \
        device/adalm_pluto/sdrusbgadget.h \
        device/adalm_pluto/tx_usb_plutosdr.h \
        device/adalm_pluto/upload_sdrusbgadget.h
    FORMS += \
        device/adalm_pluto/pluto_sdr.ui
    DISTFILES += \
        device/adalm_pluto/resources/S24udc \
        device/adalm_pluto/resources/runme.sh \
        device/adalm_pluto/resources/sdr_usb_gadget
    RESOURCES += \
        device/adalm_pluto/resources/resources.qrc
    }

equals(hackrf,1):{
    QMAKE_CXXFLAGS += -DUSE_HACKRF
    SOURCES += \
        device/hackrf_one/hackrf_one.cpp \
        device/hackrf_one/hackrf_one_rx.cpp \
        device/hackrf_one/hackrf_one_tx.cpp \
        device/hackrf_one/libhackrf/hackrf.c \
        device/hackrf_one/rx_usb_hackrf.cpp
    HEADERS += \
        device/hackrf_one/hackrf_one.h \
        device/hackrf_one/hackrf_one_rx.h \
        device/hackrf_one/hackrf_one_tx.h \
        device/hackrf_one/libhackrf/hackrf.h \
        device/hackrf_one/rx_usb_hackrf.h
    FORMS += \
        device/hackrf_one/hackrf_one.ui
    }

equals(limesdr,1):{
    QMAKE_CXXFLAGS += -DUSE_LIMESDR
    SOURCES += \
        device/limesdr_mini/lime_sdr.cpp \
        device/limesdr_mini/lime_sdr_rx.cpp \
        device/limesdr_mini/lime_sdr_tx.cpp
    HEADERS += \
        device/limesdr_mini/lime_sdr.h \
        device/limesdr_mini/lime_sdr_rx.h \
        device/limesdr_mini/lime_sdr_tx.h
    FORMS += \
        device/limesdr_mini/lime_sdr.ui
    DISTFILES += \
        device/limesdr_mini/driver/ConnectionRegistry/CMakeLists.txt \
        device/limesdr_mini/lime_default.cfg
    }

SOURCES += \
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
    device/callback_device.h \
    device/device.h \
    device/device_type.h \
    higher_layer.h \
    ieee802_15_4/callback_layer.h \
    ieee802_15_4/mac_sublayer/mac_sublayer.h \
    ieee802_15_4/mac_sublayer/mac_types.h \
    ieee802_15_4/oqpsk_demodulator.h \
    ieee802_15_4/oqpsk_modulator.h \
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
    main_window.ui



unix{
INCLUDEPATH += /usr/include/libusb-1.0
LIBS += -lusb-1.0
LIBS += -lssh
LIBS += -liio
#LIBS += -lad9361
LIBS += -lLimeSuite
}
win32 {
LIBS += -L$$PWD/device/limesdr_mini/driver/ConnectionFTDI/FTD3XXLibrary/x64/ -lFTD3XX
LIBS += -L$$PWD/device/libusb-1.0/ -llibusb-1.0
INCLUDEPATH += $$PWD/device/libusb-1.0
DEPENDPATH += $$PWD/device/libusb-1.0
LIBS += -L$$PWD/device/adalm_pluto/lib_iio/Windows-VS-2019-x64/ -llibiio
INCLUDEPATH += $$PWD/device/adalm_pluto/lib_iio/include
DEPENDPATH += $$PWD/device/adalm_pluto/lib_iio/include
LIBS += -L$$PWD/device/adalm_pluto/libssh/ -lssh
INCLUDEPATH += $$PWD/device/adalm_pluto/libssh/include
DEPENDPATH += $$PWD/device/adalm_pluto/libssh/include
LIBS += -lws2_32
}









