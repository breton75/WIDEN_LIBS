QT -= gui
QT += network

TEMPLATE = lib
DEFINES += TCP_CLIENT_LIBRARY

CONFIG += c++11 plugin


VERSION = 1.2.0    # major.minor.patch
DEFINES += LIB_VERSION=\\\"$$VERSION\\\"
DEFINES += "LIB_AUTHOR=\"\\\"Свиридов С. А.\\\"\""

TARGET = /home/user/Modus/lib/interfaces/tcp_client
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Для генерации ошибки линкёра в случае наличия неопределённых
# ссылок (undefined references) при сборке разделяемой библиотеки:
QMAKE_LFLAGS += -Wno-unused-variable, -Wl,--no-undefined

# Для отключения отладочного вывода через qDebug() в release-версии:
CONFIG (debug, debug|release){
    message ("Debug")
}else{
    message ("Release")
    DEFINES += QT_NO_DEBUG_OUTPUT
}

SOURCES += \
    sv_tcp_client.cpp

HEADERS += \
    ../../../../Modus/global/device/interface/sv_abstract_interface.h \
    tcp_client_defs.h \
    sv_tcp_client.h \
    tcp_client_global.h \
    ../../../../Modus/global/device/device_defs.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
