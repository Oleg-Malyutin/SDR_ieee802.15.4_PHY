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
    device/adalm_pluto/pluto_sdr.cpp \
    device/adalm_pluto/pluto_sdr_rx.cpp \
    device/adalm_pluto/pluto_sdr_tx.cpp \
    device/limesdr_mini/lime_sdr.cpp \
    device/limesdr_mini/lime_sdr_rx.cpp \
    device/limesdr_mini/lime_sdr_tx.cpp \
    device/device.cpp \
    ieee802_15_4/rx_ieee802_15_4.cpp \
    ieee802_15_4/rx_mac_sublayer.cpp \
    ieee802_15_4/tx_ieee802_15_4.cpp \
    main.cpp \
    main_window.cpp \
    plot/plot.cpp \
    plot/qcustomplot.cpp

HEADERS += \
    device/adalm_pluto/pluto_sdr.h \
    device/adalm_pluto/pluto_sdr_rx.h \
    device/adalm_pluto/pluto_sdr_tx.h \
    device/limesdr_mini/lime_sdr.h \
    device/limesdr_mini/lime_sdr_rx.h \
    device/limesdr_mini/lime_sdr_tx.h \
    device/device.h \
    device/device_type.h \
    ieee802_15_4/ieee802_15_4.h \
    ieee802_15_4/rx_ieee802_15_4.h \
    ieee802_15_4/rx_mac_sublayer.h \
    ieee802_15_4/tx_ieee802_15_4.h \
    main_window.h \
    plot/plot.h \
    plot/qcustomplot.h \
    utils/buffers.hh

