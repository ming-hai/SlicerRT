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

// SubjectHierarchy includes
#include "qSlicerSegmentEditorScriptedMorphologyEffect.h"

// Qt includes
#include <QDebug>
#include <QFileInfo>

// SlicerQt includes
#include "qSlicerScriptedUtils_p.h"

// PythonQt includes
#include <PythonQt.h>
#include <PythonQtConversion.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPythonUtil.h>

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorScriptedMorphologyEffectPrivate
{
public:
  typedef qSlicerSegmentEditorScriptedMorphologyEffectPrivate Self;
  qSlicerSegmentEditorScriptedMorphologyEffectPrivate();
  virtual ~qSlicerSegmentEditorScriptedMorphologyEffectPrivate();

  enum {
    IconMethod = 0,
    HelpTextMethod,
    CloneMethod,
    ActivateMethod,
    DeactivateMethod,
    SetupOptionsFrameMethod,
    CreateCursorMethod,
    ProcessInteractionEventsMethod,
    ProcessViewNodeEventsMethod,
    SetMRMLDefaultsMethod,
    EditedLabelmapChangedMethod,
    MasterVolumeNodeChangedMethod,
    LayoutChangedMethod,
    UpdateGUIFromMRMLMethod,
    UpdateMRMLFromGUIMethod,
    };

  mutable qSlicerPythonCppAPI PythonCppAPI;

  QString PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorScriptedMorphologyEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedMorphologyEffectPrivate::qSlicerSegmentEditorScriptedMorphologyEffectPrivate()
{
  this->PythonCppAPI.declareMethod(Self::IconMethod, "icon");
  this->PythonCppAPI.declareMethod(Self::HelpTextMethod, "helpText");
  this->PythonCppAPI.declareMethod(Self::CloneMethod, "clone");
  this->PythonCppAPI.declareMethod(Self::ActivateMethod, "activate");
  this->PythonCppAPI.declareMethod(Self::DeactivateMethod, "deactivate");
  this->PythonCppAPI.declareMethod(Self::SetupOptionsFrameMethod, "setupOptionsFrame");
  this->PythonCppAPI.declareMethod(Self::CreateCursorMethod, "createCursor");
  this->PythonCppAPI.declareMethod(Self::ProcessInteractionEventsMethod, "processInteractionEvents");
  this->PythonCppAPI.declareMethod(Self::ProcessViewNodeEventsMethod, "processViewNodeEvents");
  this->PythonCppAPI.declareMethod(Self::SetMRMLDefaultsMethod, "setMRMLDefaults");
  this->PythonCppAPI.declareMethod(Self::EditedLabelmapChangedMethod, "editedLabelmapChanged");
  this->PythonCppAPI.declareMethod(Self::MasterVolumeNodeChangedMethod, "masterVolumeNodeChanged");
  this->PythonCppAPI.declareMethod(Self::LayoutChangedMethod, "layoutChanged");
  this->PythonCppAPI.declareMethod(Self::UpdateGUIFromMRMLMethod, "updateGUIFromMRML");
  this->PythonCppAPI.declareMethod(Self::UpdateMRMLFromGUIMethod, "updateMRMLFromGUI");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedMorphologyEffectPrivate::~qSlicerSegmentEditorScriptedMorphologyEffectPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorScriptedMorphologyEffect methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedMorphologyEffect::qSlicerSegmentEditorScriptedMorphologyEffect(QObject *parent)
  : Superclass(parent)
  , d_ptr(new qSlicerSegmentEditorScriptedMorphologyEffectPrivate)
{
  this->m_Name = QString("UnnamedScriptedMorphologyEffect");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedMorphologyEffect::~qSlicerSegmentEditorScriptedMorphologyEffect()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorScriptedMorphologyEffect::pythonSource()const
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  return d->PythonSource;
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScriptedMorphologyEffect::setPythonSource(const QString newPythonSource)
{
  Q_D(qSlicerSegmentEditorScriptedMorphologyEffect);

  if (!Py_IsInitialized())
    {
    return false;
    }

  if (!newPythonSource.endsWith(".py") && !newPythonSource.endsWith(".pyc"))
    {
    return false;
    }

  // Extract moduleName from the provided filename
  QString moduleName = QFileInfo(newPythonSource).baseName();

  // In case the effect is within the main module file
  QString className = moduleName;
  if (!moduleName.endsWith("Effect"))
    {
    className.append("Effect");
    }

  // Get a reference to the main module and global dictionary
  PyObject * main_module = PyImport_AddModule("__main__");
  PyObject * global_dict = PyModule_GetDict(main_module);

  // Get a reference (or create if needed) the <moduleName> python module
  PyObject * module = PyImport_AddModule(moduleName.toLatin1());

  // Get a reference to the python module class to instantiate
  PythonQtObjectPtr classToInstantiate;
  if (PyObject_HasAttrString(module, className.toLatin1()))
    {
    classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toLatin1()));
    }
  if (!classToInstantiate)
    {
    PythonQtObjectPtr local_dict;
    local_dict.setNewRef(PyDict_New());
    if (!qSlicerScriptedUtils::loadSourceAsModule(moduleName, newPythonSource, global_dict, local_dict))
      {
      return false;
      }
    if (PyObject_HasAttrString(module, className.toLatin1()))
      {
      classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toLatin1()));
      }
    }

  if (!classToInstantiate)
    {
    PythonQt::self()->handleError();
    PyErr_SetString(PyExc_RuntimeError,
                    QString("qSlicerSegmentEditorScriptedMorphologyEffect::setPythonSource - "
                            "Failed to load segment editor scripted effect: "
                            "class %1 was not found in %2").arg(className).arg(newPythonSource).toLatin1());
    PythonQt::self()->handleError();
    return false;
    }

  d->PythonCppAPI.setObjectName(className);

  PyObject* self = d->PythonCppAPI.instantiateClass(this, className, classToInstantiate);
  if (!self)
    {
    return false;
    }

  d->PythonSource = newPythonSource;

  if (!qSlicerScriptedUtils::setModuleAttribute(
        "slicer", className, self))
    {
    qCritical() << "Failed to set" << ("slicer." + className);
    }

  return true;
}

