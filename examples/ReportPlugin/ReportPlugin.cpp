#include "ReportPlugin.h"

#include <QDebug>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

// 每个预览先读取当前文本，再监听共享状态的后续变更。
class ReportViewerWidget : public QWidget
{
public:
    ReportViewerWidget(ReportState *state, QWidget *parent) : QWidget(parent)
    {
        setObjectName("reportViewerWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("ReportPlugin.dll · 只读预览"), this));
        auto *preview = new QPlainTextEdit(this);
        preview->setObjectName("reportPreview");
        preview->setReadOnly(true);
        preview->setPlainText(state->text);
        layout->addWidget(preview);
        connect(state, &ReportState::changed, this, [state, preview] {
            preview->setPlainText(state->text);
        });
    }
};

// 编辑框保存本窗口的草稿，点击“更新预览”才提交到共享状态。
// 其他编辑器的草稿不会被覆盖，所有预览则会收到同一次 changed 通知。
class ReportEditorWidget : public QWidget
{
public:
    ReportEditorWidget(ReportState *state, QWidget *parent) : QWidget(parent)
    {
        setObjectName("reportEditorWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("内容仅在本次运行中保留"), this));
        auto *editor = new QPlainTextEdit(this);
        editor->setObjectName("reportTextEditor");
        editor->setPlainText(state->text);
        auto *apply = new QPushButton(QStringLiteral("更新预览"), this);
        apply->setObjectName("reportApplyButton");
        layout->addWidget(editor);
        layout->addWidget(apply);
        connect(apply, &QPushButton::clicked, this, [state, editor] {
            state->text = editor->toPlainText();
            emit state->changed();
        });
    }
};

bool ReportPlugin::initialize()
{
    // 同一插件实例重复初始化时保留已有文本，不重新分配状态。
    if (!state)
        state = new ReportState;
    qInfo() << "ReportPlugin initialized";
    return true;
}

bool ReportPlugin::shutdown()
{
    delete state;
    state = nullptr;
    qInfo() << "ReportPlugin shutdown";
    return true;
}

QStringList ReportPlugin::widgetClasses() const
{
    // 这些名称是主程序布局配置与控件工厂约定的键，不是 C++ 类名。
    return {QStringLiteral("report.viewer"), QStringLiteral("report.editor")};
}

QWidget *ReportPlugin::createWidget(const QString &name, QWidget *parent)
{
    // 同一个状态可供多个窗口使用；返回控件的所有权由 Qt 的 parent 链管理。
    if (!state)
        return nullptr;
    if (name == "report.viewer")
        return new ReportViewerWidget(state, parent);
    if (name == "report.editor")
        return new ReportEditorWidget(state, parent);
    return nullptr;
}
