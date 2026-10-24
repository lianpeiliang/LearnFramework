QT += core widgets
CONFIG += c++11
TEMPLATE = app
TARGET = LearnFramework
DESTDIR = $$PWD/bin
INCLUDEPATH += $$PWD/src/foundation
INCLUDEPATH += $$PWD/src/runtime

# 程序入口与核心调度
SOURCES += src/runtime/main.cpp
SOURCES += src/runtime/DesktopApplication.cpp
SOURCES += src/runtime/ApplicationHost.cpp
SOURCES += src/runtime/CommandBindings.cpp
SOURCES += src/runtime/PluginLoader.cpp

HEADERS += src/runtime/DesktopApplication.h
HEADERS += src/runtime/ApplicationHost.h
HEADERS += src/runtime/CommandBindings.h
HEADERS += src/runtime/PluginLoader.h

# 基础接口与通用服务
SOURCES += src/foundation/ServiceRegistry.cpp
SOURCES += src/foundation/CommandBus.cpp
SOURCES += src/foundation/Plugin.cpp

HEADERS += src/foundation/ServiceRegistry.h
HEADERS += src/foundation/CommandBus.h
HEADERS += src/foundation/Plugin.h
