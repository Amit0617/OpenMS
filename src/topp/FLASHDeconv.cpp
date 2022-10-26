// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2022.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Kyowon Jeong, Jihyung Kim $
// $Authors: Kyowon Jeong, Jihyung Kim $
// --------------------------------------------------------------------------

#include <OpenMS/APPLICATIONS/TOPPBase.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHIda.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvAlgorithm.h>
#include <OpenMS/ANALYSIS/TOPDOWN/MassFeatureTrace.h>
#include <OpenMS/ANALYSIS/TOPDOWN/DeconvolvedSpectrum.h>
#include <QFileInfo>
#include <OpenMS/FORMAT/FileTypes.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/METADATA/SpectrumLookup.h>
#include <OpenMS/ANALYSIS/TOPDOWN/QScore.h>
#include <OpenMS/FORMAT/FLASHDeconvFeatureFile.h>
#include <OpenMS/FORMAT/FLASHDeconvSpectrumFile.h>
#include <OpenMS/FILTERING/TRANSFORMERS/SpectraMerger.h>

using namespace OpenMS;
using namespace std;

//#define DEBUG_EXTRA_PARAMTER

//-------------------------------------------------------------
// Doxygen docu
//-------------------------------------------------------------
/**
  @page TOPP_FLASHDeconv TOPP_FLASHDeconv

  @brief FLASHDeconv performs ultrafast deconvolution of top down proteomics MS datasets.
  FLASHDeconv takes mzML file as input and outputs deconvolved feature list (.tsv) and
  deconvolved spectra files (.tsv, .mzML, .msalign, .ms1ft).
  FLASHDeconv uses FLASHDeconvAlgorithm for spectral level deconvolution and MassFeatureTracer to detect mass features.
  Also for MSn spectra, the precursor masses (not peak m/zs) should be determined and assigned in most cases. This assignment
  can be done by tracking MSn-1 spectra deconvolution information. Thus FLASHDeconv class keeps MSn-1 spectra deconvolution information
  for a certain period for precursor mass assignment in DeconvolvedSpectrum class.
  In case of FLASHIda runs, this precursor mass assignment is done by FLASHIda. Thus FLASHDeconv class simply parses the log file
  from FLASHIda runs and pass the parsed information to DeconvolvedSpectrum class.
*/


