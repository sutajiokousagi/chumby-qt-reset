requires(qws)
TEMPLATE=plugin
PLUGIN_FOR=qte
PLUGIN_TYPE=kbddrivers

TARGET=examplekbdhandler

CONFIG+=qtopia
QTOPIA=base

HEADERS=\
	examplekbddriverplugin.h\
	examplekbdhandler.h

SOURCES=\
	examplekbddriverplugin.cpp\
	examplekbdhandler.cpp
