#include "ApplicationHost.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPointer>
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

    void cleanupTestCase()
    {
        QFile::remove(uiPath());
    }
};

QTEST_MAIN(ApplicationHostUiTests)
#include "ApplicationHostUiTests.moc"
