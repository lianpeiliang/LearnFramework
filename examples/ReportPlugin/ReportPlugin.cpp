#include "ReportPlugin.h"

#include <QDebug>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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
    return {QStringLiteral("report.viewer"), QStringLiteral("report.editor")};
}

QWidget *ReportPlugin::createWidget(const QString &name, QWidget *parent)
{
    if (!state)
        return nullptr;
    if (name == "report.viewer")
        return new ReportViewerWidget(state, parent);
    if (name == "report.editor")
        return new ReportEditorWidget(state, parent);
    return nullptr;
}
