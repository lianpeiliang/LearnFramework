#include "ApplicationHost.h"

#include <QApplication>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QtTest>

class DemoPluginSmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void loadsThroughCommand1000AndCleansUp()
    {
        QPointer<QWidget> pluginWindow;
        {
            // 走真实启动流程：命令绑定 → 分发 1000 → 动态加载插件。
            ApplicationHost host;
            QVERIFY(host.show());
            QApplication::processEvents();

            for (QWidget *widget : QApplication::topLevelWidgets())
            {
                if (widget->objectName() == "demoPluginWindow")
                    pluginWindow = widget;
            }
            QVERIFY2(pluginWindow, "DemoPlugin was not loaded from bin/plugins");
            QVERIFY(pluginWindow->isVisible());

            auto *button = pluginWindow->findChild<QPushButton *>("demoPluginButton");
            auto *label = pluginWindow->findChild<QLabel *>("demoPluginLabel");
            QVERIFY(button);
            QVERIFY(label);
            QCOMPARE(label->text(), QStringLiteral("插件加载成功，点击按钮试试。"));

            QTest::mouseClick(button, Qt::LeftButton);
            QTest::mouseClick(button, Qt::LeftButton);
            QCOMPARE(label->text(), QStringLiteral("你已经点击了 2 次。"));

            // 用户关闭小窗口后，主程序仍存活；退出主程序才释放插件窗口。
            pluginWindow->close();
            QVERIFY(!pluginWindow->isVisible());
            QVERIFY(pluginWindow);
        }
        QVERIFY2(pluginWindow.isNull(), "Plugin shutdown did not destroy its window");
    }
};

QTEST_MAIN(DemoPluginSmokeTests)
#include "DemoPluginSmokeTests.moc"
