QT += core widgets uitools testlib
CONFIG += c++11 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = WidgetPluginSmokeTests
DESTDIR = $$OUT_PWD/plugin-test-bin
INCLUDEPATH += $$PWD/../src/foundation $$PWD/../src/runtime
include(../src/widgets/widgets.pri)
HEADERS += $$PWD/../src/foundation/WidgetProvider.h
HEADERS += $$PWD/../src/runtime/PluginMessaging.h
SOURCES += $$PWD/../src/runtime/PluginMessaging.cpp
SOURCES += WidgetPluginSmokeTests.cpp \
    ../src/runtime/ApplicationHost.cpp \
    ../src/runtime/CommandBindings.cpp \
    ../src/runtime/PluginLoader.cpp \
    ../src/foundation/CommandBus.cpp \
    ../src/foundation/ServiceRegistry.cpp \
    ../src/foundation/Plugin.cpp
