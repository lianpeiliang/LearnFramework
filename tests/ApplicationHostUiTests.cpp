#include "ApplicationHost.h"
#include "DemoWidgets.h"
#include "WidgetContainer.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

class ApplicationHostUiTests : public QObject
{
    Q_OBJECT

    QString uiPath() const
    {
        return QCoreApplication::applicationDirPath() + "/ui/main.ui";
    }

    void writeUi(const QByteArray &content)
    {
        QFile file(uiPath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(content), qint64(content.size()));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(QDir().mkpath(QCoreApplication::applicationDirPath() + "/ui"));
    }

    void reloadsEditedUiAndOwnsWindow()
    {
        const QByteArray original =
            "<ui version=\"4.0\"><class>TestWindow</class>"
            "<widget class=\"QWidget\" name=\"uiTestWindow\">"
            "<property name=\"windowTitle\"><string>Original</string></property>"
            "<layout class=\"QVBoxLayout\" name=\"layout\"><item>"
            "<widget class=\"QLabel\" name=\"message\">"
            "<property name=\"text\"><string>Original</string></property>"
            "</widget></item></layout></widget><resources/><connections/></ui>";

        QTemporaryDir otherDirectory;
        QVERIFY(otherDirectory.isValid());
        const QString previousDirectory = QDir::currentPath();
        QVERIFY(QDir::setCurrent(otherDirectory.path()));
        struct RestoreDirectory
        {
            QString path;
            ~RestoreDirectory() { QDir::setCurrent(path); }
        } restore{previousDirectory};

        for (const QByteArray &text : {QByteArray("Original"), QByteArray("Edited")})
        {
            QByteArray content = original;
            content.replace("Original", text);
            writeUi(content);
            QPointer<QWidget> loadedWindow;
            {
                ApplicationHost host;
                QVERIFY(host.show());
                for (QWidget *widget : QApplication::topLevelWidgets())
                    if (widget->objectName() == "uiTestWindow")
                        loadedWindow = widget;
                QVERIFY(loadedWindow);
                QVERIFY(loadedWindow->isVisible());
                QCOMPARE(loadedWindow->windowTitle(), QString::fromUtf8(text));
                QLabel *label = loadedWindow->findChild<QLabel *>("message");
                QVERIFY(label);
                QCOMPARE(label->text(), QString::fromUtf8(text));
                QVERIFY(host.show());
            }
            QVERIFY(loadedWindow.isNull());
        }
    }

    void rejectsMissingAndMalformedUi()
    {
        QVERIFY(QFile::remove(uiPath()));
        {
            ApplicationHost host;
            QVERIFY(!host.show());
        }
        writeUi("<ui><broken>");
        ApplicationHost host;
        QVERIFY(!host.show());
        QVERIFY(QApplication::topLevelWidgets().isEmpty());
    }

    void loadsConfiguredContainersAfterRestart()
    {
        const QByteArray ui =
            "<ui version=\"4.0\"><class>ContainerWindow</class>"
            "<widget class=\"QWidget\" name=\"containerTestWindow\">"
            "<layout class=\"QHBoxLayout\" name=\"layout\">"
            "<item><widget class=\"WidgetContainer\" name=\"leftContainer\">"
            "<property name=\"contentClass\" stdset=\"0\"><string>SELECTED</string></property>"
            "</widget></item>"
            "<item><widget class=\"WidgetContainer\" name=\"rightContainer\">"
            "<property name=\"contentClass\" stdset=\"0\"><string>c</string></property>"
            "</widget></item></layout></widget>"
            "<customwidgets><customwidget><class>WidgetContainer</class>"
            "<extends>QWidget</extends><header>WidgetContainer.h</header>"
            "<container>1</container></customwidget></customwidgets>"
            "<resources/><connections/></ui>";

        for (const QByteArray &type : {QByteArray("c"), QByteArray("d"), QByteArray("e")})
        {
            QByteArray configured = ui;
            configured.replace("SELECTED", type);
            writeUi(configured);
            QPointer<QWidget> leftContent;
            QPointer<QWidget> rightContent;
            {
                ApplicationHost host;
                QVERIFY(host.show());
                QWidget *root = nullptr;
                for (QWidget *widget : QApplication::topLevelWidgets())
                    if (widget->objectName() == "containerTestWindow")
                        root = widget;
                QVERIFY(root);
                auto *left = root->findChild<WidgetContainer *>("leftContainer");
                auto *right = root->findChild<WidgetContainer *>("rightContainer");
                QVERIFY(left);
                QVERIFY(right);
                leftContent = left->contentWidget();
                rightContent = right->contentWidget();
                QVERIFY(leftContent);
                QVERIFY(rightContent);
                QVERIFY(leftContent != rightContent);
                QCOMPARE(leftContent->objectName(), QString::fromUtf8(type) + "Widget");
                QCOMPARE(rightContent->objectName(), QStringLiteral("cWidget"));
                QCOMPARE(leftContent->parentWidget(), left);
                QCOMPARE(rightContent->parentWidget(), right);
                if (type == "c")
                {
                    auto *leftButton = leftContent->findChild<QPushButton *>("counterButton");
                    auto *rightButton = rightContent->findChild<QPushButton *>("counterButton");
                    QVERIFY(leftButton);
                    QVERIFY(rightButton);
                    QTest::mouseClick(leftButton, Qt::LeftButton);
                    QVERIFY(leftButton->text() != rightButton->text());
                }
            }
            QVERIFY(leftContent.isNull());
            QVERIFY(rightContent.isNull());
        }

        QByteArray invalid = ui;
        invalid.replace("SELECTED", "missing");
        writeUi(invalid);
        ApplicationHost host;
        QVERIFY(!host.show());
        QVERIFY(QApplication::topLevelWidgets().isEmpty());
    }

    void replacesContentAndPreservesItOnFailure()
    {
        WidgetFactory factory;
        registerDemoWidgets(factory);
        factory.registerWidget("failed", [](QWidget *) -> QWidget * { return nullptr; });
        WidgetContainer container(factory);
        QString error;
        QVERIFY(container.setContentClass("c", &error));
        QPointer<QWidget> original = container.contentWidget();
        QVERIFY(container.setContentClass("c", &error));
        QCOMPARE(container.contentWidget(), original.data());
        QVERIFY(!container.setContentClass("missing", &error));
        QVERIFY(error.contains("missing"));
        QVERIFY(!container.setContentClass("failed", &error));
        QCOMPARE(container.contentWidget(), original.data());
        QCOMPARE(container.property("contentClass").toString(), QStringLiteral("c"));
        QVERIFY(container.setContentClass("d", &error));
        QVERIFY(error.isEmpty());
        QVERIFY(original.isNull());
        QCOMPARE(container.contentWidget()->objectName(), QStringLiteral("dWidget"));
        QPointer<QWidget> replacement = container.contentWidget();
        QVERIFY(container.setContentClass("", &error));
        QVERIFY(replacement.isNull());
        QVERIFY(!container.contentWidget());
    }

    void cleanupTestCase()
    {
        QFile::remove(uiPath());
    }
};

QTEST_MAIN(ApplicationHostUiTests)
#include "ApplicationHostUiTests.moc"
