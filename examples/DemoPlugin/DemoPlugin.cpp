#include "DemoPlugin.h"

#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DemoPlugin::~DemoPlugin()
{
    shutdown();
}

bool DemoPlugin::initialize()
{
    // 当前加载器只调用 initialize/shutdown，因此在初始化时直接显示窗口。
    // 防止重复初始化时创建多个窗口。
    if (window)
        return start();

    clickCount = 0;
    // 没有父窗口，因此由插件在 shutdown 中负责释放，而不是交给内容容器管理。
    window = new QWidget(nullptr, Qt::Tool);
    window->setObjectName("demoPluginWindow");
    window->setWindowTitle(QStringLiteral("示例插件"));
    window->resize(360, 160);

    auto *layout = new QVBoxLayout(window.data());
    auto *label = new QLabel(QStringLiteral("插件加载成功，点击按钮试试。"), window.data());
    label->setObjectName("demoPluginLabel");
    auto *button = new QPushButton(QStringLiteral("点我计数"), window.data());
    button->setObjectName("demoPluginButton");
    layout->addWidget(label);
    layout->addWidget(button);

    // 使用窗口作为连接的上下文；窗口销毁后，Qt 自动断开这个连接。
    connect(button, &QPushButton::clicked, window.data(), [this, label]() {
        ++clickCount;
        label->setText(QStringLiteral("你已经点击了 %1 次。").arg(clickCount));
    });

    qInfo() << "DemoPlugin initialized";
    return start();
}

bool DemoPlugin::start()
{
    if (!window)
        return false;
    window->show();
    return true;
}

bool DemoPlugin::stop()
{
    // 暂停仅隐藏窗口，保留按钮计数，后续 start 可继续显示同一窗口。
    if (window)
        window->hide();
    return true;
}

bool DemoPlugin::shutdown()
{
    // 可以重复调用，兼容加载器主动关闭及 Qt 销毁插件两种情况。
    if (window)
    {
        stop();
        delete window.data();
        window = nullptr;
        qInfo() << "DemoPlugin shutdown";
    }
    clickCount = 0;
    return true;
}
