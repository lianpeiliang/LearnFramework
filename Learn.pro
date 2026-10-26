QT += core widgets uitools
CONFIG += c++11
TEMPLATE = app
TARGET = LearnFramework
isEmpty(DESTDIR): DESTDIR = $$PWD/bin

# 外部 UI 不交给 uic，也不嵌入 qrc；构建时部署到程序旁。
DISTFILES += ui/main.ui
build_pass|!debug_and_release {
    deployUi.target = deploy_runtime_ui
    deployUi.commands = $$QMAKE_MKDIR $$shell_quote($$shell_path($$DESTDIR/ui))
    win32: deployUi.commands = if not exist $$shell_quote($$shell_path($$DESTDIR/ui)) $$deployUi.commands
    deployUi.commands += $$escape_expand(\n\t) \
        $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/ui/main.ui)) $$shell_quote($$shell_path($$DESTDIR/ui/main.ui))
    QMAKE_EXTRA_TARGETS += deployUi first
    first.depends += deploy_runtime_ui
}
INCLUDEPATH += $$PWD/src/foundation
INCLUDEPATH += $$PWD/src/runtime
include(src/widgets/widgets.pri)

# 程序入口与核心调度
SOURCES += src/runtime/main.cpp
SOURCES += src/runtime/ApplicationHost.cpp
SOURCES += src/runtime/CommandBindings.cpp
SOURCES += src/runtime/PluginLoader.cpp
SOURCES += src/runtime/PluginMessaging.cpp

HEADERS += src/runtime/ApplicationHost.h
HEADERS += src/runtime/CommandBindings.h
HEADERS += src/runtime/PluginLoader.h
HEADERS += src/runtime/PluginMessaging.h

# 基础接口与通用服务
SOURCES += src/foundation/ServiceRegistry.cpp
SOURCES += src/foundation/CommandBus.cpp
SOURCES += src/foundation/Plugin.cpp

HEADERS += src/foundation/ServiceRegistry.h
HEADERS += src/foundation/CommandBus.h
HEADERS += src/foundation/Plugin.h
HEADERS += src/foundation/WidgetProvider.h
HEADERS += src/foundation/PluginServices.h src/foundation/DeviceProtocol.h
