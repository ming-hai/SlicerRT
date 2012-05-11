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

// .NAME vtkSlicerDoseVolumeHistogramLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseVolumeHistogramLogic_h
#define __vtkSlicerDoseVolumeHistogramLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// VTK includes
#include "vtkImageAccumulate.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLChartNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLChartViewNode;
class vtkMRMLDoubleArrayNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramLogic :
  public vtkSlicerModuleLogic
{
public:
  static const std::string DVH_TYPE_ATTRIBUTE_NAME;
  static const std::string DVH_TYPE_ATTRIBUTE_VALUE;
  static const std::string DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_LIST_ATTRIBUTE_NAME;
  static const char DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  static const std::string DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX;
  static const std::string DVH_METRIC_VOXEL_COUNT_ATTRIBUTE_NAME;
  static const std::string DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX;

public:
  static vtkSlicerDoseVolumeHistogramLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute DVH for the selected structure set based on the selected dose volume
  void ComputeDvh();

  /// Add dose volume histogram of a structure (ROI) to the selected chart given its plot name (including table row number) and the corresponding DVH double array node ID
  void AddDvhToSelectedChart(const char* structurePlotName, const char* dvhArrayNodeId);

  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromSelectedChart(const char* dvhArrayNodeId);

  /// Compute V metrics for the given DVH using the given dose values and put them in the vMetricsCc and vMetricsPercent output lists
  void ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetricsCc, std::vector<double> &vMetricsPercent);

  /// Compute D metrics for the given DVH using the given volume sizes and put them in the dMetrics output list
  void ComputeDMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> volumeSizes, std::vector<double> &dMetrics);

  /// Return false if the dose volume contains a volume that is really a dose volume
  bool DoseVolumeContainsDose();

  /// Refreshes DVH double array MRML node vector from the scene
  void RefreshDvhDoubleArrayNodesFromScene();

  /// Export DVH values 
  /// \param comma Flag determining if the CSV file to be saved is deliminated using commas or tabs (regional considerations)
  /// \return True if file written and saved successfully, false otherwise
  bool ExportDvhToCsv(const char* fileName, bool comma=true);

  /// Export DVH metrics
  /// \param vDoseValuesCc List of doses for V(cc) metrics to be computed and exported
  /// \param vDoseValuesPercent List of doses for V(%) metrics to be computed and exported
  /// \param dVolumeValues List of volume sizes for D metrics to be computed and exported
  /// \param comma Flag determining if the CSV file to be saved is deliminated using commas or tabs (regional considerations)
  /// \return True if file written and saved successfully, false otherwise
  bool ExportDvhMetricsToCsv(const char* fileName, std::vector<double> vDoseValuesCc, std::vector<double> vDoseValuesPercent, std::vector<double> dVolumeValues, bool comma=true);

  /// Collect DVH metrics from a collection of DVH double array nodes and try to order some of them
  static void CollectMetricsForDvhNodes(vtkCollection* dvhNodeCollection, std::vector<std::string> &metricList);

public:
  void SetDoseVolumeNode( vtkMRMLVolumeNode* );
  void SetStructureSetModelNode( vtkMRMLNode* );
  void SetChartNode( vtkMRMLChartNode* );

  vtkGetObjectMacro( DoseVolumeNode, vtkMRMLVolumeNode );
  vtkGetObjectMacro( StructureSetModelNode, vtkMRMLNode );
  vtkGetObjectMacro( ChartNode, vtkMRMLChartNode );
  vtkGetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

  vtkSetMacro( SceneChanged, bool );
  vtkGetMacro( SceneChanged, bool );
  vtkBooleanMacro( SceneChanged, bool );

protected:
  vtkSlicerDoseVolumeHistogramLogic();
  virtual ~vtkSlicerDoseVolumeHistogramLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Compute DVH for the given volume (which is the selected dose volume stenciled with a structure) with the given structure model node
  void ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode);

  /// Get stenciled dose volume for a structure (ROI)
  virtual void GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModel);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

  /// Get selected structure model nodes (expands a hierarchy node if found)
  void GetSelectedStructureModelNodes(std::vector<vtkMRMLModelNode*> &structureModelNodes);

protected:
  vtkSetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

private:
  vtkSlicerDoseVolumeHistogramLogic(const vtkSlicerDoseVolumeHistogramLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramLogic&);               // Not implemented

protected:
  /// Selected dose volume MRML node object
  vtkMRMLVolumeNode* DoseVolumeNode;

  /// Selected structure set MRML node object. Can be model node or model hierarchy node
  vtkMRMLNode* StructureSetModelNode;

  /// Selected chart MRML node object
  vtkMRMLChartNode* ChartNode;

  /// List of all the DVH double array MRML nodes that are present in the scene
  vtkCollection* DvhDoubleArrayNodes;

  /// Flag indicating if the scene has recently changed (update of the module GUI needed)
  bool SceneChanged;
};

#endif
