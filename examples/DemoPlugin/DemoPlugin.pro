QT += core widgets
CONFIG += c++11 plugin
TEMPLATE = lib
TARGET = DemoPlugin

# 与主程序的 bin 目录对应，无需手动复制 DLL。
DESTDIR = $$PWD/../../bin/plugins
INCLUDEPATH += $$PWD/../../src/foundation

HEADERS += DemoPlugin.h
HEADERS += ../../src/foundation/Plugin.h
SOURCES += DemoPlugin.cpp
# 插件独立构建，包含接口析构函数的实现，不依赖主程序导出符号。
SOURCES += ../../src/foundation/Plugin.cpp
