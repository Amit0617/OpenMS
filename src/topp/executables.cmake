### the directory name
set(directory source/APPLICATIONS/TOPP)

### list all filenames of the directory here
set(TOPP_executables
BaselineFilter
CometAdapter
CompNovo
CompNovoCID
ConsensusID
ConsensusMapNormalizer
CruxAdapter
DatabaseSuitability
Decharger
DTAExtractor
EICExtractor
ExternalCalibration
FalseDiscoveryRate
FeatureFinderCentroided
FeatureFinderIdentification
FeatureFinderIsotopeWavelet
FeatureFinderMetabo
FeatureFinderMRM
FeatureFinderMultiplex
FeatureLinkerLabeled
FeatureLinkerUnlabeled
FeatureLinkerUnlabeledKD
FeatureLinkerUnlabeledQT
FidoAdapter
FileConverter
FileFilter
FileInfo
FileMerger
FLASHDeconv
GenericWrapper
GNPSExport
HighResPrecursorMassCorrector
IDConflictResolver
IDFileConverter
IDFilter
IDMapper
IDMerger
IDPosteriorErrorProbability
IDRipper
IDRTCalibration
InclusionExclusionListCreator
InternalCalibration
IsobaricAnalyzer
LuciphorAdapter
MapAlignerIdentification
MapAlignerPoseClustering
MapAlignerSpectrum
MapAlignerTreeGuided
MapNormalizer
MapRTTransformer
MapStatistics
MaRaClusterAdapter
MascotAdapter
MascotAdapterOnline
MassTraceExtractor
MRMMapper
MSGFPlusAdapter
MyriMatchAdapter
MzTabExporter
NoiseFilterGaussian
NoiseFilterSGolay
OMSSAAdapter
OpenPepXL
OpenPepXLLF
OpenSwathAnalyzer
OpenSwathAssayGenerator
OpenSwathChromatogramExtractor
OpenSwathConfidenceScoring
OpenSwathDecoyGenerator
OpenSwathFeatureXMLToTSV
OpenSwathRTNormalizer
PeakPickerHiRes
PeakPickerWavelet
PepNovoAdapter
PeptideIndexer
PercolatorAdapter
PhosphoScoring
PrecursorIonSelector
PrecursorMassCorrector
ProteinInference
ProteinQuantifier
ProteinResolver
PTModel
PTPredict
QualityControl
RTModel
RTPredict
SeedListGenerator
SpecLibSearcher
SpectraFilterBernNorm
SpectraFilterMarkerMower
SpectraFilterNLargest
SpectraFilterNormalizer
SpectraFilterParentPeakMower
SpectraFilterScaler
SpectraFilterSqrtMower
SpectraFilterThresholdMower
SpectraFilterWindowMower
SpectraMerger
TextExporter
TOFCalibration
XFDR
XTandemAdapter
)

## all targets requiring OpenMS_GUI
set(TOPP_executables_with_GUIlib
ExecutePipeline
Resampler
)

### add filenames to Visual Studio solution tree
set(sources_VS)
foreach(i ${TOPP_executables} ${TOPP_executables_with_GUIlib})
	list(APPEND sources_VS "${i}.cpp")
endforeach(i)

source_group("" FILES ${sources_VS})
