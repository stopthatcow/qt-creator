#include "gdboptionspage.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

GdbOptionsPage::GdbOptionsPage()
{
}

QString GdbOptionsPage::settingsId()
{
    return QLatin1String("M.Gdb");
}

QString GdbOptionsPage::trName() const
{
    return tr("Gdb");
}

QString GdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString GdbOptionsPage::trCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.gdbLocationChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.gdbLocationChooser->setPromptDialogTitle(tr("Choose Gdb Location"));
    m_ui.scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.scriptFileChooser->setPromptDialogTitle(tr("Choose Location of Startup Script File"));

    m_group.clear();
    m_group.insert(theDebuggerAction(GdbLocation),
        m_ui.gdbLocationChooser);
    m_group.insert(theDebuggerAction(GdbScriptFile),
        m_ui.scriptFileChooser);
    m_group.insert(theDebuggerAction(GdbEnvironment),
        m_ui.environmentEdit);

#if 1
    m_ui.groupBoxPluginDebugging->hide();
#else // The related code (handleAqcuiredInferior()) is disabled as well.
    m_group.insert(theDebuggerAction(AllPluginBreakpoints),
        m_ui.radioButtonAllPluginBreakpoints);
    m_group.insert(theDebuggerAction(SelectedPluginBreakpoints),
        m_ui.radioButtonSelectedPluginBreakpoints);
    m_group.insert(theDebuggerAction(NoPluginBreakpoints),
        m_ui.radioButtonNoPluginBreakpoints);
    m_group.insert(theDebuggerAction(SelectedPluginBreakpointsPattern),
        m_ui.lineEditSelectedPluginBreakpointsPattern);
#endif

    m_ui.lineEditSelectedPluginBreakpointsPattern->
        setEnabled(theDebuggerAction(SelectedPluginBreakpoints)->value().toBool());
    connect(m_ui.radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui.lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    // FIXME
    m_ui.environmentEdit->hide();
    m_ui.labelEnvironment->hide();

    if (m_searchKeywords.isEmpty()) {
        // TODO: Add breakpoints, environment?
        QTextStream(&m_searchKeywords) << ' ' << m_ui.labelGdbLocation->text()
                << ' ' << m_ui.labelEnvironment->text()
                << ' ' << m_ui.labelGdbStartupScript->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}
void GdbOptionsPage::apply()
{
    m_group.apply(Core::ICore::instance()->settings());
}

void GdbOptionsPage::finish()
{
    m_group.finish();
}

bool GdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger
