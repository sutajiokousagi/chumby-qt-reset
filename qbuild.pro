TARGET = qchumbyresetkbddriver
include(qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/kbddrivers
target.path = $$[QT_INSTALL_PLUGINS]/kbddrivers
INSTALLS += target

HEADERS	= qchumbyresetkb_qws.h

SOURCES	= main.cpp \
	qchumbyresetkb_qws.cpp

