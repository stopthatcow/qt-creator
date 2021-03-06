/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qbsnodes.h"

#include "qbsnodetreebuilder.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsrunconfiguration.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QtDebug>
#include <QDir>
#include <QStyle>

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

static QIcon generateIcon(const QString &overlay)
{
    const QSize desiredSize = QSize(16, 16);
    const QIcon overlayIcon(overlay);
    const QPixmap pixmap
            = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon, overlayIcon, desiredSize);

    QIcon result;
    result.addPixmap(pixmap);

    return result;
}

namespace QbsProjectManager {
namespace Internal {

QIcon QbsGroupNode::m_groupIcon;
QIcon QbsProjectNode::m_projectIcon;
QIcon QbsProductNode::m_productIcon;

static QbsProjectNode *parentQbsProjectNode(ProjectExplorer::Node *node)
{
    for (ProjectExplorer::FolderNode *pn = node->managingProject(); pn; pn = pn->parentProjectNode()) {
        QbsProjectNode *prjNode = dynamic_cast<QbsProjectNode *>(pn);
        if (prjNode)
            return prjNode;
    }
    return 0;
}

static QbsProductNode *parentQbsProductNode(ProjectExplorer::Node *node)
{
    for (; node; node = node->parentFolderNode()) {
        QbsProductNode *prdNode = dynamic_cast<QbsProductNode *>(node);
        if (prdNode)
            return prdNode;
    }
    return 0;
}

static qbs::GroupData findMainQbsGroup(const qbs::ProductData &productData)
{
    foreach (const qbs::GroupData &grp, productData.groups()) {
        if (grp.name() == productData.name() && grp.location() == productData.location())
            return grp;
    }
    return qbs::GroupData();
}

class FileTreeNode {
public:
    explicit FileTreeNode(const QString &n = QString(), FileTreeNode *p = 0, bool f = false) :
        parent(p), name(n), m_isFile(f)
    {
        if (p)
            p->children.append(this);
    }

    ~FileTreeNode()
    {
        qDeleteAll(children);
    }

    FileTreeNode *addPart(const QString &n, bool isFile)
    {
        foreach (FileTreeNode *c, children) {
            if (c->name == n)
                return c;
        }
        return new FileTreeNode(n, this, isFile);
    }

    bool isFile() const { return m_isFile; }

    static FileTreeNode *moveChildrenUp(FileTreeNode *node)
    {
        QTC_ASSERT(node, return 0);

        FileTreeNode *newParent = node->parent;
        if (!newParent)
            return 0;

        // disconnect node and parent:
        node->parent = 0;
        newParent->children.removeOne(node);

        foreach (FileTreeNode *c, node->children) {
            // update path, make sure there will be no / before "C:" on windows:
            if (Utils::HostOsInfo::isWindowsHost() && node->name.isEmpty())
                c->name = node->name;
            else
                c->name = node->name + QLatin1Char('/') + c->name;

            newParent->children.append(c);
            c->parent = newParent;
        }

        // Delete node
        node->children.clear();
        delete node;
        return newParent;
    }

    // Moves the children of the node pointing to basedir to the root of the tree.
    static void reorder(FileTreeNode *node, const QString &basedir)
    {
        QTC_CHECK(!basedir.isEmpty());
        QString prefix = basedir;
        if (basedir.startsWith(QLatin1Char('/')))
            prefix = basedir.mid(1);
        prefix.append(QLatin1Char('/'));

        if (node->path() == basedir) {
            // Find root node:
            FileTreeNode *root = node;
            while (root->parent)
                root = root->parent;

            foreach (FileTreeNode *c, node->children) {
                // Update children names by prepending basedir
                c->name = prefix + c->name;
                // Update parent information:
                c->parent = root;

                root->children.append(c);
            }

            // Clean up node:
            node->children.clear();
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;

            return;
        }

        foreach (FileTreeNode *n, node->children)
            reorder(n, basedir);
    }

