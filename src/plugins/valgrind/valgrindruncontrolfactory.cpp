/****************************************************************************
**
** Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "valgrindruncontrolfactory.h"

#include "valgrindengine.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"
#include "callgrindtool.h"
#include "memchecktool.h"

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerrunconfigwidget.h>

#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <QTcpServer>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind {
namespace Internal {

ValgrindRunControlFactory::ValgrindRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool ValgrindRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    Q_UNUSED(runConfiguration);
    return mode == CALLGRIND_RUN_MODE || mode == MEMCHECK_RUN_MODE || mode == MEMCHECK_WITH_GDB_RUN_MODE;
}

RunControl *ValgrindRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    auto runControl = qobject_cast<ValgrindRunControl *>(AnalyzerManager::createRunControl(runConfiguration, mode));
    QTC_ASSERT(runControl, return 0);

    ApplicationLauncher::Mode localRunMode = ApplicationLauncher::Gui;
    IDevice::ConstPtr device = DeviceKitInformation::device(runConfiguration->target()->kit());
    Utils::Environment environment;
    AnalyzerRunnable runnable;
    AnalyzerConnection connection;
    QString workingDirectory;
    Runnable rcRunnable = runConfiguration->runnable();
    QTC_ASSERT(rcRunnable.is<StandardRunnable>(), return 0);
    auto stdRunnable = runConfiguration->runnable().as<StandardRunnable>();
    if (device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        environment = stdRunnable.environment;
        workingDirectory = stdRunnable.workingDirectory;
        runnable.debuggee = stdRunnable.executable;
        runnable.debuggeeArgs = stdRunnable.commandLineArguments;
        QTcpServer server;
        if (!server.listen(QHostAddress::LocalHost) && !server.listen(QHostAddress::LocalHostIPv6)) {
            qWarning() << "Cannot open port on host for profiling.";
            return 0;
        }
        connection.connParams.host = server.serverAddress().toString();
        connection.connParams.port = server.serverPort();
        localRunMode = stdRunnable.runMode;
    } else {
        runnable.debuggee = stdRunnable.executable;
        runnable.debuggeeArgs = stdRunnable.commandLineArguments;
        connection.connParams = device->sshParameters();
    }

    runControl->setRunnable(runnable);
    runControl->setConnection(connection);
    runControl->setLocalRunMode(localRunMode);
    runControl->setEnvironment(environment);
    runControl->setWorkingDirectory(workingDirectory);
    return runControl;
}


class ValgrindRunConfigurationAspect : public IRunConfigurationAspect
{
public:
    ValgrindRunConfigurationAspect(RunConfiguration *parent)
        : IRunConfigurationAspect(parent)
    {
        setProjectSettings(new ValgrindProjectSettings());
        setGlobalSettings(ValgrindPlugin::globalSettings());
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindRunConfigurationAspect", "Valgrind Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
    }

    ValgrindRunConfigurationAspect *create(RunConfiguration *parent) const override
    {
        return new ValgrindRunConfigurationAspect(parent);
    }

    RunConfigWidget *createConfigurationWidget() override
    {
        return new AnalyzerRunConfigWidget(this);
    }
};

IRunConfigurationAspect *ValgrindRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    return new ValgrindRunConfigurationAspect(rc);
}

} // namespace Internal
} // namespace Valgrind
