import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorThresholdEffect(AbstractScriptedSegmentEditorEffect):
  """ ThresholdEffect is an Effect implementing the global threshold
      operation in the segment editor
      
      This is also an example for scripted effects, and some methods have no
      function. The methods that are not needed (i.e. the default implementation in
      qSlicerSegmentEditorAbstractEffect is satisfactory) can simply be omitted.
  """
  
  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)
    scriptedEffect.name = 'Threshold'

    # Effect-specific members
    self.timer = qt.QTimer()
    self.previewState = 0
    self.previewStep = 1
    self.previewSteps = 5
    self.timer.connect('timeout()', self.preview)
    
    self.previewPipelines = {}
    self.setupPreviewDisplay()

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(SegmentEditorThresholdEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Threshold.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()
    
  def helpText(self):
    return "Set segment based on threshold range.\nNote: this replaces the current values."

  def activate(self):
    # Save segment opacity and set it to zero
    import vtkSlicerSegmentationsModuleMRML
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
    displayNode = segmentationNode.GetDisplayNode()
    if displayNode is not None:
      self.segmentOpacity = displayNode.GetSegmentOpacity2DFill(segmentID)
      displayNode.SetSegmentOpacity2DFill(segmentID, 0)

    # Setup and start preview pulse
    self.setupPreviewDisplay()
    self.timer.start(200)
    
  def deactivate(self):
    # Restore segment opacity
    import vtkSlicerSegmentationsModuleMRML
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
    displayNode = segmentationNode.GetDisplayNode()
    if displayNode is not None:
      displayNode.SetSegmentOpacity2DFill(segmentID, self.segmentOpacity)

    # Clear preview pipeline and stop timer
    self.previewPipelines = {}
    self.timer.stop()

  def setupOptionsFrame(self):
    self.thresholdSliderLabel = qt.QLabel("Threshold Range:")
    self.thresholdSliderLabel.setToolTip("Set the range of the background values that should be labeled.")
    self.scriptedEffect.addOptionsWidget(self.thresholdSliderLabel)

    self.thresholdSlider = ctk.ctkRangeWidget()
    self.thresholdSlider.spinBoxAlignment = qt.Qt.AlignTop
    self.thresholdSlider.singleStep = 0.01
    self.scriptedEffect.addOptionsWidget(self.thresholdSlider)

    self.useForPaintButton = qt.QPushButton("Use For Paint")
    self.useForPaintButton.setToolTip("Transfer the current threshold settings to be used for labeling operations such as Paint and Draw.")
    self.scriptedEffect.addOptionsWidget(self.useForPaintButton)

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Apply current threshold settings to the label map.")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.useForPaintButton.connect('clicked()', self.onUseForPaint)
    self.thresholdSlider.connect('valuesChanged(double,double)', self.onThresholdValuesChanged)
    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def editedLabelmapChanged(self):
    pass # For the sake of example

  def masterVolumeNodeChanged(self):
    # Set scalar range of master volume image data to threshold slider
    import vtkSegmentationCore
    masterImageData = vtkSegmentationCore.vtkOrientedImageData()
    success = self.scriptedEffect.masterVolumeImageData(masterImageData)
    if success:
      lo, hi = masterImageData.GetScalarRange()
      self.thresholdSlider.minimum, self.thresholdSlider.maximum = lo, hi
      self.thresholdSlider.singleStep = (hi - lo) / 1000.

  def layoutChanged(self):
    self.setupPreviewDisplay()

  def processInteractionEvents(self, callerInteractor, eventId, viewWidget):
    pass # For the sake of example

  def processViewNodeEvents(self, callerViewNode, eventId, viewWidget):
    pass # For the sake of example

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("MinimumThreshold", 0.)
    self.scriptedEffect.setParameter("MaximumThreshold", 100.)

  def updateGUIFromMRML(self):
    self.thresholdSlider.blockSignals(True)
    self.thresholdSlider.setMinimumValue(self.scriptedEffect.doubleParameter("MinimumThreshold"))
    self.thresholdSlider.setMaximumValue(self.scriptedEffect.doubleParameter("MaximumThreshold"))
    self.thresholdSlider.blockSignals(False)

  def updateMRMLFromGUI(self):
    self.scriptedEffect.setParameter("MinimumThreshold", self.thresholdSlider.minimumValue)
    self.scriptedEffect.setParameter("MaximumThreshold", self.thresholdSlider.maximumValue)
 
  #
  # Effect specific methods (the above ones are the API methods to override)
  #
  def onThresholdValuesChanged(self,min,max):
    self.scriptedEffect.updateMRMLFromGUI()

  def onUseForPaint(self):
    self.scriptedEffect.setCommonParameter("PaintThreshold", 1)
    self.scriptedEffect.setCommonParameter("PaintThresholdMin", self.thresholdSlider.minimumValue)
    self.scriptedEffect.setCommonParameter("PaintThresholdMax", self.thresholdSlider.maximumValue)

  def onApply(self):
    try:
      # Get master volume image data
      import vtkSegmentationCore
      masterImageData = vtkSegmentationCore.vtkOrientedImageData()
      self.scriptedEffect.masterVolumeImageData(masterImageData)
      # Get edited labelmap
      editedLabelmap = self.scriptedEffect.parameterSetNode().GetEditedLabelmap()
      # Get parameters
      min = self.scriptedEffect.doubleParameter("MinimumThreshold")
      max = self.scriptedEffect.doubleParameter("MaximumThreshold")

      # Save state for undo
      #TODO:
      #self.undoRedo.saveState()

      # Perform thresholding
      thresh = vtk.vtkImageThreshold()
      thresh.SetInputData(masterImageData)
      thresh.ThresholdBetween(min, max)
      thresh.SetInValue(1)
      thresh.SetOutValue(0)
      thresh.SetOutputScalarType(editedLabelmap.GetScalarType())
      thresh.Update()
      editedLabelmap.DeepCopy(thresh.GetOutput())
    except IndexError:
      logging.error('apply: Failed to threshold master volume!')
      pass

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.apply()
    
    # De-select effect
    self.scriptedEffect.selectEffect("")

  def setupPreviewDisplay(self):
    # Clear previous pipelines before setting up the new ones
    self.previewPipelines = {}

    layoutManager = slicer.app.layoutManager()
    if layoutManager is None:
      return

    # Add a pipeline for each 2D slice view
    for sliceViewName in layoutManager.sliceViewNames():
      sliceWidget = layoutManager.sliceWidget(sliceViewName)
      renderer = self.scriptedEffect.renderer(sliceWidget)
      if renderer is None:
        logging.error("setupPreviewDisplay: Failed to get renderer!")
        continue

      # Create pipeline
      pipeline = PreviewPipeline()
      self.previewPipelines[sliceWidget] = pipeline

      # Add actor
      renderer.AddActor2D(pipeline.actor)
      self.scriptedEffect.addActor(sliceWidget, pipeline.actor)

  def preview(self):
    opacity = 0.5 + self.previewState / (2. * self.previewSteps)
    min = self.scriptedEffect.doubleParameter("MinimumThreshold")
    max = self.scriptedEffect.doubleParameter("MaximumThreshold")
    
    # Get color of edited segment
    import vtkSlicerSegmentationsModuleMRML
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    displayNode = segmentationNode.GetDisplayNode()
    if displayNode is None:
      logging.error("preview: Invalid segmentation display node!")
      color = [0.5,0.5,0.5]
    segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
    colorVtk = displayNode.GetSegmentColor(segmentID)
    color = [colorVtk.GetX(), colorVtk.GetY(), colorVtk.GetZ()]
    r,g,b = color

    # Set values to pipelines
    for sliceWidget in self.previewPipelines:
      pipeline = self.previewPipelines[sliceWidget]
      pipeline.lookupTable.SetTableValue(1,  r, g, b,  opacity)
      sliceLogic = sliceWidget.sliceLogic()
      backgroundLogic = sliceLogic.GetBackgroundLayer()
      pipeline.thresholdFilter.SetInputConnection(backgroundLogic.GetReslice().GetOutputPort())
      pipeline.thresholdFilter.ThresholdBetween(min, max)
      pipeline.actor.VisibilityOn()
      sliceWidget.sliceView().scheduleRender()
    
    self.previewState += self.previewStep
    if self.previewState >= self.previewSteps:
      self.previewStep = -1
    if self.previewState <= 0:
      self.previewStep = 1

#
# PreviewPipeline
#
class PreviewPipeline:
  """ Visualization objects and pipeline for each slice view for threshold preview
  """
  
  def __init__(self):
    self.lookupTable = vtk.vtkLookupTable()
    self.lookupTable.SetRampToLinear()
    self.lookupTable.SetNumberOfTableValues(2)
    self.lookupTable.SetTableRange(0, 1)
    self.lookupTable.SetTableValue(0,  0, 0, 0,  0)
    self.colorMapper = vtk.vtkImageMapToRGBA()
    self.colorMapper.SetOutputFormatToRGBA()
    self.colorMapper.SetLookupTable(self.lookupTable)
    self.thresholdFilter = vtk.vtkImageThreshold()
    self.thresholdFilter.SetInValue(1)
    self.thresholdFilter.SetOutValue(0)
    self.thresholdFilter.SetOutputScalarTypeToUnsignedChar()

    # Feedback actor
    self.mapper = vtk.vtkImageMapper()
    self.dummyImage = vtk.vtkImageData()
    self.dummyImage.AllocateScalars(vtk.VTK_UNSIGNED_INT, 1)
    self.mapper.SetInputData(self.dummyImage)
    self.actor = vtk.vtkActor2D()
    self.actor.VisibilityOff()
    self.actor.SetMapper(self.mapper)
    self.mapper.SetColorWindow(255)
    self.mapper.SetColorLevel(128)

    # Setup pipeline
    self.colorMapper.SetInputConnection(self.thresholdFilter.GetOutputPort())
    self.mapper.SetInputConnection(self.colorMapper.GetOutputPort())
