TEMPLATE = subdirs

# 同时构建主程序和示例插件；Learn.pro 仍可单独打开。
SUBDIRS += application demoPlugin
application.file = Learn.pro
demoPlugin.file = examples/DemoPlugin/DemoPlugin.pro
demoPlugin.depends = application