    static void simplify(FileTreeNode *node)
    {
        foreach (FileTreeNode *c, node->children)
            simplify(c);

        if (!node->parent)
            return;

        if (node->children.isEmpty() && !node->isFile()) {
            // Clean up empty folder nodes:
            node->parent->children.removeOne(node);
            node->parent = 0;
            delete node;
        } else if (node->children.count() == 1 && !node->children.at(0)->isFile()) {
            // Compact folder nodes with one child only:
            moveChildrenUp(node);
        }
    }

    QString path() const
    {
        QString p = name;
        FileTreeNode *node = parent;
        while (node) {
            if (!Utils::HostOsInfo::isWindowsHost() || !node->name.isEmpty())
                p = node->name + QLatin1Char('/') + p;
            node = node->parent;
        }
        return p;
    }

    QList<FileTreeNode *> children;
    FileTreeNode *parent;
    QString name;
    bool m_isFile;
};


static QList<ProjectExplorer::ProjectAction> supportedNodeActions(ProjectExplorer::Node *node,
                                                                  bool managesFiles)
{
    QList<ProjectExplorer::ProjectAction> actions;
    const QbsProject * const project = parentQbsProjectNode(node)->project();
    if (!project->isProjectEditable())
        return actions;
    if (managesFiles)
        actions << ProjectExplorer::AddNewFile << ProjectExplorer::AddExistingFile;
    if (node->nodeType() == ProjectExplorer::NodeType::File
            && !project->qbsProject().buildSystemFiles().contains(node->filePath().toString())) {
        actions << ProjectExplorer::RemoveFile << ProjectExplorer::Rename;
    }
    return actions;
}

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

QbsFileNode::QbsFileNode(const Utils::FileName &filePath,
                         const ProjectExplorer::FileType fileType,
                         bool generated,
                         int line) :
    ProjectExplorer::FileNode(filePath, fileType, generated, line)
{ }

QString QbsFileNode::displayName() const
{
    int l = line();
    if (l < 0)
        return ProjectExplorer::FileNode::displayName();
    return ProjectExplorer::FileNode::displayName() + QLatin1Char(':') + QString::number(l);
}


QbsFolderNode::QbsFolderNode(const Utils::FileName &folderPath, ProjectExplorer::NodeType nodeType,
                             const QString &displayName, bool isGeneratedFilesFolder)
    : ProjectExplorer::FolderNode(folderPath, nodeType, displayName),
      m_isGeneratedFilesFolder(isGeneratedFilesFolder)
{
}

QList<ProjectExplorer::ProjectAction> QbsFolderNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, false);
}

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

QbsBaseProjectNode::QbsBaseProjectNode(const Utils::FileName &path) :
    ProjectExplorer::ProjectNode(path)
{ }

bool QbsBaseProjectNode::showInSimpleTree() const
{
    return false;
}

QList<ProjectExplorer::ProjectAction> QbsBaseProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::ProjectAction>();
}

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

QbsGroupNode::QbsGroupNode(const qbs::GroupData &grp, const QString &productPath) :
    QbsBaseProjectNode(Utils::FileName())
{
    if (m_groupIcon.isNull())
        m_groupIcon = QIcon(QString::fromLatin1(Constants::QBS_GROUP_ICON));

    setIcon(m_groupIcon);

    m_productPath = productPath;
    m_qbsGroupData = grp;
}

QList<ProjectExplorer::ProjectAction> QbsGroupNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true);
}

bool QbsGroupNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    QbsProductNode *prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    return prjNode->project()->addFilesToProduct(filePaths, prdNode->qbsProductData(),
                                                 m_qbsGroupData, notAdded);
}

bool QbsGroupNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    QbsProductNode *prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    return prjNode->project()->removeFilesFromProduct(filePaths, prdNode->qbsProductData(),
                                                      m_qbsGroupData, notRemoved);
}

bool QbsGroupNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QbsProjectNode * const prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid())
        return false;
    QbsProductNode * const prdNode = parentQbsProductNode(this);
    if (!prdNode || !prdNode->qbsProductData().isValid())
        return false;

    return prjNode->project()->renameFileInProduct(filePath, newFilePath,
                                                   prdNode->qbsProductData(), m_qbsGroupData);
}

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

