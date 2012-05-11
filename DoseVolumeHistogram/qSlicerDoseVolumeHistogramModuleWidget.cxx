/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerDoseVolumeHistogramModuleWidget.h"
#include "ui_qSlicerDoseVolumeHistogramModule.h"

// ModuleTemplate includes
#include "vtkSlicerDoseVolumeHistogramLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkStringArray.h>
#include <vtkDoubleArray.h>

// STD includes
#include <set>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDoseVolumeHistogramModuleWidgetPrivate: public Ui_qSlicerDoseVolumeHistogramModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseVolumeHistogramModuleWidget);
protected:
  qSlicerDoseVolumeHistogramModuleWidget* const q_ptr;
public:
  qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget &object);
  vtkSlicerDoseVolumeHistogramLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic*
qSlicerDoseVolumeHistogramModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseVolumeHistogramModuleWidget);
  return vtkSlicerDoseVolumeHistogramLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::qSlicerDoseVolumeHistogramModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseVolumeHistogramModuleWidgetPrivate(*this) )
{
  m_ChartCheckboxToStructureSetNameMap.clear();
  m_ShowInChartCheckStates.clear();
  m_ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::~qSlicerDoseVolumeHistogramModuleWidget()
{
  m_ChartCheckboxToStructureSetNameMap.clear();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setup()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Hide widgets whose functions have not been implemented yet
  d->label_ImportCSV->setVisible(false);
  d->pushButton_ImportCSV->setVisible(false);

  d->tableWidget_ChartStatistics->setSortingEnabled(false);
  d->tableWidget_ChartStatistics->setSelectionMode(QAbstractItemView::NoSelection);

  d->label_NotDoseVolumeWarning->setText("");

  // Make connections
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_StructureSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( structureSetNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_Chart, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( chartNodeChanged(vtkMRMLNode*) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportMetricsToCsv, SIGNAL( clicked() ), this, SLOT( exportMetricsToCsv() ) );
  connect( d->checkBox_ShowHideAll, SIGNAL( stateChanged(int) ), this, SLOT( showHideAllCheckedStateChanged(int) ) );
  connect( d->checkBox_ShowVMetricsCc, SIGNAL( stateChanged(int) ), this, SLOT( showMetricsCheckedStateChanged(int) ) );
  connect( d->checkBox_ShowVMetricsPercent, SIGNAL( stateChanged(int) ), this, SLOT( showMetricsCheckedStateChanged(int) ) );
  connect( d->lineEdit_VDose, SIGNAL( textEdited(QString) ), this, SLOT( lineEditMetricEdited(QString) ) );
  connect( d->checkBox_ShowDMetrics, SIGNAL( stateChanged(int) ), this, SLOT( showMetricsCheckedStateChanged(int) ) );
  connect( d->lineEdit_DVolume, SIGNAL( textEdited(QString) ), this, SLOT( lineEditMetricEdited(QString) ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  bool dvhCanBeComputed = d->logic()->GetDoseVolumeNode() && d->logic()->GetStructureSetModelNode();
  d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);

  bool vdMetricCanBeShown = (d->tableWidget_ChartStatistics->rowCount() > 0);
  d->checkBox_ShowVMetricsCc->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowVMetricsPercent->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowDMetrics->setEnabled(vdMetricCanBeShown);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;

  // If there is no chart node selected, disable all checkboxes
  if (d->logic()->GetChartNode() == NULL)
  {
    for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
    {
      it->first->setEnabled(false);
    }
    d->checkBox_ShowHideAll->setEnabled(false);

    return;
  }

  vtkStringArray* arraysInSelectedChart = d->logic()->GetChartNode()->GetArrays();
  m_ShowInChartCheckStates.clear();

  for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
  {
    it->first->setEnabled(true);
    it->first->blockSignals(true); // block signals for the checkboxes so that changing it do not toggle the visibility of the plot
    it->first->setChecked(false);

    for (int i=0; i<arraysInSelectedChart->GetNumberOfValues(); ++i)
    {
      if (arraysInSelectedChart->GetValue(i).compare(it->second.second) == 0)
      {
        it->first->setChecked(true);
        break;
      }
    }

    m_ShowInChartCheckStates.push_back(it->first->isChecked());

    it->first->blockSignals(false); // unblock signal for the checkbox in question
  }

  // Change show/hide all checkbox state
  d->checkBox_ShowHideAll->setEnabled(true);
  d->checkBox_ShowHideAll->blockSignals(true);
  if (arraysInSelectedChart->GetNumberOfValues() == 0)
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
  }
  else if (arraysInSelectedChart->GetNumberOfValues() == m_ChartCheckboxToStructureSetNameMap.size())
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
  }
  else
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
  }
  d->checkBox_ShowHideAll->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLVolumeNode* volumeNode = dynamic_cast<vtkMRMLVolumeNode*>(node);
  if (volumeNode)
  {
    d->logic()->SetDoseVolumeNode(volumeNode);
    updateButtonsState();

    if (d->logic()->DoseVolumeContainsDose())
    {
      d->label_NotDoseVolumeWarning->setText("");
    }
    else
    {
      d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::structureSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->logic()->SetStructureSetModelNode(node);
  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLChartNode* chartNode = dynamic_cast<vtkMRMLChartNode*>(node);
  if (chartNode)
  {
    d->logic()->SetChartNode(chartNode);
    updateButtonsState();
    updateChartCheckboxesState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onLogicModified()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (d->logic()->GetSceneChanged())
  {
    refreshDvhTable();

    d->logic()->SceneChangedOff();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::refreshDvhTable()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkCollection* dvhNodes = d->logic()->GetDvhDoubleArrayNodes();

  // Clear the table
  d->tableWidget_ChartStatistics->clear();

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;
  for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
  {
    QCheckBox* checkbox = it->first;
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );
    delete checkbox;
  }

  m_ChartCheckboxToStructureSetNameMap.clear();

  if (dvhNodes->GetNumberOfItems() < 1)
  {
    return;
  }

  std::vector<std::string> metricList;
  d->logic()->CollectMetricsForDvhNodes(dvhNodes, metricList);

  // Get requested V metrics
  std::vector<double> vDoseValues;
  if (d->checkBox_ShowVMetricsCc->isChecked() || d->checkBox_ShowVMetricsPercent->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValues);
  }
  int vColumnCount = (d->checkBox_ShowVMetricsCc->isChecked() ? vDoseValues.size() : 0) + (d->checkBox_ShowVMetricsPercent->isChecked() ? vDoseValues.size() : 0);

  // Get requested D metrics
  std::vector<double> dVolumeValues;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_DVolume, dVolumeValues);
  }

  // Set up the table
  d->tableWidget_ChartStatistics->setColumnCount(2 + metricList.size() + vColumnCount + dVolumeValues.size());
  QStringList headerLabels;
  headerLabels << "" << "Structure";
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    QString metricName(it->c_str());
    metricName = metricName.right( metricName.length() - vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size() );
    headerLabels << metricName;
  }
  for (std::vector<double>::iterator it = vDoseValues.begin(); it != vDoseValues.end(); ++it)
  {
    if (d->checkBox_ShowVMetricsCc->isChecked())
    {
      QString metricName = QString("V%1 (cc)").arg(*it);
      headerLabels << metricName;
      d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
    }
    if (d->checkBox_ShowVMetricsPercent->isChecked())
    {
      QString metricName = QString("V%1 (%)").arg(*it);
      headerLabels << metricName;
      d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
    }
  }
  for (std::vector<double>::iterator it = dVolumeValues.begin(); it != dVolumeValues.end(); ++it)
  {
    QString metricName = QString("D%1cc (Gy)").arg(*it);
    headerLabels << metricName;
    d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
  }
  d->tableWidget_ChartStatistics->setColumnWidth(0, 24);
  d->tableWidget_ChartStatistics->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_ChartStatistics->setRowCount(dvhNodes->GetNumberOfItems());

  // Fill the table
  for (int i=0; i<dvhNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast( dvhNodes->GetItemAsObject(i) );
    if (!dvhNode)
    {
      continue;
    }

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_ChartStatistics);
    checkbox->setToolTip(tr("Show/hide DVH plot of structure '%1' in selected chart").arg( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );

    // Store checkbox with the augmented structure set name and the double array ID
    std::string plotName( dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) );
    plotName.append( QString("(%1)").arg(i+1).toAscii().data() );
    dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME.c_str(), plotName.c_str());
    m_ChartCheckboxToStructureSetNameMap[checkbox] = std::pair<std::string, std::string>(plotName, dvhNode->GetID());

    d->tableWidget_ChartStatistics->setCellWidget(i, 0, checkbox);

    d->tableWidget_ChartStatistics->setItem(i, 1, new QTableWidgetItem( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ) );    

    // Add default metric values
    int col = 2;
    for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
    {
      QString metricValue( dvhNode->GetAttribute(it->c_str()) );
      if (metricValue.isEmpty())
      {
        ++col;
        continue;
      }

      d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
      ++col;
    }

    // Add V metric values
    if (vDoseValues.size() > 0)
    {
      std::vector<double> volumes;
      std::vector<double> percents;
      d->logic()->ComputeVMetrics(dvhNode, vDoseValues, volumes, percents);
      if (volumes.size() != percents.size())
      {
        std::cerr << "Error: V metric result mismatch!" << std::endl;
        continue;
      }
      col = 2 + metricList.size();

      for (int j=0; j<volumes.size(); ++j)
      {
        if (d->checkBox_ShowVMetricsCc->isChecked())
        {
          QString metricValue;
          metricValue.setNum(volumes[j], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
          ++col;
        }
        if (d->checkBox_ShowVMetricsPercent->isChecked())
        {
          QString metricValue;
          metricValue.setNum(percents[j], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
          ++col;
        }
      }
    }

    // Add D metric values
    if (dVolumeValues.size() > 0)
    {
      std::vector<double> doses;
      d->logic()->ComputeDMetrics(dvhNode, dVolumeValues, doses);
      col = 2 + metricList.size() + vColumnCount;
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        QString metricValue;
        metricValue.setNum((*it), 'f', 2);
        d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
        ++col;
      }
    }
  }

  updateButtonsState();
  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the DVH for the selected structure set using the selected dose volume
  d->logic()->ComputeDvh();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());

  if (!senderCheckbox)
  {
    std::cerr << "Error: Invalid sender checkbox for show/hide in chart checkbox state change" << std::endl;
    return;
  }

  if (aState)
  {
    d->logic()->AddDvhToSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str(), m_ChartCheckboxToStructureSetNameMap[senderCheckbox].second.c_str());
  }
  else
  {
    d->logic()->RemoveDvhFromSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str());
  }

  if (!m_ShowHideAllClicked)
  {
    // Update states vector
    std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator checkboxIt;
    std::vector<bool>::iterator stateIt;
    for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(), stateIt=m_ShowInChartCheckStates.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
    {
      if (checkboxIt->first == senderCheckbox)
      {
        (*stateIt) = (bool)aState;
      }
    }

    // Change show/hide all checkbox state
    d->checkBox_ShowHideAll->blockSignals(true);
    bool isThereChecked = false;
    bool isThereUnchecked = false;
    std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;
    for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
    {
      if (it->first->isChecked())
      {
        isThereChecked = true;
      }
      else
      {
        isThereUnchecked = true;
      }
    }
    if (isThereChecked && isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
    }
    else if (isThereChecked && !isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
    }
    else
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
    }
    d->checkBox_ShowHideAll->blockSignals(false);
  }

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showHideAllCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator checkboxIt;

  m_ShowHideAllClicked = true;

  if (aState == Qt::PartiallyChecked)
  {
    std::vector<bool>::iterator stateIt;

    bool noneIsOn = true;
    bool noneIsOff = true;
    for (stateIt=m_ShowInChartCheckStates.begin(); stateIt!=m_ShowInChartCheckStates.end(); ++stateIt)
    {
      if (*stateIt)
      {
        noneIsOn = false;
      }
      else
      {
        noneIsOff = false;
      }
    }
    // If all the states are the same then advance to Checked state
    if (noneIsOn || noneIsOff)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
    }
    else // Else set the states one by one and leave it PartiallyChecked
    {
      for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(), stateIt=m_ShowInChartCheckStates.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
      {
        checkboxIt->first->setChecked(*stateIt);
      }
    }
  }
  else
  {
    bool state = (aState==Qt::Checked ? true : false);
    for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt)
    {
      checkboxIt->first->setChecked(state);
    }
  }

  m_ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportDvhToCsvClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // User selects file and format
  QString* selectedFilter = new QString();

	QString fileName = QFileDialog::getSaveFileName( NULL, QString( tr( "Save DVH values to CSV file" ) ), tr(""), QString( tr( "CSV with COMMA ( *.csv );;CSV with TAB ( *.csv )" ) ), selectedFilter );
	if ( fileName.isNull() )
	{
		return;
	}

	bool comma = true;
	if ( selectedFilter->compare( "CSV with TAB ( *.csv )" ) == 0 )
	{
		comma = false;
	}

	delete selectedFilter;

  // Export
  if (! d->logic()->ExportDvhToCsv(fileName.toAscii().data(), comma) )
  {
    std::cerr << "Error occured while exporting DVH to CSV!" << std::endl;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportMetricsToCsv()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // User selects file and format
  QString* selectedFilter = new QString();

	QString fileName = QFileDialog::getSaveFileName( NULL, QString( tr( "Save DVH metrics to CSV file" ) ), tr(""), QString( tr( "CSV with COMMA ( *.csv );;CSV with TAB ( *.csv )" ) ), selectedFilter );
	if ( fileName.isNull() )
	{
		return;
	}

	bool comma = true;
	if ( selectedFilter->compare( "CSV with TAB ( *.csv )" ) == 0 )
	{
		comma = false;
	}

	delete selectedFilter;

  // Get requested V metrics
  std::vector<double> vDoseValuesCc;
  if (d->checkBox_ShowVMetricsCc->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesCc);
  }
  std::vector<double> vDoseValuesPercent;
  if (d->checkBox_ShowVMetricsPercent->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesPercent);
  }

  // Get requested D metrics
  std::vector<double> dVolumeValues;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_DVolume, dVolumeValues);
  }

  // Export
  if (! d->logic()->ExportDvhMetricsToCsv(fileName.toAscii().data(), vDoseValuesCc, vDoseValuesPercent, dVolumeValues, comma) )
  {
    std::cerr << "Error occured while exporting DVH metrics to CSV!" << std::endl;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::getNumbersFromLineEdit(QLineEdit* aLineEdit, std::vector<double> &aValues)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  aValues.clear();

  if (!aLineEdit)
  {
    return;
  }

  QStringList valuesStringList = aLineEdit->text().split(',');
  QStringListIterator it(valuesStringList);
  while (it.hasNext())
  {
    QString nextValue( it.next() );

    bool toDoubleOk;
    double value = nextValue.toDouble(&toDoubleOk);
    if (toDoubleOk)
    {
      aValues.push_back(value);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditMetricEdited(QString aText)
{
  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showMetricsCheckedStateChanged(int aState)
{
  refreshDvhTable();
}
