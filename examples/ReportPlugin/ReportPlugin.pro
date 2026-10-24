QT += core widgets
CONFIG += c++11 plugin
TEMPLATE = lib
TARGET = ReportPlugin
isEmpty(DESTDIR): DESTDIR = $$PWD/../../bin/plugins
INCLUDEPATH += $$PWD/../../src/foundation
HEADERS += ReportPlugin.h ../../src/foundation/Plugin.h ../../src/foundation/WidgetProvider.h
SOURCES += ReportPlugin.cpp ../../src/foundation/Plugin.cpp