//-----------------------------------------------------------------------------
PyObject* qSlicerSegmentEditorScriptedMorphologyEffect::self() const
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  return d->PythonCppAPI.pythonSelf();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::setName(QString name)
{
  this->m_Name = name;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::setPerSegment(bool perSegment)
{
  this->m_PerSegment = perSegment;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentEditorScriptedMorphologyEffect::icon()
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->IconMethod);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::icon();
    }

  // Parse result
  QVariant resultVariant = PythonQtConv::PyObjToQVariant(result, QVariant::Icon);
  if (resultVariant.isNull())
    {
    return this->Superclass::icon();
    }
  return resultVariant.value<QIcon>();
}

//-----------------------------------------------------------------------------
const QString qSlicerSegmentEditorScriptedMorphologyEffect::helpText()const
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->HelpTextMethod);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::helpText();
    }

  // Parse result
  if (!PyString_Check(result))
    {
    qWarning() << d->PythonSource << ": qSlicerSegmentEditorScriptedMorphologyEffect: Function 'helpText' is expected to return a string!";
    return this->Superclass::helpText();
    }

  const char* role = PyString_AsString(result);
  return QString::fromLocal8Bit(role);
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorScriptedMorphologyEffect::clone()
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->CloneMethod);
  if (!result)
    {
    qCritical() << d->PythonSource << ": clone: Failed to call mandatory clone method! If it is implemented, please see python output for errors.";
    return NULL;
    }

  // Parse result
  QVariant resultVariant = PythonQtConv::PyObjToQVariant(result);
  qSlicerSegmentEditorAbstractEffect* clonedEffect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
    resultVariant.value<QObject*>() );
  if (!clonedEffect)
    {
    qCritical() << d->PythonSource << ": clone: Invalid cloned effect object returned from python!";
    return NULL;
    }
  return clonedEffect;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::activate()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::activate();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  d->PythonCppAPI.callMethod(d->ActivateMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::deactivate()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::deactivate();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  d->PythonCppAPI.callMethod(d->DeactivateMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::apply()
{
  // Notify editor about changes
  // This method needs to be called so that the changes are written back to the edited segment
  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::setupOptionsFrame()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::setupOptionsFrame();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->SetupOptionsFrameMethod);
}

//-----------------------------------------------------------------------------
QCursor qSlicerSegmentEditorScriptedMorphologyEffect::createCursor(qMRMLWidget* viewWidget)
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* arguments = PyTuple_New(1);
  PyTuple_SET_ITEM(arguments, 0, PythonQtConv::QVariantToPyObject(QVariant::fromValue<QObject*>((QObject*)viewWidget)));
  PyObject* result = d->PythonCppAPI.callMethod(d->CreateCursorMethod, arguments);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::createCursor(viewWidget);
    }

  // Parse result
  QVariant resultVariant = PythonQtConv::PyObjToQVariant(result, QVariant::Cursor);
  return resultVariant.value<QCursor>();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget)
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* arguments = PyTuple_New(3);
  PyTuple_SET_ITEM(arguments, 0, vtkPythonUtil::GetObjectFromPointer((vtkObject*)callerInteractor));
  PyTuple_SET_ITEM(arguments, 1, PyInt_FromLong(eid));
  PyTuple_SET_ITEM(arguments, 2, PythonQtConv::QVariantToPyObject(QVariant::fromValue<QObject*>((QObject*)viewWidget)));
  PyObject* result = d->PythonCppAPI.callMethod(d->ProcessInteractionEventsMethod, arguments);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::processInteractionEvents(callerInteractor, eid, viewWidget);
    }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget)
{
  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* arguments = PyTuple_New(3);
  PyTuple_SET_ITEM(arguments, 0, vtkPythonUtil::GetObjectFromPointer((vtkObject*)callerViewNode));
  PyTuple_SET_ITEM(arguments, 1, PyInt_FromLong(eid));
  PyTuple_SET_ITEM(arguments, 2, PythonQtConv::QVariantToPyObject(QVariant::fromValue<QObject*>((QObject*)viewWidget)));
  PyObject* result = d->PythonCppAPI.callMethod(d->ProcessViewNodeEventsMethod, arguments);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::processViewNodeEvents(callerViewNode, eid, viewWidget);
    }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::setMRMLDefaults()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::setMRMLDefaults();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->SetMRMLDefaultsMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::editedLabelmapChanged()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::editedLabelmapChanged();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->EditedLabelmapChangedMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::masterVolumeNodeChanged()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::masterVolumeNodeChanged();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->MasterVolumeNodeChangedMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::layoutChanged()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::layoutChanged();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->LayoutChangedMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::updateGUIFromMRML()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::updateGUIFromMRML();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->UpdateGUIFromMRMLMethod);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedMorphologyEffect::updateMRMLFromGUI()
{
  // Base class implementation needs to be called before the effect-specific one
  this->Superclass::updateMRMLFromGUI();

  Q_D(const qSlicerSegmentEditorScriptedMorphologyEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->UpdateMRMLFromGUIMethod);
}
