#include "ApplicationHost.h"
#include "PluginLoader.h"
#include "WidgetContainer.h"
#include "WidgetFactory.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPluginLoader>
#include <QPointer>
#include <QPushButton>
#include <QtTest>

class WidgetPluginSmokeTests : public QObject
{
    Q_OBJECT

    QByteArray pluginPath() const
    {
        return (QCoreApplication::applicationDirPath() + "/plugins").toUtf8();
    }

    bool load(PluginLoader &loader)
    {
        QByteArray path = pluginPath();
        return loader.dispatchCommand(1000, path.data(), path.size());
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(QFile::exists(QString::fromUtf8(pluginPath()) + "/ContentPlugin.dll"),
                 "Build ContentPlugin with the same Kit/config and copy it into test-bin/plugins");
        QVERIFY(QDir().mkpath(QCoreApplication::applicationDirPath() + "/ui"));
    }

    void createsPluginContentThroughApplicationHost()
    {
        QFile ui(QCoreApplication::applicationDirPath() + "/ui/main.ui");
        QVERIFY(ui.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray xml =
            "<ui version=\"4.0\"><class>Window</class>"
            "<widget class=\"QWidget\" name=\"pluginTestWindow\">"
            "<layout class=\"QHBoxLayout\" name=\"layout\">"
            "<item><widget class=\"WidgetContainer\" name=\"left\">"
            "<property name=\"contentClass\" stdset=\"0\"><string>plugin.f</string></property>"
            "</widget></item><item><widget class=\"WidgetContainer\" name=\"right\">"
            "<property name=\"contentClass\" stdset=\"0\"><string>plugin.g</string></property>"
            "</widget></item></layout></widget>"
            "<customwidgets><customwidget><class>WidgetContainer</class>"
            "<extends>QWidget</extends><header>WidgetContainer.h</header>"
            "</customwidget></customwidgets><resources/><connections/></ui>";
        QCOMPARE(ui.write(xml), qint64(xml.size()));
        ui.close();

        QPointer<QWidget> first;
        QPointer<QWidget> second;
        {
            ApplicationHost host;
            QVERIFY(host.show());
            QWidget *root = nullptr;
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (widget->objectName() == "pluginTestWindow")
                    root = widget;
            QVERIFY(root);
            auto *left = root->findChild<WidgetContainer *>("left");
            auto *right = root->findChild<WidgetContainer *>("right");
            QVERIFY(left);
            QVERIFY(right);
            first = left->contentWidget();
            second = right->contentWidget();
            QVERIFY(first);
            QVERIFY(second);
            QCOMPARE(first->objectName(), QStringLiteral("pluginFWidget"));
            QCOMPARE(second->objectName(), QStringLiteral("pluginGWidget"));
            auto *button = first->findChild<QPushButton *>("pluginCounterButton");
            QVERIFY(button);
            QTest::mouseClick(button, Qt::LeftButton);
            QCOMPARE(button->text(), QStringLiteral("插件计数：1"));
            auto *input = second->findChild<QLineEdit *>("pluginTextInput");
            auto *label = second->findChild<QLabel *>("pluginTextLabel");
            QVERIFY(input);
            QVERIFY(label);
            input->setText("from DLL");
            QCOMPARE(label->text(), QStringLiteral("from DLL"));
            QVERIFY(left->setContentClass("plugin.g"));
            QVERIFY(first.isNull());
            QVERIFY(left->setContentClass("c"));
            QCOMPARE(left->contentWidget()->objectName(), QStringLiteral("cWidget"));
        }
        QVERIFY(second.isNull());
        QVERIFY(QApplication::topLevelWidgets().isEmpty());
    }

    void factoryCopiesKeepPluginAliveAndDestroyContentBeforePlugin()
    {
        WidgetFactory factory;
        WidgetFactory copy;
        QPointer<QObject> pluginObject;
        {
            PluginLoader loader(&factory);
            QVERIFY(load(loader));
            QVERIFY(load(loader)); // 重复扫描不会重复初始化或报名称冲突。
            copy = factory;
            QPluginLoader probe(QString::fromUtf8(pluginPath()) + "/ContentPlugin.dll");
            pluginObject = probe.instance();
            QVERIFY(pluginObject);
            probe.unload(); // 真正的模块引用仍由 PluginLoader 和工厂持有。
        }
        QVERIFY(pluginObject);
        factory.clear();
        QWidget parent;
        QPointer<QWidget> widget = copy.create("plugin.f", &parent);
        QVERIFY(widget);
        QStringList destructionOrder;
        QObject observer;
        connect(widget.data(), &QObject::destroyed, &observer, [&] { destructionOrder << "widget"; });
        connect(pluginObject.data(), &QObject::destroyed, &observer, [&] { destructionOrder << "plugin"; });
        copy.clear();
        QVERIFY(widget.isNull());
        QVERIFY(pluginObject.isNull());
        QCOMPARE(destructionOrder, QStringList({"widget", "plugin"}));
    }

    void rejectsDuplicateNamesWithoutPartialRegistration()
    {
        WidgetFactory factory;
        QVERIFY(factory.registerWidget("plugin.g", [](QWidget *parent) { return new QWidget(parent); }));
        QVERIFY(!factory.registerWidget("plugin.g", [](QWidget *parent) { return new QWidget(parent); }));
        QVERIFY(!factory.registerWidget("", [](QWidget *parent) { return new QWidget(parent); }));
        PluginLoader loader(&factory);
        QVERIFY(!load(loader));
        QVERIFY(!factory.contains("plugin.f"));
        QWidget parent;
        QVERIFY(factory.create("plugin.g", &parent));
    }

    void secondHostDoesNotRemoveFirstHostsCommandBinding()
    {
        ApplicationHost first;
        {
            ApplicationHost second;
            QVERIFY(!second.show());
        }
        QVERIFY(first.show());
    }

    void cleanupTestCase()
    {
        QFile::remove(QCoreApplication::applicationDirPath() + "/ui/main.ui");
    }
};

QTEST_MAIN(WidgetPluginSmokeTests)
#include "WidgetPluginSmokeTests.moc"
