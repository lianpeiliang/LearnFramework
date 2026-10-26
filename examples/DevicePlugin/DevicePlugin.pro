QT += core widgets
CONFIG += c++11 plugin
TEMPLATE = lib
TARGET = DevicePlugin
isEmpty(DESTDIR): DESTDIR = $$PWD/../../bin/plugins
INCLUDEPATH += $$PWD/../../src/foundation
HEADERS += DevicePlugin.h ../../src/foundation/Plugin.h ../../src/foundation/WidgetProvider.h
HEADERS += ../../src/foundation/PluginServices.h ../../src/foundation/DeviceProtocol.h
SOURCES += DevicePlugin.cpp ../../src/foundation/Plugin.cpp

HEADERS += DeviceState.h widgets/DeviceMonitorWidget.h widgets/DeviceSettingsWidget.h
SOURCES += widgets/DeviceMonitorWidget.cpp widgets/DeviceSettingsWidget.cpp
