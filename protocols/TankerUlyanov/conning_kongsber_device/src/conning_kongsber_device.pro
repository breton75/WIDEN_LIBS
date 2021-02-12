#-------------------------------------------------
#
# Project created by QtCreator 2020-07-30T20:00:27
#
#-------------------------------------------------

QT       += network dbus serialport

QT       -= gui

TARGET = /home/user/Modus/lib/conning_kongsber
TEMPLATE = lib

CONFIG += c++11 plugin

VERSION += 1.1.0

DEFINES += CONNING_KONGSBER_LIBRARY
#DEFINES += VERSION_2

SOURCES += conning_kongsber_device.cpp \
    ../../../Modus/global/sv_interface_adaptor.cpp \
    ../../../svlib/sv_abstract_logger.cpp \
    ../../../Modus/global/sv_signal.cpp

HEADERS += conning_kongsber_device.h\
    ../../../Modus/global/sv_interface_adaptor.h \
        conning_kongsber_device_global.h \
    ../../../Modus/global/sv_abstract_device.h \
    ../../../svlib/sv_abstract_logger.h \
    device_params.h \
    ifc_udp_params.h \
    ifc_serial_params.h \
    ifc_test_params.h \
    signal_params.h \
    ../../../Modus/global/sv_signal.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}