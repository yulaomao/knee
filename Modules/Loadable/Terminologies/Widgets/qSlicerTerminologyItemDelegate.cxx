/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Terminologies includes
#include "qSlicerTerminologyItemDelegate.h"

#include "qSlicerTerminologySelectorButton.h"
#include "qSlicerTerminologyNavigatorWidget.h"

#include "vtkSlicerTerminologiesModuleLogic.h"
#include "vtkSlicerTerminologyEntry.h"

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
vtkSlicerTerminologiesModuleLogic* terminologiesLogic()
{
  vtkSlicerTerminologiesModuleLogic* terminologiesLogic = vtkSlicerTerminologiesModuleLogic::SafeDownCast(
    qSlicerCoreApplication::application()->moduleLogic("Terminologies"));
  if (!terminologiesLogic)
    {
    qCritical() << Q_FUNC_INFO << ": Terminologies logic is not found";
    }
  return terminologiesLogic;
}

//-----------------------------------------------------------------------------
qSlicerTerminologyItemDelegate::qSlicerTerminologyItemDelegate(QObject *parent)
  : QStyledItemDelegate(parent) { }

//-----------------------------------------------------------------------------
QWidget* qSlicerTerminologyItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
  qSlicerTerminologySelectorButton* terminologyButton = new qSlicerTerminologySelectorButton(parent);
  terminologyButton->setProperty("changeDataOnSet", true);
  connect(terminologyButton, SIGNAL(terminologyChanged()), this, SLOT(commitAndClose()), Qt::QueuedConnection);
  connect(terminologyButton, SIGNAL(canceled()), this, SLOT(close()), Qt::QueuedConnection);
  return terminologyButton;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  qSlicerTerminologySelectorButton* terminologyButton = qobject_cast<qSlicerTerminologySelectorButton*>(editor);
  if (!terminologyButton)
    {
    return;
    }
  if (!terminologyButton->property("changeDataOnSet").toBool())
    {
    return;
    }
  vtkSlicerTerminologiesModuleLogic* logic = terminologiesLogic();
  terminologyButton->setProperty("changeDataOnSet", false);

  // Get string list value from model index
  QString terminologyString = index.model()->data(index, TerminologyRole).toString();

  // Convert string list to VTK terminology entry. Do not check success, as an empty terminology is also a valid starting point
  vtkNew<vtkSlicerTerminologyEntry> terminologyEntry;
  if (logic)
    {
    logic->DeserializeTerminologyEntry(terminologyString.toUtf8().constData(), terminologyEntry);
    }

  // Get metadata
  QString name = index.model()->data(index, NameRole).toString();
  bool nameAutoGenerated = index.model()->data(index, NameAutoGeneratedRole).toBool();
  QColor color = index.model()->data(index, Qt::DecorationRole).value<QColor>();
  bool colorAutoGenerated = index.model()->data(index, ColorAutoGeneratedRole).toBool();
  QColor generatedColor = index.model()->data(index, qSlicerTerminologyItemDelegate::GeneratedColorRole).value<QColor>();

  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle terminologyInfo(
    terminologyEntry, name, nameAutoGenerated, color, colorAutoGenerated, generatedColor );
  terminologyButton->setTerminologyInfo(terminologyInfo);

  terminologyButton->changeTerminology();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  vtkSlicerTerminologiesModuleLogic* logic = terminologiesLogic();
  if (!logic)
    {
    return;
    }
  // Get terminology (changed by the user) from the terminology button
  qSlicerTerminologySelectorButton* terminologyButton = qobject_cast<qSlicerTerminologySelectorButton*>(editor);
  if (!terminologyButton)
    {
    return;
    }
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle terminologyInfo;
  terminologyButton->terminologyInfo(terminologyInfo);

  // Set color to model
  model->setData(index, terminologyInfo.ColorAutoGenerated, ColorAutoGeneratedRole);
  model->setData(index, terminologyInfo.Color, Qt::DecorationRole);
  // Set name to model
  model->setData(index, terminologyInfo.NameAutoGenerated, NameAutoGeneratedRole);
  model->setData(index, terminologyInfo.Name, NameRole);
  // Set terminology string to model
  model->setData(index, logic->SerializeTerminologyEntry(terminologyInfo.GetTerminologyEntry()).c_str(), TerminologyRole);
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

//------------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::commitSenderData()
{
  QWidget* editor = qobject_cast<QWidget*>(this->sender());
  if (!editor)
    {
    return;
    }
  emit commitData(editor);
}

//------------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::commitAndClose()
{
  QWidget* editor = qobject_cast<QWidget*>(this->sender());
  if (!editor)
    {
    return;
    }
  emit commitData(editor);
  emit closeEditor(editor);
}

//------------------------------------------------------------------------------
void qSlicerTerminologyItemDelegate::close()
{
  QWidget* editor = qobject_cast<QWidget*>(this->sender());
  if (!editor)
    {
    return;
    }
  emit closeEditor(editor);
}