class TOPPFLASHDeconv :
    public TOPPBase
{
public:
  TOPPFLASHDeconv() :
      TOPPBase("FLASHDeconv",
               "Ultra-fast high-quality deconvolution enables online processing of top-down MS data")
  {
  }

protected:
  // this function will be used to register the tool parameters
  // it gets automatically called on tool execution
  void registerOptionsAndFlags_() override
  {
    registerInputFile_("in", "<file>", "", "Input file (mzML)");
    setValidFormats_("in", ListUtils::create<String>("mzML"));

    #ifdef DEBUG_EXTRA_PARAMTER
    registerInputFile_("in_train", "<file>", "", "topPIC result *prsm.tsv file for QScore training", false, true);
    setValidFormats_("in_train", ListUtils::create<String>("tsv"));
    registerOutputFile_("out_train", "<file>", "", "train result csv file for QScore training by weka", false, true);
    setValidFormats_("out_train", ListUtils::create<String>("csv"));
    #endif

    registerInputFile_("in_log",
                       "<file>",
                       "",
                       "log file generated by FLASHIda (IDA*.log). Only needed for coupling with FLASHIda acquisition",
                       false,
                       true);

    setValidFormats_("in_log", ListUtils::create<String>("log"), false);

    registerOutputFile_("out",
                        "<file>",
                        "",
                        "Default output tsv file containing deconvolved features");
    setValidFormats_("out", ListUtils::create<String>("tsv"));

    registerOutputFileList_("out_spec", "<file for MS1, file for MS2, ...>", {""},
                            "Output tsv files containing deconvolved spectra (per MS level)", false);
    setValidFormats_("out_spec", ListUtils::create<String>("tsv"));

    registerOutputFile_("out_mzml", "<file>", "",
                        "Output mzml file containing deconvolved spectra (of all MS levels)", false);
    setValidFormats_("out_mzml", ListUtils::create<String>("mzML"));

    registerOutputFile_("out_annotated_mzml", "<file>", "",
                        "Output mzml file containing annotated spectra. For each annotated peak, monoisotopic mass, charge, and isotope index are stored as meta data. Unannotated peaks are also copied as well without meta data.", false);
    setValidFormats_("out_annotated_mzml", ListUtils::create<String>("mzML"));

    registerOutputFile_("out_promex",
                        "<file>",
                        "",
                        "Output ms1ft (promex compatible) file containing deconvolved spectra. Only for MS1 level",
                        false);
    setValidFormats_("out_promex", ListUtils::create<String>("ms1ft"), false);

    registerOutputFileList_("out_topFD",
                            "<file for MS1, file for MS2, ...>",
                            {""},
                            "Output msalign (topFD compatible) files containing deconvolved spectra (per MS level)."
                            " The file name for MSn should end with msn.msalign to be able to be recognized by TopPIC GUI. "
                            "For example, -out_topFD [name]_ms1.msalign [name]_ms2.msalign",
                            false);

    setValidFormats_("out_topFD", ListUtils::create<String>("msalign"), false);

    registerOutputFile_("out_topFD_feature",
                            "<file>",
                            "",
                            "Output feature (topFD compatible) file containing MS1 deconvolved features. MS1 feature file is necessary for TopPIC feature intensity output",
                            false);
    setValidFormats_("out_topFD_feature", ListUtils::create<String>("feature"), false);

    registerDoubleOption_("min_precursor_snr",
                          "<SNR value>",
                          1.0,
                          "Minimum precursor SNR (SNR within the precursor envelope range) for identification. Similar to precursor interference level, but more stringent."
                          "When FLASHIda log file is used, this parameter is ignored. Applied only for topFD msalign outputs.",
                          false,
                          false);

    registerIntOption_("mzml_mass_charge",
                       "<0:uncharged 1: +1 charged -1: -1 charged>",
                       0,
                       "Charge status of deconvolved masses in mzml output (specified by out_mzml)",
                       false);

    setMinInt_("mzml_mass_charge", -1);
    setMaxInt_("mzml_mass_charge", 1);

    /*
    registerIntOption_("mzml_output_undeconvolved_peaks",
                       "<0:do not output undeconvolved peaks 1: output undeconvolved peaks>",
                       0,
                       "The undeconvolved raw peaks in the input spectra are output in mzML output.",
                       false,
                       true);

    setMinInt_("mzml_output_undeconvolved_peaks", 0);
    setMaxInt_("mzml_output_undeconvolved_peaks", 1);
*/
    registerIntOption_("preceding_MS1_count",
                       "<number>",
                       3,
                       "Specifies the number of preceding MS1 spectra for MS2 precursor determination. "
                       "In TDP, the precursor peak of a MS2 spectrum may not belong to any "
                       "deconvolved masses in the MS1 spectrum immediately preceding the MS2 spectrum. "
                       "Increasing this parameter to N allows for the search for the deconvolved masses in the N preceding MS1 spectra from the MS2 spectrum"
                       ", increasing the chance that its precursor is deconvolved.",
                       false,
                       false);

    setMinInt_("preceding_MS1_count", 1);

    registerIntOption_("write_detail",
                       "<1:true 0:false>",
                       0,
                       "To write peak information per deconvolved mass in detail or not in tsv files for deconvolved spectra. "
                       "If set to 1, all peak information (m/z, intensity, charge, "
                       "and isotope index) per mass is reported.",
                       false,
                       false);

    setMinInt_("write_detail", 0);
    setMaxInt_("write_detail", 1);

    registerIntOption_("max_MS_level", "<number>", 3, "Maximum MS level (inclusive) for deconvolution.", false, true);
    setMinInt_("max_MS_level", 1);

    registerIntOption_("forced_MS_level",
                       "",
                       0,
                       "If set to an integer N, MS level of all spectra will be set to N regardless of original MS level. Useful when deconvolving datasets containing only MS2 spectra.",
                       false,
                       true);
    setMinInt_("forced_MS_level", 0);

    registerIntOption_("merging_method", "<0: None 1: gaussian averaging 2: block method>", 0,
                                            "Method for spectra merging before deconvolution. 0: No merging "
                                                                "1: Average gaussian method to perform moving gaussian averaging of spectra per MS level. Effective to increase proteoform ID sensitivity "
                       "(in particular for Q-TOF datasets). "
                                                                "2: Block method to perform merging of all spectra into a single one per MS level (e.g., for NativeMS datasets)", false);
    setMinInt_("merging_method", 0);
    setMaxInt_("merging_method", 2);

    registerIntOption_("report_FDR", "<0: Do not report 1: report>", 0, "Report qvalues (roughly, mass-wise FDR) for deconvolved masses in the tsv files for deconvolved spectra. Decoy masses to calculate qvalues and FDR are also reported. Beta version.", false, false);
    setMinInt_("report_FDR", 0);
    setMaxInt_("report_FDR", 1);

    /*
        registerIntOption_("isobaric_labeling_option",
                           "",
                           -1,
                           "-1: none (default), 0: manual reporter ions m/zs with -isobaric_mz option, 1: IodoTMT6",
                           false,
                           false);

        registerDoubleList_("isobaric_mz",
                            "",
                            DoubleList{},
                            "isobaric reporter ion m/zs. If -isobaric_labeling_option 0 is used ,these m/zs intensities will be reported.",
                            false,
                            false);


    registerStringOption_("target_mass", "<target monoisotopic masses or file name containing target masses>", "", "Target monoisotopic masses for deconvolution or a txt file containing target masses. Masses are separated by commas. "
                                                                                                               "For instance, 100.0,200.0 will target 100.0 and 200.0 Da masses. "
                                                                                                                   "A plane text file containing the same target mass information may be used instead. "
                                                                                                                   "For each targeted mass, FLASHDeconv attempts to find the mass from input spectrum file. "
                                                                                                                   "If spectral peaks corresponding to the target mass, the target mass will be reported regardless of its quality (e.g., IsotopeCosine score). ", false, false);
  */
    registerIntOption_("use_RNA_averagine", "", 0, "If set to 1, RNA averagine model is used", false, true);
    setMinInt_("use_RNA_averagine", 0);
    setMaxInt_("use_RNA_averagine", 1);

    Param fd_defaults = FLASHDeconvAlgorithm().getDefaults();
    fd_defaults.setValue("tol", DoubleList{10.0, 10.0, 10.0}, "ppm tolerance for MS1, MS2, ... ");
    fd_defaults.setValue("min_charge", 1);
    fd_defaults.setValue("max_charge", 100);
    fd_defaults.setValue("min_mz", -1.0);
    fd_defaults.addTag("min_mz", "advanced");
    fd_defaults.setValue("max_mz", -1.0);
    fd_defaults.addTag("max_mz", "advanced");
    fd_defaults.setValue("min_rt", -1.0);
    fd_defaults.addTag("min_rt", "advanced");
    fd_defaults.setValue("max_rt", -1.0);
    fd_defaults.addTag("max_rt", "advanced");
    fd_defaults.setValue("min_mass", 50.0);
    fd_defaults.setValue("max_mass", 100000.0);
    //fd_defaults.addTag("tol", "advanced"); // hide entry
    fd_defaults.setValue("min_intensity", 10.0, "Intensity threshold");
    fd_defaults.addTag("min_intensity", "advanced");
    fd_defaults.setValue("min_isotope_cosine",
                         DoubleList{.85, .85, .85},
                         "Cosine similarity thresholds "
                         "between avg. and observed isotope patterns for MS1, 2, ... "
                         "(e.g., -min_isotope_cosine 0.8 0.6 to specify 0.8 and 0.6 for MS1 and MS2, respectively)");
    /*fd_defaults.setValue("max_mass_count",
                         IntList{-1, -1, -1},
                         "Maximum mass counts per spec for MS1, 2, ... "
                         "(e.g., -max_mass_count_ 100 50 to specify 100 and 50 for MS1 and MS2, respectively. -1 specifies unlimited)");
    fd_defaults.addTag("max_mass_count", "advanced");

    fd_defaults.remove("max_mass_count");*/
    //fd_defaults.remove("min_mass_count");

    Param mf_defaults = MassFeatureTrace().getDefaults();
    mf_defaults.setValue("min_isotope_cosine",
                         -1.0,
                         "Cosine similarity threshold between avg. and observed isotope pattern "
                         "for mass features. if not set, controlled by -Algorithm:min_isotope_cosine_ option");
    mf_defaults.addTag("min_isotope_cosine", "advanced");
    mf_defaults.remove("noise_threshold_int");
    mf_defaults.remove("reestimate_mt_sd");
    mf_defaults.remove("trace_termination_criterion");
    mf_defaults.remove("trace_termination_outliers");
    mf_defaults.remove("chrom_peak_snr");

    DoubleList tols = fd_defaults.getValue("tol");
    mf_defaults.setValue("mass_error_ppm", -1.0, "Feature tracing mass ppm tolerance. When negative, MS1 tolerance for mass deconvolution will be used (e.g., 16 ppm is used when -Algorithm:tol 16).");
    mf_defaults.setValue("min_sample_rate", 0.05);

    /*
    Param sm_defaults = SpectraMerger().getDefaults();
    sm_defaults.remove("mz_binning_width");
    sm_defaults.remove("mz_binning_width_unit");
    sm_defaults.remove("sort_blocks");

    sm_defaults.remove("average_gaussian:spectrum_type");
    sm_defaults.remove("average_gaussian:rt_FWHM");
    sm_defaults.remove("average_gaussian:cutoff");
    sm_defaults.remove("block_method:rt_max_length");

    sm_defaults.remove("average_gaussian:ms_level");
    sm_defaults.remove("block_method:ms_levels");

    sm_defaults.removeAll("precursor_method");
    sm_defaults.removeAll("average_tophat");
*/
    Param combined;

    combined.insert("Algorithm:", fd_defaults);
    combined.insert("FeatureTracing:", mf_defaults);
    //combined.insert("SpectraMerger:", sm_defaults);

    registerFullParam_(combined);
  }

  static void filterLowPeaks(MSExperiment& map, Size count)
  {
    for (auto & it : map)
    {
      if(it.size()<=count)
      {
        continue;
      }
      it.sortByIntensity(true);

      while(it.size()>count)
      {
        it.pop_back();
      }
      it.sortByPosition();
    }
  }

  static std::vector<double> getTargetMasses(String targets)
  {
    vector<double> result;
    if(targets.empty())
    {
      return result;
    }

    String target_masses;
    if(isdigit(targets[0]))
    {
      target_masses = targets;
    }else // if it is a file
    {
      std::ifstream in_trainstream(targets);
      String line;
      while (std::getline(in_trainstream, line))
      {
        target_masses.append(line);
      }
    }

    std::cout<<"Monoisotopic masses ";
    stringstream s_stream(target_masses); //create string stream from the string
    while(s_stream.good()) {
      String substr;
      getline(s_stream, substr, ','); //get first string delimited by comma
      result.push_back(std::stod(substr));
      std::cout<<std::stod(substr)<< " ";
    }
    std::cout<<" Da are targeted." << std::endl;
    return result;
  }

  // the main_ function is called after all parameters are read
  ExitCodes main_(int, const char **) override
  {
    OPENMS_LOG_INFO << "Initializing ... " << endl;
    const bool write_detail_qscore_att = false;

    //-------------------------------------------------------------
    // parsing parameters
    //-------------------------------------------------------------

    const Size max_peak_count_ = 30000;
    String in_file = getStringOption_("in");
    String out_file = getStringOption_("out");
    String in_train_file = "";
    String in_log_file = getStringOption_("in_log");
    String out_train_file = "";

    String out_att_file = out_file + ".csv";
#ifdef DEBUG_EXTRA_PARAMTER
    in_train_file = getStringOption_("in_train");
    out_train_file = getStringOption_("out_train");
#endif

    auto out_spec_file = getStringList_("out_spec");
    String out_mzml_file = getStringOption_("out_mzml");
    String out_anno_mzml_file = getStringOption_("out_annotated_mzml");
    String out_promex_file = getStringOption_("out_promex");
    auto out_topfd_file = getStringList_("out_topFD");
    auto out_topfd_feature_file = getStringOption_("out_topFD_feature");
    double topFD_SNR_threshold = //in_log_file.length() > 0 ? .0 :
                                                            getDoubleOption_("min_precursor_snr");
    bool use_RNA_averagine = getIntOption_("use_RNA_averagine") > 0;
    int max_ms_level = getIntOption_("max_MS_level");
    int forced_ms_level = getIntOption_("forced_MS_level");
    int merge = getIntOption_("merging_method");
    bool write_detail = getIntOption_("write_detail") > 0;
    int mzml_charge = getIntOption_("mzml_mass_charge");
    bool report_decoy = getIntOption_("report_FDR") == 1;
   // bool out_undeconvolved = getIntOption_("mzml_output_undeconvolved_peaks") == 1;
    double min_mz = getDoubleOption_("Algorithm:min_mz");
    double max_mz = getDoubleOption_("Algorithm:max_mz");
    double min_rt = getDoubleOption_("Algorithm:min_rt");
    double max_rt = getDoubleOption_("Algorithm:max_rt");
    //String targets = getStringOption_("target_mass");

    #ifdef DEBUG_EXTRA_PARAMTER
    auto out_topfd_file_log =  out_topfd_file[1] + ".log";
    fstream f_out_topfd_file_log;
    f_out_topfd_file_log.open(out_topfd_file_log, fstream::out);

    in_train_file = getStringOption_("in_train");
    out_train_file = getStringOption_("out_train");
    fstream fi_out;
    fi_out.open(in_file + ".txt", fstream::out); //
    #endif

    fstream out_stream, out_train_stream, out_promex_stream, out_att_stream, out_topfd_feature_stream;
    std::vector<fstream> out_spec_streams, out_topfd_streams;

    out_stream.open(out_file, fstream::out);
#ifdef DEBUG_EXTRA_PARAMTER
    out_att_stream.open(out_att_file, fstream::out);

    QScore::writeAttCsvFromDecoyHeader(out_att_stream);
#endif
    FLASHDeconvFeatureFile::writeHeader(out_stream);

    if (!out_promex_file.empty())
    {
      out_promex_stream.open(out_promex_file, fstream::out);
      FLASHDeconvFeatureFile::writePromexHeader(out_promex_stream);
    }

    if (!out_topfd_feature_file.empty())
    {

      out_topfd_feature_stream.open(out_topfd_feature_file, fstream::out);

      FLASHDeconvFeatureFile::writeTopFDFeatureHeader(out_topfd_feature_stream);
    }

    if (!out_topfd_file.empty())
    {
      out_topfd_streams = std::vector<fstream>(out_topfd_file.size());
      for (Size i = 0; i < out_topfd_file.size(); i++)
      {
        out_topfd_streams[i].open(out_topfd_file[i], fstream::out);
      }
    }
    if (!out_spec_file.empty())
    {
      out_spec_streams = std::vector<fstream>(out_spec_file.size());
      for (Size i = 0; i < out_spec_file.size(); i++)
      {
        out_spec_streams[i].open(out_spec_file[i], fstream::out);
        FLASHDeconvSpectrumFile::writeDeconvolvedMassesHeader(out_spec_streams[i], i + 1, write_detail, report_decoy);

      }
    }

    std::unordered_map<int, FLASHDeconvHelperStructs::TopPicItem> top_pic_map;

    if (!in_train_file.empty() && !out_train_file.empty())
    {
      out_train_stream.open(out_train_file, fstream::out);

      QScore::writeAttCsvFromTopPICHeader(out_train_stream, write_detail_qscore_att);
      std::ifstream in_trainstream(in_train_file);
      String line;
      bool start = false;
      while (std::getline(in_trainstream, line))
      {
        if (line.rfind("Data file name", 0) == 0)
        {
          start = true;
          continue;
        }
        if (!start)
        {
          continue;
        }

        auto tp = FLASHDeconvHelperStructs::TopPicItem(line);
        top_pic_map[tp.scan] = tp;
      }
      in_trainstream.close();
    }


    std::map<int, std::vector<std::vector<double>>> precursor_map_for_real_time_acquisition = FLASHIda::parseFLASHIdaLog(
        in_log_file); // ms1 scan -> mass, charge ,score, mz range, precursor int, mass int, color


    //-------------------------------------------------------------
    // reading input
    //-------------------------------------------------------------

    MSExperiment map;
    MzMLFile mzml;

    double expected_identification_count = .0;

    // feature number per input file
    int feature_cntr = 0;

    OPENMS_LOG_INFO << "Processing : " << in_file << endl;

    PeakFileOptions opt = mzml.getOptions();
    if(min_rt > 0 || max_rt > 0)
    {
      opt.setRTRange(DRange<1>{min_rt, max_rt});
    }
    if(min_mz > 0 || max_mz > 0)
    {
      opt.setMZRange(DRange<1>{min_mz, max_mz});
    }
    mzml.setLogType(log_type_);
    mzml.setOptions(opt);
    mzml.load(in_file, map);

    int current_max_ms_level = 0;

    auto spec_cntr = std::vector<int>(max_ms_level, 0);
    // spectrum number with at least one deconvolved mass per ms level per input file
    auto qspec_cntr = std::vector<int>(max_ms_level, 0);
    // mass number per ms level per input file
    auto mass_cntr = std::vector<int>(max_ms_level, 0);
    auto elapsed_deconv_cpu_secs = std::vector<double>(max_ms_level, .0);
    auto elapsed_deconv_wall_secs = std::vector<double>(max_ms_level, .0);
    std::map<int, double> scan_rt_map;
    std::map<int, PeakGroup> precursor_peak_groups; // MS2 scan number, peak group

    // read input dataset once to count spectra
    double gradient_rt = .0;
    int max_precursor_c = 0;
    for (auto& it: map)
    {
      gradient_rt = std::max(gradient_rt, it.getRT());

      // if forced_ms_level > 0, force MS level of all spectra to 1.
      if (forced_ms_level > 0)
      {
        it.setMSLevel(forced_ms_level);
      }

      if (it.empty())
      {
        continue;
      }

      if ((int) it.getMSLevel() > max_ms_level)
      {
        continue;
      }
      if(it.getMSLevel() ==2)
      {
        max_precursor_c =std::max(max_precursor_c, it.getPrecursors()[0].getCharge());
      }

      int ms_level = it.getMSLevel();
      current_max_ms_level = current_max_ms_level < ms_level ? ms_level : current_max_ms_level;

      if (min_rt > 0 && it.getRT() < min_rt)
      {
        continue;
      }
      if (max_rt > 0 && it.getRT() > max_rt)
      {
        break;
      }
    }
    //std::cout<<max_precursor_c<<std::endl;
    // Max MS Level is adjusted according to the input dataset
    current_max_ms_level = current_max_ms_level > max_ms_level ? max_ms_level : current_max_ms_level;

    // Run FLASHDeconv here

    int scan_number = 0;
    int num_last_deconvolved_spectra = getIntOption_("preceding_MS1_count");
    if (!in_log_file.empty())
    {
      num_last_deconvolved_spectra = 50; // if FLASHIda log file exists, keep up to 50 survey scans.
    }

    auto last_deconvolved_spectra = std::unordered_map<UInt, std::vector<DeconvolvedSpectrum>>();
    MSExperiment exp, exp_annotated;

    auto fd = FLASHDeconvAlgorithm();
    FLASHDeconvAlgorithm fd_charge_decoy, fd_noise_decoy;

    Param fd_param = getParam_().copy("Algorithm:", true);
    DoubleList tols = fd_param.getValue("tol");
    //fd_param.setValue("tol", getParam_().getValue("tol"));

    filterLowPeaks(map, max_peak_count_);

    // if a merged spectrum is analyzed, replace the input dataset with the merged one
    if (merge == 1)
    {
      OPENMS_LOG_INFO<< "Merging spectra using gaussian averaging... "<<std::endl;
      SpectraMerger merger;
      merger.setLogType(log_type_);
      Param sm_param = merger.getDefaults();
      sm_param.setValue("average_gaussian:precursor_mass_tol", tols[0]);
      sm_param.setValue("average_gaussian:precursor_max_charge", std::abs((int)fd_param.getValue("max_charge")));
      //sm_param.setValue("average_gaussian:rt_FWHM", 20.0); // for top down 10 seconds make sense.

      merger.setParameters(sm_param);
      map.sortSpectra();

      for (int tmp_ms_level = 1; tmp_ms_level <= current_max_ms_level; tmp_ms_level++)
      {
        merger.average(map, "gaussian", tmp_ms_level);
      }
    }
    else if(merge == 2)
    {
      //mz_binning_width - ms tols
      OPENMS_LOG_INFO<< "Merging spectra into a single spectrum per MS level... "<<std::endl;
      SpectraMerger merger;
      merger.setLogType(log_type_);
      Param sm_param = merger.getDefaults();
      sm_param.setValue("block_method:rt_block_size", (int)gradient_rt + 10);
      map.sortSpectra();

      for(int ml = 1; ml<=current_max_ms_level; ml++)
      {
        sm_param.setValue("mz_binning_width", tols[ml - 1]/2.0);
        sm_param.setValue("block_method:ms_levels", IntList{ml});
        merger.setParameters(sm_param);
        merger.mergeSpectraBlockWise(map);
      }

      fd_param.setValue("min_rt", .0);
      fd_param.setValue("max_rt", .0);
    }

    filterLowPeaks(map, max_peak_count_);

    fd.setParameters(fd_param);
    fd.calculateAveragine(use_RNA_averagine);
    //fd.setTargetMasses(getTargetMasses(targets));

    if(report_decoy)
    {
      fd_charge_decoy.setParameters(fd_param);
      fd_charge_decoy.setAveragine(fd.getAveragine());
      fd_charge_decoy.setDecoyFlag(1); // charge


      fd_noise_decoy.setParameters(fd_param);
      fd_noise_decoy.setAveragine(fd.getAveragine());
      fd_noise_decoy.setDecoyFlag(2); // noise

      fd.setDecoyFlag(3); // isotope
    }
    auto avg = fd.getAveragine();
    auto mass_tracer = MassFeatureTrace();
    Param mf_param = getParam_().copy("FeatureTracing:", true);
    DoubleList isotope_cosines = fd_param.getValue("min_isotope_cosine");
    //mf_param.setValue("mass_error_ppm", ms1tol);

    if(((double)mf_param.getValue("mass_error_ppm")) < 0)
    {
      mf_param.setValue("mass_error_ppm", tols[0]);
    }
    mf_param.setValue("noise_threshold_int", .0);
    mf_param.setValue("reestimate_mt_sd", "false");
    mf_param.setValue("trace_termination_criterion", "outlier");
    mf_param.setValue("trace_termination_outliers", 20);
    mf_param.setValue("chrom_peak_snr", .0);

    if (((double) mf_param.getValue("min_isotope_cosine")) < 0)
    {
      mf_param.setValue("min_isotope_cosine", isotope_cosines[0]);
    }
    mass_tracer.setParameters(mf_param);

    ProgressLogger progresslogger;
    progresslogger.setLogType(log_type_);
    progresslogger.startProgress(0, map.size(), "running FLASHDeconv");

    std::vector<DeconvolvedSpectrum> deconvolved_spectra;
    deconvolved_spectra.reserve(map.size());

    std::vector<DeconvolvedSpectrum> decoy_deconvolved_spectra;
    decoy_deconvolved_spectra.reserve(map.size() * 3);

    for (auto it = map.begin(); it != map.end(); ++it)
    {
      scan_number = SpectrumLookup::extractScanNumber(it->getNativeID(),
                                                      map.getSourceFiles()[0].getNativeIDTypeAccession());
      if (it->empty())
      {
        continue;
      }
      int ms_level = it->getMSLevel();
      if (ms_level > current_max_ms_level)
      {
        continue;
      }
      spec_cntr[ms_level - 1]++;
      auto deconv_begin = clock();
      auto deconv_t_start = chrono::high_resolution_clock::now();

      // for MS>1 spectrum, register precursor
      std::vector<DeconvolvedSpectrum> precursor_specs;

      if (ms_level > 1 && last_deconvolved_spectra.find(ms_level - 1) != last_deconvolved_spectra.end())
      {
        precursor_specs = (last_deconvolved_spectra[ms_level - 1]);
      }
      fd.performSpectrumDeconvolution(*it, precursor_specs, scan_number, write_detail, precursor_map_for_real_time_acquisition);
      auto& deconvolved_spectrum = fd.getDeconvolvedSpectrum();
      auto& decoy_deconvolved_spectrum = fd.getDecoyDeconvolvedSpectrum();

      if (deconvolved_spectrum.empty())
      {
        continue;
      }

      if (it->getMSLevel() > 1 && !deconvolved_spectrum.getPrecursorPeakGroup().empty())
      {
        precursor_peak_groups[scan_number] = deconvolved_spectrum.getPrecursorPeakGroup();
        if (deconvolved_spectrum.getPrecursorPeakGroup().getChargeSNR(deconvolved_spectrum.getPrecursorCharge()) >
            topFD_SNR_threshold)
        {
          expected_identification_count += deconvolved_spectrum.getPrecursorPeakGroup().getQScore();
        }
      }

      if (it->getMSLevel() == 2 && !in_train_file.empty() && !out_train_file.empty()
          && !deconvolved_spectrum.getPrecursorPeakGroup().empty()
          )
      {
        QScore::writeAttCsvFromTopPIC(deconvolved_spectrum, top_pic_map[scan_number], avg, out_train_stream, write_detail_qscore_att);
      }

      if (!out_mzml_file.empty())
      {
        if (!deconvolved_spectrum.empty())
        {
          auto dspec = deconvolved_spectrum.toSpectrum(mzml_charge, tols[ms_level-1], false);

          if(dspec.size() > 0)
          {
            exp.addSpectrum(dspec);
          }
        }
      }

      if(!out_anno_mzml_file.empty())
      {
        auto anno_spec( *it); // TODO make it the original input ...
        if (!deconvolved_spectrum.empty())
        {
          std::stringstream val{};

          for(auto& pg: deconvolved_spectrum)
          {
            val<<std::to_string(pg.getMonoMass())<<":";
            for(int k=0;k<pg.size();k++)
            {
              auto& p = pg[k];
              auto pindex = it->findNearest(p.mz);
              val<<pindex;
              if(k < pg.size() - 1){
                val<<",";
              }
            }
            val<<";";
          }
          anno_spec.setMetaValue("DeconvMassPeakIndices", val.str());
        }

        exp_annotated.addSpectrum(anno_spec);
      }

      if (ms_level < current_max_ms_level)
      {
        if ((int) last_deconvolved_spectra[ms_level].size() >= num_last_deconvolved_spectra)
        {
          last_deconvolved_spectra.erase(last_deconvolved_spectra.begin());
        }
        last_deconvolved_spectra[ms_level].push_back(deconvolved_spectrum);
      }

      if (merge != 2)
      {
        scan_rt_map[deconvolved_spectrum.getScanNumber()] = it->getRT();
      }

      if(report_decoy)
      {
        fd_charge_decoy.clearPreviouslyDeconvolvedMonoMasses();
        for(auto& pg: deconvolved_spectrum){
          for(int iso=-5;iso <=  (int)avg.getLastIndex(pg.getMonoMass()) + 2;iso++){
            fd_charge_decoy.addPreviouslyDeconvolvedMonoMass(pg.getMonoMass() + iso * Constants::ISOTOPE_MASSDIFF_55K_U);
          }
        }
        fd_charge_decoy.performSpectrumDeconvolution(*it, precursor_specs, scan_number, write_detail, precursor_map_for_real_time_acquisition);
        fd_noise_decoy.performSpectrumDeconvolution(*it, precursor_specs, scan_number, write_detail, precursor_map_for_real_time_acquisition);

        decoy_deconvolved_spectrum.reserve(decoy_deconvolved_spectrum.size() + fd_charge_decoy.getDecoyDeconvolvedSpectrum().size()
                                           + fd_noise_decoy.getDecoyDeconvolvedSpectrum().size());

        for(auto& pg: fd_charge_decoy.getDecoyDeconvolvedSpectrum())
        {
          decoy_deconvolved_spectrum.push_back(pg);
        }

        for(auto& pg: fd_noise_decoy.getDecoyDeconvolvedSpectrum())
        {
          decoy_deconvolved_spectrum.push_back(pg);
        }

        decoy_deconvolved_spectrum.sort();
        if(!write_detail)
        {
          for(auto& pg: decoy_deconvolved_spectrum)
          {
            pg.clear();
          }
        }
        decoy_deconvolved_spectra.push_back(decoy_deconvolved_spectrum);
      }

      qspec_cntr[ms_level - 1]++;
      mass_cntr[ms_level - 1] += deconvolved_spectrum.size();
      deconvolved_spectra.push_back(deconvolved_spectrum);

      elapsed_deconv_cpu_secs[ms_level - 1] += double(clock() - deconv_begin) / CLOCKS_PER_SEC;
      elapsed_deconv_wall_secs[ms_level - 1] += chrono::duration<double>(
                                                  chrono::high_resolution_clock::now() - deconv_t_start).count();


      progresslogger.nextProgress();
    }
    progresslogger.endProgress();

    std::cout<<" writing per spectrum deconvolution results ... "<<std::endl;

    DeconvolvedSpectrum::updatePeakGroupQvalues(deconvolved_spectra, decoy_deconvolved_spectra);

    for(auto& deconvolved_spectrum: deconvolved_spectra)
    {
      int ms_level = deconvolved_spectrum.getOriginalSpectrum().getMSLevel();

      if ((int) out_spec_streams.size() > ms_level - 1)
      {
        FLASHDeconvSpectrumFile::writeDeconvolvedMasses(deconvolved_spectrum, out_spec_streams[ms_level - 1], in_file, avg, write_detail, report_decoy);
        mass_tracer.storeInformationFromDeconvolvedSpectrum(deconvolved_spectrum);// add deconvolved mass in mass_tracer
      }
      if ((int) out_topfd_streams.size() > ms_level - 1)
      {
        FLASHDeconvSpectrumFile::writeTopFD(deconvolved_spectrum,out_topfd_streams[ms_level - 1], topFD_SNR_threshold);//, 1, (float)rand() / (float)RAND_MAX * 10 + 10);
#ifdef DEBUG_EXTRA_PARAMTER
        if(ms_level ==2 && !deconvolved_spectrum.getPrecursorPeakGroup().empty()){
          f_out_topfd_file_log << scan_number <<","<<deconvolved_spectrum.getPrecursorPeakGroup().getMonoMass()
                               <<","<<deconvolved_spectrum.getPrecursorPeakGroup().getRepAbsCharge()<<","
                               <<deconvolved_spectrum.getPrecursorPeakGroup().getIntensity()<<"\n";
        }
#endif
      }
#ifdef DEBUG_EXTRA_PARAMTER
      QScore::writeAttCsvFromDecoy(deconvolved_spectrum, out_att_stream);
#endif
    }
    if(report_decoy)
    {
      for (auto& deconvolved_spectrum : decoy_deconvolved_spectra)
      {
        int ms_level = deconvolved_spectrum.getOriginalSpectrum().getMSLevel();

        if ((int)out_spec_streams.size() > ms_level - 1)
        {
          FLASHDeconvSpectrumFile::writeDeconvolvedMasses(deconvolved_spectrum, out_spec_streams[ms_level - 1], in_file, avg, write_detail, report_decoy);
        }
#ifdef DEBUG_EXTRA_PARAMTER
        QScore::writeAttCsvFromDecoy(deconvolved_spectrum, out_att_stream);
#endif
      }
    }

    // mass_tracer run
    if (merge != 2) // unless spectra are merged into a single one
    {
      auto mass_features = mass_tracer.findFeatures(// !out_promex_file.empty(), !out_topfd_feature_file.empty(),
                              // precursor_peak_groups,
        fd.getAveragine()
                               //feature_cntr , out_stream, out_promex_stream, out_topfd_feature_streams
                               );
      feature_cntr = mass_features.size();
      if(feature_cntr > 0)
      {
        FLASHDeconvFeatureFile::writeFeatures(mass_features, in_file, out_stream);
      }
      if(!out_topfd_feature_file.empty())
      {
        FLASHDeconvFeatureFile::writeTopFDFeatures(mass_features, precursor_peak_groups, scan_rt_map, in_file,out_topfd_feature_stream);
      }

      if(!out_promex_file.empty())
      {
        FLASHDeconvFeatureFile::writePromexFeatures(mass_features, precursor_peak_groups, scan_rt_map, avg, out_promex_stream);
      }
    }
    if (!out_mzml_file.empty())
    {
      MzMLFile mzml_file;
      mzml_file.store(out_mzml_file, exp);
    }

    if(!out_anno_mzml_file.empty())
    {
      MzMLFile mzml_file;
      mzml_file.store(out_anno_mzml_file, exp_annotated);
    }

    for (int j = 0; j < (int) current_max_ms_level; j++)
    {
      if (spec_cntr[j] == 0)
      {
        continue;
      }

      if (merge == 2)
      {
        OPENMS_LOG_INFO << "So far, FLASHDeconv found " << mass_cntr[j] << " masses in the merged MS"
                        << (j + 1) << " spectrum" << endl;
      }
      else
      {
        OPENMS_LOG_INFO << "So far, FLASHDeconv found " << mass_cntr[j] << " masses in " << qspec_cntr[j]
                        << " MS" << (j + 1) << " spectra out of "
                        << spec_cntr[j] << endl;
      }
    }
    if (feature_cntr > 0)
    {
      OPENMS_LOG_INFO << "Mass tracer found " << feature_cntr << " features" << endl;
    }

    int total_spec_cntr = 0;
    for (int j = 0; j < (int) current_max_ms_level; j++)
    {
      total_spec_cntr += spec_cntr[j];

      OPENMS_LOG_INFO << "-- deconv per MS" << (j + 1)
                      << " spectrum (except spec loading, feature finding) [took "
                      << 1000.0 * elapsed_deconv_cpu_secs[j] / total_spec_cntr
                      << " ms (CPU), " << 1000.0 * elapsed_deconv_wall_secs[j] / total_spec_cntr
                      << " ms (Wall)] --"
                      << endl;
    }

    if (expected_identification_count > 0)
    {
      OPENMS_LOG_INFO << "Expected number of PrSMs: " << expected_identification_count << endl;
    }

    #ifdef DEBUG_EXTRA_PARAMTER
    f_out_topfd_file_log.close();
    fi_out.close();
    out_att_stream.close();
    #endif
    out_stream.close();

    if (!out_promex_file.empty())
    {
      out_promex_stream.close();
    }
    if (!out_topfd_feature_file.empty())
    {
        out_topfd_feature_stream.close();
    }

    if (!out_topfd_file.empty())
    {
      int j = 0;
      for (auto& out_topfd_stream: out_topfd_streams)
      {
        out_topfd_stream.close();
        if (j + 1 > current_max_ms_level)
        {
          std::remove(out_topfd_file[j].c_str());
        }
        j++;
      }
    }
    if (!out_spec_file.empty())
    {
      int j = 0;
      for (auto& out_spec_stream: out_spec_streams)
      {
        out_spec_stream.close();
        if (j + 1 > current_max_ms_level)
        {
          std::remove(out_spec_file[j].c_str());
        }
        j++;
      }
    }

    if (!out_train_file.empty())
    {
      out_train_stream.close();
    }

    return EXECUTION_OK;
  }

};

// the actual main function needed to create an executable
int main(int argc, const char **argv)
{
  TOPPFLASHDeconv tool;
  return tool.main(argc, argv);
}