QbsProductNode::QbsProductNode(const qbs::ProductData &prd) :
    QbsBaseProjectNode(Utils::FileName::fromString(prd.location().filePath()))
{
    if (m_productIcon.isNull())
        m_productIcon = generateIcon(QString::fromLatin1(Constants::QBS_PRODUCT_OVERLAY_ICON));

    setIcon(m_productIcon);
}

bool QbsProductNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectExplorer::ProjectAction> QbsProductNode::supportedActions(ProjectExplorer::Node *node) const
{
    return supportedNodeActions(node, true);
}

bool QbsProductNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    QStringList notAddedDummy;
    if (!notAdded)
        notAdded = &notAddedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notAdded += filePaths;
        return false;
    }

    qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->addFilesToProduct(filePaths, m_qbsProductData, grp, notAdded);
    }

    QTC_ASSERT(false, return false);
}

bool QbsProductNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    QStringList notRemovedDummy;
    if (!notRemoved)
        notRemoved = &notRemovedDummy;

    QbsProjectNode *prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid()) {
        *notRemoved += filePaths;
        return false;
    }

    qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    if (grp.isValid()) {
        return prjNode->project()->removeFilesFromProduct(filePaths, m_qbsProductData, grp,
                                                          notRemoved);
    }

    QTC_ASSERT(false, return false);
}

bool QbsProductNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    QbsProjectNode * const prjNode = parentQbsProjectNode(this);
    if (!prjNode || !prjNode->qbsProject().isValid())
        return false;
    const qbs::GroupData grp = findMainQbsGroup(m_qbsProductData);
    QTC_ASSERT(grp.isValid(), return false);
    return prjNode->project()->renameFileInProduct(filePath, newFilePath, m_qbsProductData, grp);
}

QList<ProjectExplorer::RunConfiguration *> QbsProductNode::runConfigurations() const
{
    QList<ProjectExplorer::RunConfiguration *> result;
    auto pn = dynamic_cast<const QbsProjectNode *>(managingProject());
    if (!isEnabled() || !pn || m_qbsProductData.targetExecutable().isEmpty())
        return result;

    foreach (ProjectExplorer::RunConfiguration *rc, pn->project()->activeTarget()->runConfigurations()) {
        QbsRunConfiguration *qbsRc = qobject_cast<QbsRunConfiguration *>(rc);
        if (!qbsRc)
            continue;
        if (qbsRc->buildSystemTarget() == QbsProject::uniqueProductName(qbsProductData()))
            result << qbsRc;
    }

    return result;
}

QbsGroupNode *QbsProductNode::findGroupNode(const QString &name)
{
    for (ProjectExplorer::Node *n : nodes()) {
        if (QbsGroupNode *qn = dynamic_cast<QbsGroupNode *>(n))
            if (qn->qbsGroupData().name() == name)
                return qn;
    }
    return 0;
}

// --------------------------------------------------------------------
// QbsProjectNode:
// --------------------------------------------------------------------

QbsProjectNode::QbsProjectNode(const Utils::FileName &projectDirectory) :
    QbsBaseProjectNode(projectDirectory)
{
    if (m_projectIcon.isNull())
        m_projectIcon = generateIcon(QString::fromLatin1(ProjectExplorer::Constants::FILEOVERLAY_QT));

    setIcon(m_projectIcon);
}

QbsProjectNode::~QbsProjectNode()
{
    // do not delete m_project
}

QbsProject *QbsProjectNode::project() const
{
    return static_cast<QbsProjectNode *>(parentFolderNode())->project();
}

const qbs::Project QbsProjectNode::qbsProject() const
{
    return project()->qbsProject();
}

bool QbsProjectNode::showInSimpleTree() const
{
    return true;
}

void QbsProjectNode::setProjectData(const qbs::ProjectData &data)
{
    m_projectData = data;
}

// --------------------------------------------------------------------
// QbsRootProjectNode:
// --------------------------------------------------------------------

QbsRootProjectNode::QbsRootProjectNode(QbsProject *project) :
    QbsProjectNode(project->projectDirectory()),
    m_project(project)
{ }

} // namespace Internal
} // namespace QbsProjectManager
