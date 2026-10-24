#include "DemoWidgets.h"
#include "WidgetFactory.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

CWidget::CWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("cWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("C：计数面板"), this));
    auto *button = new QPushButton(QStringLiteral("点击次数：0"), this);
    button->setObjectName("counterButton");
    layout->addWidget(button);
    layout->addStretch();
    // 使用面板作为连接上下文，面板销毁时自动断开捕获 this/button 的回调。
    connect(button, &QPushButton::clicked, this, [this, button] {
        button->setText(QStringLiteral("点击次数：%1").arg(++count));
    });
}

DWidget::DWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("dWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("D：文本面板"), this));
    auto *input = new QLineEdit(this);
    input->setPlaceholderText(QStringLiteral("输入一些文字"));
    auto *label = new QLabel(this);
    layout->addWidget(input);
    layout->addWidget(label);
    layout->addStretch();
    // 直接连接文本变化信号与标签的设置槽，无需另外维护一份文本状态。
    connect(input, &QLineEdit::textChanged, label, &QLabel::setText);
}

EWidget::EWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("eWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("E：列表面板"), this));
    auto *list = new QListWidget(this);
    list->addItems({QStringLiteral("项目一"), QStringLiteral("项目二"), QStringLiteral("项目三")});
    layout->addWidget(list);
}

void registerDemoWidgets(WidgetFactory &factory)
{
    // 每次调用回调都创建新面板，并将容器传入的 parent 交给 QWidget 构造函数。
    factory.registerWidget("c", [](QWidget *parent) { return new CWidget(parent); });
    factory.registerWidget("d", [](QWidget *parent) { return new DWidget(parent); });
    factory.registerWidget("e", [](QWidget *parent) { return new EWidget(parent); });
}