unix{
SOURCES += \
    device/limesdr_mini/driver/ADF4002/ADF4002.cpp \
    device/limesdr_mini/driver/API/LimeNET_micro.cpp \
    device/limesdr_mini/driver/API/LimeSDR.cpp \
    device/limesdr_mini/driver/API/LimeSDR_Core.cpp \
    device/limesdr_mini/driver/API/LimeSDR_PCIE.cpp \
    device/limesdr_mini/driver/API/LimeSDR_mini.cpp \
    device/limesdr_mini/driver/API/LmsGeneric.cpp \
    device/limesdr_mini/driver/API/lms7_api.cpp \
    device/limesdr_mini/driver/API/lms7_device.cpp \
    device/limesdr_mini/driver/API/qLimeSDR.cpp \
    device/limesdr_mini/driver/ConnectionFTDI/ConnectionFT601.cpp \
    device/limesdr_mini/driver/ConnectionFTDI/ConnectionFT601Entry.cpp \
    device/limesdr_mini/driver/ConnectionRegistry/BuiltinConnections.in.cpp \
    device/limesdr_mini/driver/ConnectionRegistry/ConnectionHandle.cpp \
    device/limesdr_mini/driver/ConnectionRegistry/ConnectionRegistry.cpp \
    device/limesdr_mini/driver/ConnectionRegistry/IConnection.cpp \
    device/limesdr_mini/driver/FPGA_common/FPGA_Mini.cpp \
    device/limesdr_mini/driver/FPGA_common/FPGA_Q.cpp \
    device/limesdr_mini/driver/FPGA_common/FPGA_common.cpp \
    device/limesdr_mini/driver/GFIR/corrections.c \
    device/limesdr_mini/driver/GFIR/gfir_lms.c \
    device/limesdr_mini/driver/GFIR/lms.c \
    device/limesdr_mini/driver/GFIR/recipes.c \
    device/limesdr_mini/driver/GFIR/rounding.c \
    device/limesdr_mini/driver/Logger.cpp \
    device/limesdr_mini/driver/Si5351C/Si5351C.cpp \
    device/limesdr_mini/driver/SystemResources.in.cpp \
    device/limesdr_mini/driver/VersionInfo.in.cpp \
    device/limesdr_mini/driver/limeRFE/RFE_Device.cpp \
    device/limesdr_mini/driver/limeRFE/limeRFE_api.cpp \
    device/limesdr_mini/driver/limeRFE/limeRFE_cmd.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_BaseCalibrations.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_RegistersMap.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_RxTxCalibrations.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_filtersCalibration.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_gainCalibrations.cpp \
    device/limesdr_mini/driver/lms7002m/LMS7002M_parameters.cpp \
    device/limesdr_mini/driver/lms7002m/mcu_dc_iq_calibration.cpp \
    device/limesdr_mini/driver/lms7002m_mcu/MCU_BD.cpp \
    device/limesdr_mini/driver/protocols/ConnectionImages.cpp \
    device/limesdr_mini/driver/protocols/LMS64CProtocol.cpp \
    device/limesdr_mini/driver/protocols/Streamer.cpp \
    device/limesdr_mini/driver/threadHelper/threadHelper.cpp \


HEADERS += \
    device/limesdr_mini/driver/ADF4002/ADF4002.h \
    device/limesdr_mini/driver/API/LimeNET_micro.h \
    device/limesdr_mini/driver/API/LimeSDR.h \
    device/limesdr_mini/driver/API/LimeSDR_Core.h \
    device/limesdr_mini/driver/API/LimeSDR_PCIE.h \
    device/limesdr_mini/driver/API/LimeSDR_mini.h \
    device/limesdr_mini/driver/API/LmsGeneric.h \
    device/limesdr_mini/driver/API/device_constants.h \
    device/limesdr_mini/driver/API/lms7_device.h \
    device/limesdr_mini/driver/API/qLimeSDR.h \
    device/limesdr_mini/driver/ConnectionFTDI/ConnectionFT601.h \
    device/limesdr_mini/driver/ConnectionFTDI/FTD3XXLibrary/FTD3XX.h \
    device/limesdr_mini/driver/ConnectionRegistry/ConnectionHandle.h \
    device/limesdr_mini/driver/ConnectionRegistry/ConnectionRegistry.h \
    device/limesdr_mini/driver/ConnectionRegistry/IConnection.h \
    device/limesdr_mini/driver/FPGA_common/FPGA_Mini.h \
    device/limesdr_mini/driver/FPGA_common/FPGA_Q.h \
    device/limesdr_mini/driver/FPGA_common/FPGA_common.h \
    device/limesdr_mini/driver/GFIR/dfilter.h \
    device/limesdr_mini/driver/GFIR/lms.h \
    device/limesdr_mini/driver/GFIR/lms_gfir.h \
    device/limesdr_mini/driver/GFIR/recipes.h \
    device/limesdr_mini/driver/GFIR/rounding.h \
    device/limesdr_mini/driver/INI.h \
    device/limesdr_mini/driver/LMS7002M_parameters.h \
    device/limesdr_mini/driver/LimeSuite.h \
    device/limesdr_mini/driver/LimeSuiteConfig.h \
    device/limesdr_mini/driver/Logger.h \
    device/limesdr_mini/driver/Si5351C/Si5351C.h \
    device/limesdr_mini/driver/SystemResources.h \
    device/limesdr_mini/driver/VersionInfo.h \
    device/limesdr_mini/driver/limeRFE/RFE_Device.h \
    device/limesdr_mini/driver/limeRFE/limeRFE.h \
    device/limesdr_mini/driver/limeRFE/limeRFE_cmd.h \
    device/limesdr_mini/driver/limeRFE/limeRFE_constants.h \
    device/limesdr_mini/driver/lms7002m/LMS7002M.h \
    device/limesdr_mini/driver/lms7002m/LMS7002M_RegistersMap.h \
    device/limesdr_mini/driver/lms7002m/mcu_programs.h \
    device/limesdr_mini/driver/lms7002m_mcu/MCU_BD.h \
    device/limesdr_mini/driver/lms7002m_mcu/MCU_File.h \
    device/limesdr_mini/driver/protocols/ADCUnits.h \
    device/limesdr_mini/driver/protocols/LMS64CCommands.h \
    device/limesdr_mini/driver/protocols/LMS64CProtocol.h \
    device/limesdr_mini/driver/protocols/LMSBoards.h \
    device/limesdr_mini/driver/protocols/Streamer.h \
    device/limesdr_mini/driver/protocols/dataTypes.h \
    device/limesdr_mini/driver/protocols/fifo.h \
    device/limesdr_mini/driver/threadHelper/threadHelper.h \
    device/limesdr_mini/usb_power_cycle.h

INCLUDEPATH += \
    device/limesdr_mini/driver/ADF4002 \
    device/limesdr_mini/driver/API \
    device/limesdr_mini/driver/ConnectionFTDI \
    device/limesdr_mini/driver/ConnectionRegistry \
    device/limesdr_mini/driver/FPGA_common \
    device/limesdr_mini/driver/GFIR \
    device/limesdr_mini/driver/limeRFE \
    device/limesdr_mini/driver/lms7002m \
    device/limesdr_mini/driver/lms7002m_mcu \
    device/limesdr_mini/driver/protocols \
    device/limesdr_mini/driver/threadHelper \
    device/limesdr_mini/driver

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
}

FORMS += \
    device/adalm_pluto/pluto_sdr.ui \
    device/limesdr_mini/lime_sdr.ui \
    main_window.ui

MOC_DIR = tmp
OBJECTS_DIR = obj
DESTDIR = ./bin

unix:!macx: LIBS += -lusb-1.0

win32 {
LIBS += -L$$PWD/device/libusb-1.0/ -llibusb-1.0.dll
INCLUDEPATH += $$PWD/device/libusb-1.0
DEPENDPATH += $$PWD/device/libusb-1.0
}

win32 {
LIBS += -L$$PWD/device/adalm_pluto/lib_iio/Windows-VS-2019-x64/ -llibiio
INCLUDEPATH += $$PWD/device/adalm_pluto/lib_iio/include
DEPENDPATH += $$PWD/device/adalm_pluto/lib_iio/include
}
else{
LIBS +=  -L"$${DESTDIR}" -liio
}

win32: LIBS += -lws2_32

win32 {
LIBS += -L$$PWD/device/limesdr_mini/lib/ -llime_sdr_mini_v1
INCLUDEPATH += $$PWD/device/limesdr_mini/lib
DEPENDPATH += $$PWD/device/limesdr_mini/lib
}



