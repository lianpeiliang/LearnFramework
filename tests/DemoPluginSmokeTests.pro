QT += core widgets uitools testlib
CONFIG += c++11 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = DemoPluginSmokeTests
DESTDIR = $$PWD/../bin
INCLUDEPATH += $$PWD/../src/foundation $$PWD/../src/runtime
include(../src/widgets/widgets.pri)

SOURCES += DemoPluginSmokeTests.cpp \
    ../src/runtime/ApplicationHost.cpp \
    ../src/runtime/CommandBindings.cpp \
    ../src/runtime/PluginLoader.cpp \
    ../src/foundation/CommandBus.cpp \
    ../src/foundation/ServiceRegistry.cpp \
    ../src/foundation/Plugin.cpp
