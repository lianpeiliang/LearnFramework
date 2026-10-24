QT += core widgets uitools testlib
CONFIG += c++11 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ApplicationHostUiTests
# 隔离测试 UI，避免改动实际运行目录的设计文件。
DESTDIR = $$OUT_PWD/ui-test-bin
INCLUDEPATH += $$PWD/../src/foundation $$PWD/../src/runtime
include(../src/widgets/widgets.pri)
HEADERS += $$PWD/../src/runtime/PluginMessaging.h
SOURCES += $$PWD/../src/runtime/PluginMessaging.cpp
SOURCES += ApplicationHostUiTests.cpp \
    ../src/runtime/ApplicationHost.cpp \
    ../src/runtime/CommandBindings.cpp \
    ../src/runtime/PluginLoader.cpp \
    ../src/foundation/CommandBus.cpp \
    ../src/foundation/ServiceRegistry.cpp \
    ../src/foundation/Plugin.cpp
