TEMPLATE = subdirs

# 同时构建主程序和示例插件；Learn.pro 仍可单独打开。
SUBDIRS += application demoPlugin contentPlugin devicePlugin reportPlugin
application.file = Learn.pro
demoPlugin.file = examples/DemoPlugin/DemoPlugin.pro
demoPlugin.depends = application
contentPlugin.file = examples/ContentPlugin/ContentPlugin.pro
contentPlugin.depends = application
devicePlugin.file = examples/DevicePlugin/DevicePlugin.pro
devicePlugin.depends = application
reportPlugin.file = examples/ReportPlugin/ReportPlugin.pro
reportPlugin.depends = application
