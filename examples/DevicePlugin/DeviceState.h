#pragma once

#include <QObject>
#include <QString>

// 两个面板共享插件内的设备状态，仅演示模拟设备，不连接真实硬件。
class DeviceState : public QObject
{
    Q_OBJECT
public:
    QString name = QStringLiteral("示例设备");
    int refreshInterval = 1000;
    bool connected = false;

signals:
    void changed();
};

