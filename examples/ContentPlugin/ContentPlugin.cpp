#include "ContentPlugin.h"

#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

// 这两个业务类只编译在 DLL 中；主程序没有包含或链接它们。
class FWidget : public QWidget
{
public:
    explicit FWidget(QWidget *parent) : QWidget(parent)
    {
        setObjectName("pluginFWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("F：来自 ContentPlugin.dll"), this));
        auto *button = new QPushButton(QStringLiteral("插件计数：0"), this);
        button->setObjectName("pluginCounterButton");
        layout->addWidget(button);
        layout->addStretch();
        connect(button, &QPushButton::clicked, this, [this, button] {
            button->setText(QStringLiteral("插件计数：%1").arg(++count));
        });
    }

private:
    int count = 0;
};

class GWidget : public QWidget
{
public:
    explicit GWidget(QWidget *parent) : QWidget(parent)
    {
        setObjectName("pluginGWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("G：来自 ContentPlugin.dll"), this));
        auto *input = new QLineEdit(this);
        input->setObjectName("pluginTextInput");
        input->setPlaceholderText(QStringLiteral("输入文字，体验 DLL 内的界面逻辑"));
        auto *label = new QLabel(this);
        label->setObjectName("pluginTextLabel");
        layout->addWidget(input);
        layout->addWidget(label);
        layout->addStretch();
        connect(input, &QLineEdit::textChanged, label, &QLabel::setText);
    }
};

bool ContentPlugin::initialize()
{
    initialized = true;
    qInfo() << "ContentPlugin initialized";
    return true;
}

bool ContentPlugin::shutdown()
{
    if (initialized)
        qInfo() << "ContentPlugin shutdown";
    initialized = false;
    return true;
}

QStringList ContentPlugin::widgetClasses() const
{
    return {QStringLiteral("plugin.f"), QStringLiteral("plugin.g")};
}

QWidget *ContentPlugin::createWidget(const QString &name, QWidget *parent)
{
    if (!initialized)
        return nullptr;
    if (name == "plugin.f")
        return new FWidget(parent);
    if (name == "plugin.g")
        return new GWidget(parent);
    return nullptr;
}
