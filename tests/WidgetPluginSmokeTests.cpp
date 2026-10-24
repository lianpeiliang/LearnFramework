#include "ApplicationHost.h"
#include "PluginLoader.h"
#include "WidgetContainer.h"
#include "WidgetFactory.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPluginLoader>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
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
        for (const QString &name : {QStringLiteral("ContentPlugin"), QStringLiteral("DevicePlugin"),
                                    QStringLiteral("ReportPlugin")})
            QVERIFY2(QFile::exists(QString::fromUtf8(pluginPath()) + "/" + name + ".dll"),
                     qPrintable("Build and copy " + name + " into test-bin/plugins"));
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

    void loadsSixPanelsFromThreeIndependentPlugins()
    {
        const QString sourceUi = QFINDTESTDATA("../ui/main.ui");
        QVERIFY(!sourceUi.isEmpty());
        const QString runtimeUi = QCoreApplication::applicationDirPath() + "/ui/main.ui";
        QVERIFY(QFile::remove(runtimeUi));
        QVERIFY(QFile::copy(sourceUi, runtimeUi));
        QList<QPointer<QWidget>> contents;
        {
            ApplicationHost host;
            QVERIFY(host.show());
            QWidget *root = nullptr;
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (widget->objectName() == "applicationWindow")
                    root = widget;
            QVERIFY(root);
            auto *grid = qobject_cast<QGridLayout *>(root->layout());
            QVERIFY(grid);
            QCOMPARE(grid->rowCount(), 2);
            QCOMPARE(grid->columnCount(), 3);
            QCOMPARE(root->findChildren<WidgetContainer *>().size(), 6);
            const QStringList containerNames = {"contentCounterContainer", "contentTextContainer",
                "deviceMonitorContainer", "deviceSettingsContainer", "reportViewerContainer",
                "reportEditorContainer"};
            const QStringList widgetNames = {"pluginFWidget", "pluginGWidget", "deviceMonitorWidget",
                "deviceSettingsWidget", "reportViewerWidget", "reportEditorWidget"};
            for (int i = 0; i < containerNames.size(); ++i)
            {
                auto *container = root->findChild<WidgetContainer *>(containerNames.at(i));
                QVERIFY(container);
                QWidget *content = container->contentWidget();
                QVERIFY(content);
                QVERIFY(content->isVisible());
                QCOMPARE(content->objectName(), widgetNames.at(i));
                QCOMPARE(content->parentWidget(), container);
                contents.append(content);
            }

            auto *counter = root->findChild<QPushButton *>("pluginCounterButton");
            QVERIFY(counter);
            QTest::mouseClick(counter, Qt::LeftButton);
            QCOMPARE(counter->text(), QStringLiteral("插件计数：1"));
            auto *textInput = root->findChild<QLineEdit *>("pluginTextInput");
            auto *textLabel = root->findChild<QLabel *>("pluginTextLabel");
            QVERIFY(textInput);
            QVERIFY(textLabel);
            textInput->setText("independent text");
            QCOMPARE(textLabel->text(), QStringLiteral("independent text"));

            auto *nameInput = root->findChild<QLineEdit *>("deviceNameInput");
            auto *intervalInput = root->findChild<QSpinBox *>("deviceIntervalInput");
            auto *applyDevice = root->findChild<QPushButton *>("deviceApplyButton");
            auto *nameLabel = root->findChild<QLabel *>("deviceNameLabel");
            auto *intervalLabel = root->findChild<QLabel *>("deviceIntervalLabel");
            auto *connection = root->findChild<QPushButton *>("deviceConnectionButton");
            auto *status = root->findChild<QLabel *>("deviceStatusLabel");
            QVERIFY(nameInput && intervalInput && applyDevice && nameLabel && intervalLabel && connection && status);
            nameInput->setText(QStringLiteral("测试设备"));
            intervalInput->setValue(2500);
            QTest::mouseClick(applyDevice, Qt::LeftButton);
            QCOMPARE(nameLabel->text(), QStringLiteral("测试设备"));
            QVERIFY(intervalLabel->text().contains("2500"));
            QTest::mouseClick(connection, Qt::LeftButton);
            QVERIFY(status->text().contains(QStringLiteral("已连接")));
            nameInput->setText("   ");
            QTest::mouseClick(applyDevice, Qt::LeftButton);
            QCOMPARE(nameLabel->text(), QStringLiteral("测试设备"));

            auto *editor = root->findChild<QPlainTextEdit *>("reportTextEditor");
            auto *preview = root->findChild<QPlainTextEdit *>("reportPreview");
            auto *applyReport = root->findChild<QPushButton *>("reportApplyButton");
            QVERIFY(editor && preview && applyReport);
            QVERIFY(preview->isReadOnly());
            editor->setPlainText(QStringLiteral("新的报表内容"));
            QTest::mouseClick(applyReport, Qt::LeftButton);
            QCOMPARE(preview->toPlainText(), QStringLiteral("新的报表内容"));
            QCOMPARE(counter->text(), QStringLiteral("插件计数：1"));
            QCOMPARE(textLabel->text(), QStringLiteral("independent text"));

            // 重新装入同一设备插件的面板，仍使用该插件的共享状态。
            auto *monitor = root->findChild<WidgetContainer *>("deviceMonitorContainer");
            QVERIFY(monitor->setContentClass("plugin.f"));
            QVERIFY(contents.at(2).isNull());
            QVERIFY(monitor->setContentClass("device.monitor"));
            QCOMPARE(monitor->findChild<QLabel *>("deviceNameLabel")->text(), QStringLiteral("测试设备"));
            contents.append(monitor->contentWidget());
            // 同时覆盖一次窗口缩放和布局分配。
            root->resize(1000, 650);
            QApplication::processEvents();
            for (WidgetContainer *container : root->findChildren<WidgetContainer *>())
                QCOMPARE(container->contentWidget()->geometry(), container->rect());
            QVERIFY(root->grab().save(QCoreApplication::applicationDirPath() + "/six-panels-preview.png"));
        }
        for (const QPointer<QWidget> &content : contents)
            QVERIFY(content.isNull());
        QVERIFY(QApplication::topLevelWidgets().isEmpty());
    }

    void cleanupTestCase()
    {
        QFile::remove(QCoreApplication::applicationDirPath() + "/ui/main.ui");
    }
};

QTEST_MAIN(WidgetPluginSmokeTests)
#include "WidgetPluginSmokeTests.moc"
