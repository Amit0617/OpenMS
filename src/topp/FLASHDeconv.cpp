// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2021.
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
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvAlgorithm.h>
#include <OpenMS/ANALYSIS/TOPDOWN/MassFeatureTrace.h>
#include <OpenMS/ANALYSIS/TOPDOWN/DeconvolutedSpectrum.h>
#include <QFileInfo>
#include <OpenMS/FORMAT/FileTypes.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/METADATA/SpectrumLookup.h>
#include <OpenMS/ANALYSIS/TOPDOWN/QScore.h>
#include <OpenMS/TRANSFORMATIONS/RAW2PEAK/PeakPickerHiRes.h>

using namespace OpenMS;
using namespace std;

//#define DEBUG_EXTRA_PARAMTER

//-------------------------------------------------------------
// Doxygen docu
//-------------------------------------------------------------
/**
  @page TOPP_FLASHDeconv TOPP_FLASHDeconv

  @brief FLASHDeconv performs ultrafast deconvolution of top down proteomics MS datasets.

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
    registerOutputFile_("out_train", "<file>", "", "train result csv file for QScore training", false, true);
    setValidFormats_("out_train", ListUtils::create<String>("csv"));
    #endif

    registerInputFile_("in_log",
                       "<file>",
                       "",
                       "log file generated by FLASHIda (IDA*.log). Only needed for coupling with FLASHIda acquisition",
                       false,
                       false);

    setValidFormats_("in_log", ListUtils::create<String>("log"), false);

    registerOutputFile_("out",
                        "<file>",
                        "",
                        "feature level deconvolution output tsv file (or ensemble spectrum level deconvluted mass if use_ensemble_spectrum is set to 1) ");
    setValidFormats_("out", ListUtils::create<String>("tsv"));

    registerOutputFileList_("out_spec", "<file for MS1, file for MS2, ...>", {""},
                            "spectrum level deconvolution output tsv file per MS level", false);
    setValidFormats_("out_spec", ListUtils::create<String>("tsv"));

    registerOutputFile_("out_mzml", "<file>", "",
                        "mzml format spectrum level deconvolution output file per MS level\"", false);
    setValidFormats_("out_mzml", ListUtils::create<String>("mzML"));

    registerOutputFile_("out_promex",
                        "<file>",
                        "",
                        "ms1ft (promex compatible) format spectrum level deconvolution output file only for MS1 level",
                        false);
    setValidFormats_("out_promex", ListUtils::create<String>("ms1ft"), false);

    registerOutputFileList_("out_topFD",
                            "<file for MS1, file for MS2, ...>",
                            {""},
                            "msalign (topFD compatible) format spectrum level deconvolution output file per MS level."
                            " The file name for MSn should end with msn.msalign to be able to be recognized by TopPIC GUI. "
                            "For example, -out_topFD [name]_ms1.msalign [name]_ms2.msalign",
                            false);

    registerOutputFileList_("out_topFD_feature",
                            "<file  for MS1, file for MS2, ...>",
                            {""},
                            "feature (topFD compatible) format spectrum level deconvolution output file per MS level. Feature file is necessary for TopPIC feature intensity output",
                            false);
    setValidFormats_("out_topFD_feature", ListUtils::create<String>("feature"), false);

    setValidFormats_("out_topFD", ListUtils::create<String>("msalign"), false);

    registerDoubleOption_("min_precursor_snr",
                          "<SNR value>",
                          0.0,
                          "minimum precursor SNR (SNR within the precursor envelope range) for identification. Now applied only for topFD outputs.",
                          false,
                          false);

    registerIntOption_("mzml_mass_charge",
                       "<0:uncharged 1: +1 charged -1: -1 charged>",
                       0,
                       "Charge status of deconvoluted masses in mzml output (specified by out_mzml)",
                       false);

    setMinInt_("mzml_mass_charge", -1);
    setMaxInt_("mzml_mass_charge", 1);

    registerIntOption_("preceding_MS1_count",
                       "<number>",
                       3,
                       "Specifies the number of preceding MS1 spectra for MS2 precursor determination. In TDP, some precursor peaks in MS2 are not part of "
                       "the deconvoluted masses in MS1 immediately preceding the MS2. In this case, increasing this parameter allows for the search in further preceding "
                       "MS1 spectra and helps determine exact precursor masses.",
                       false,
                       false);

    setMinInt_("preceding_MS1_count", 1);

    registerIntOption_("write_detail",
                       "<1:true 0:false>",
                       0,
                       "to write peak info per deconvoluted mass in detail or not in spectrum level tsv files. If set to 1, all peak information (m/z, intensity, charge, "
                       "and isotope index) per mass is reported.",
                       false,
                       false);

    setMinInt_("write_detail", 0);
    setMaxInt_("write_detail", 1);

    registerIntOption_("max_MS_level", "", 2, "maximum MS level (inclusive) for deconvolution", false, true);
    setMinInt_("max_MS_level", 1);


    registerIntOption_("use_ensemble_spectrum",
                       "",
                       0,
                       "if set to 1, all spectra will add up to a single ensemble spectrum (per MS level) "
                       "for which the deconvolution is performed. out_spec should specify the output spectrum level deconvolution tsv files."
                       "This mode is not fully developed yet.",
                       false,
                       false);

    setMinInt_("use_ensemble_spectrum", 0);
    setMaxInt_("use_ensemble_spectrum", 1);

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
    */

    registerIntOption_("use_RNA_averagine", "", 0, "if set to 1, RNA averagine model is used", false, true);
    setMinInt_("use_RNA_averagine", 0);
    setMaxInt_("use_RNA_averagine", 1);

    Param fd_defaults = FLASHDeconvAlgorithm().getDefaults();
    fd_defaults.setValue("tol", DoubleList{10.0, 10.0}, "ppm tolerance");
    fd_defaults.setValue("min_charge", 2);
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
    fd_defaults.setValue("min_peaks", IntList{3, 3});
    fd_defaults.addTag("min_peaks", "advanced");
    fd_defaults.setValue("min_intensity", 100.0, "intensity threshold");
    fd_defaults.addTag("min_intensity", "advanced");
    fd_defaults.setValue("min_isotope_cosine",
                         DoubleList{.85, .85},
                         "cosine similarity thresholds "
                         "between avg. and observed isotope patterns for MS1, 2, ... "
                         "(e.g., -min_isotope_cosine 0.8 0.6 to specify 0.8 and 0.6 for MS1 and MS2, respectively)");
    fd_defaults.setValue("max_mass_count",
                         IntList{-1, -1},
                         "maximum mass counts per spec for MS1, 2, ... "
                         "(e.g., -max_mass_count_ 100 50 to specify 100 and 50 for MS1 and MS2, respectively. -1 specifies unlimited)");
    fd_defaults.addTag("max_mass_count", "advanced");

    fd_defaults.setValue("rt_window",
                         180.0,
                         "RT window for MS1 deconvolution. Spectra within RT window are considered together for deconvolution."
                         "When an MS1 spectrum is deconvoluted, the masses found in previous MS1 spectra within RT window are favorably considered.");
    fd_defaults.addTag("rt_window", "advanced");

    fd_defaults.remove("max_mass_count");
    //fd_defaults.remove("min_mass_count");

    Param mf_defaults = MassFeatureTrace().getDefaults();
    mf_defaults.setValue("min_isotope_cosine",
                         -1.0,
                         "cosine similarity threshold between avg. and observed isotope pattern "
                         "for mass features. if not set, controlled by -Algorithm:min_isotope_cosine_ option");
    mf_defaults.addTag("min_isotope_cosine", "advanced");
    mf_defaults.remove("noise_threshold_int");
    mf_defaults.remove("reestimate_mt_sd");
    mf_defaults.remove("trace_termination_criterion");
    mf_defaults.remove("trace_termination_outliers");
    mf_defaults.remove("chrom_peak_snr");

    DoubleList tols = fd_defaults.getValue("tol");
    mf_defaults.setValue("mass_error_ppm", tols[0]);

    //mf_defaults.remove("mass_error_ppm"); // hide entry
    mf_defaults.setValue("min_sample_rate", 0.2);

    Param combined;
    combined.insert("Algorithm:", fd_defaults);
    combined.insert("FeatureTracing:", mf_defaults);
    registerFullParam_(combined);
  }


  static std::map<int, std::vector<std::vector<double>>> read_FLASHIda_log_(const String &in_log_file)
  {
    std::map<int, std::vector<std::vector<double>>> precursor_map_for_real_time_acquisition; // ms1 scan -> mass, charge ,score, mz range, precursor int, mass int, color
    if (!in_log_file.empty())
    {
      std::ifstream instream(in_log_file);
      if (instream.good())
      {
        String line;
        int scan;
        double mass, charge, w1, w2, qscore, pint, mint, z1, z2;
        double features[6];
        while (std::getline(instream, line))
        {
          if (line.find("0 targets") != line.npos)
          {
            continue;
          }
          if (line.hasPrefix("MS1"))
          {
            Size st = line.find("MS1 Scan# ") + 10;
            Size ed = line.find(' ', st);
            String n = line.substr(st, ed);
            scan = atoi(n.c_str());
            precursor_map_for_real_time_acquisition[scan] = std::vector<std::vector<double>>();//// ms1 scan -> mass, charge ,score, mz range, precursor int, mass int, color
          }
          if (line.hasPrefix("Mass"))
          {
            Size st = 5;
            Size ed = line.find('\t');
            String n = line.substr(st, ed);
            mass = atof(n.c_str());

            st = line.find("Z=") + 2;
            ed = line.find('\t', st);
            n = line.substr(st, ed);
            charge = atof(n.c_str());

            st = line.find("Score=") + 6;
            ed = line.find('\t', st);
            n = line.substr(st, ed);
            qscore = atof(n.c_str());

            st = line.find("[") + 1;
            ed = line.find('-', st);
            n = line.substr(st, ed);
            w1 = atof(n.c_str());

            st = line.find('-', ed) + 1;
            ed = line.find(']', st);
            n = line.substr(st, ed);
            w2 = atof(n.c_str());

            st = line.find("PrecursorIntensity=", ed) + 19;
            ed = line.find('\t', st);
            n = line.substr(st, ed);
            pint = atof(n.c_str());

            st = line.find("PrecursorMassIntensity=", ed) + 23;
            ed = line.find('\t', st);
            n = line.substr(st, ed);
            mint = atof(n.c_str());

            st = line.find("Features=", ed) + 9;
            //ed = line.find(' ', st);

            st = line.find('[', st) + 1;
            ed = line.find(',', st);
            n = line.substr(st, ed);
            features[0] = atof(n.c_str());

            st = line.find(',', st) + 1;
            ed = line.find(',', st);
            n = line.substr(st, ed);
            features[1] = atof(n.c_str());

            st = line.find(',', st) + 1;
            ed = line.find(',', st);
            n = line.substr(st, ed);
            features[2] = atof(n.c_str());

            st = line.find(',', st) + 1;
            ed = line.find(',', st);
            n = line.substr(st, ed);
            features[3] = atof(n.c_str());

            st = line.find(',', st) + 1;
            ed = line.find(',', st);
            n = line.substr(st, ed);
            features[4] = atof(n.c_str());

            st = line.find(',', st) + 1;
            ed = line.find(']', st);
            n = line.substr(st, ed);
            features[5] = atof(n.c_str());

            st = line.find("ChargeRange=[", ed) + 13;
            ed = line.find('-', st);
            n = line.substr(st, ed);
            z1 = atof(n.c_str());

            st = line.find("-", ed) + 1;
            ed = line.find(']', st);
            n = line.substr(st, ed);
            z2 = atof(n.c_str());
            std::vector<double> e(15);
            e[0] = mass;
            e[1] = charge;
            e[2] = qscore;
            e[3] = w1;
            e[4] = w2;
            e[5] = pint;
            e[6] = mint;
            e[7] = z1;
            e[8] = z2;
            for (int i = 9; i < 15; i++)
            {
              e[i] = features[i - 9];
            }
            precursor_map_for_real_time_acquisition[scan].push_back(e);
          }
        }
        instream.close();
      }
      else
      {
        std::cout << in_log_file << " not found\n";
      }
    }
    return precursor_map_for_real_time_acquisition;
  }

  // the main_ function is called after all parameters are read
  ExitCodes main_(int, const char **) override
  {
    OPENMS_LOG_INFO << "Initializing ... " << endl;
    const bool write_detail_qscore_att = false;

    //-------------------------------------------------------------
    // parsing parameters
    //-------------------------------------------------------------

    String in_file = getStringOption_("in");
    String out_file = getStringOption_("out");
    String in_train_file = "";//getStringOption_("in_train");
    String in_log_file = getStringOption_("in_log");
    String out_train_file = "";//getStringOption_("out_train");
    auto out_spec_file = getStringList_("out_spec");
    String out_mzml_file = getStringOption_("out_mzml");
    String out_promex_file = getStringOption_("out_promex");
    auto out_topfd_file = getStringList_("out_topFD");
    auto out_topfd_feature_file = getStringList_("out_topFD_feature");
    double topFD_SNR_threshold = getDoubleOption_("min_precursor_snr");
    bool use_RNA_averagine = getIntOption_("use_RNA_averagine") > 0;
    int max_ms_level = getIntOption_("max_MS_level");
    bool ensemble = getIntOption_("use_ensemble_spectrum") > 0;
    bool write_detail = getIntOption_("write_detail") > 0;
    int mzml_charge = getIntOption_("mzml_mass_charge");
    double min_rt = getDoubleOption_("Algorithm:min_rt");
    double max_rt = getDoubleOption_("Algorithm:max_rt");
    //DoubleList iso_mzs = getDoubleList_("isobaric_mz");

    // int iso_option = getIntOption_("isobaric_labeling_option");
    //if (iso_option == 1)// fix later..
    //{
    // iso_mzs = DoubleList{126.127725, 127.124760, 128.134433, 129.131468, 130.141141, 131.138176};
    //iso_mzs = DoubleList{114.127725, 115.124760, 116.134433, 117.131468, 118.141141, 119.138176};
    //}


    #ifdef DEBUG_EXTRA_PARAMTER
    auto out_topfd_file_log =  out_topfd_file[1] + ".log";
    fstream f_out_topfd_file_log;
    f_out_topfd_file_log.open(out_topfd_file_log, fstream::out);

    in_train_file = getStringOption_("in_train");
    out_train_file = getStringOption_("out_train");
    fstream fi_out;
    fi_out.open(in_file + ".txt", fstream::out); //
    #endif

    fstream out_stream, out_train_stream, out_promex_stream;
    std::vector<fstream> out_spec_streams, out_topfd_streams, out_topfd_feature_streams;

    out_stream.open(out_file, fstream::out);
    MassFeatureTrace::writeHeader(out_stream);

    if (!out_promex_file.empty())
    {
      out_promex_stream.open(out_promex_file, fstream::out);
      MassFeatureTrace::writePromexHeader(out_promex_stream);
    }

    if (!out_topfd_feature_file.empty())
    {
      out_topfd_feature_streams = std::vector<fstream>(out_topfd_feature_file.size());
      for (int i = 0; i < out_topfd_feature_file.size(); i++)
      {
        out_topfd_feature_streams[i].open(out_topfd_feature_file[i], fstream::out);
      }
      MassFeatureTrace::writeTopFDFeatureHeader(out_topfd_feature_streams);
    }

    if (!out_topfd_file.empty())
    {
      out_topfd_streams = std::vector<fstream>(out_topfd_file.size());
      for (int i = 0; i < out_topfd_file.size(); i++)
      {
        out_topfd_streams[i].open(out_topfd_file[i], fstream::out);
      }
    }
    if (!out_spec_file.empty())
    {
      out_spec_streams = std::vector<fstream>(out_spec_file.size());
      for (int i = 0; i < out_spec_file.size(); i++)
      {
        out_spec_streams[i].open(out_spec_file[i], fstream::out);
        DeconvolutedSpectrum::writeDeconvolutedMassesHeader(out_spec_streams[i], i + 1, write_detail);
      }
    }

    std::unordered_map<int, FLASHDeconvHelperStructs::TopPicItem> top_pic_map;

    if (!in_train_file.empty() && !out_train_file.empty())
    {
      out_train_stream.open(out_train_file, fstream::out);
      QScore::writeAttHeader(out_train_stream, write_detail_qscore_att);
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
        top_pic_map[tp.scan_] = tp;
      }
      in_trainstream.close();
    }


    std::map<int, std::vector<std::vector<double>>> precursor_map_for_real_time_acquisition = read_FLASHIda_log_(
        in_log_file); // ms1 scan -> mass, charge ,score, mz range, precursor int, mass int, color

    int current_max_ms_level = 0;

    auto spec_cntr = std::vector<int>(max_ms_level, 0);
    // spectrum number with at least one deconvoluted mass per ms level per input file
    auto qspec_cntr = std::vector<int>(max_ms_level, 0);
    // mass number per ms level per input file
    auto mass_cntr = std::vector<int>(max_ms_level, 0);
    // feature number per input file
    int feature_cntr = 0;

    // feature index written in the output file
    int feature_index = 0;

    MSExperiment ensemble_map;
    // generate ensemble spectrum if param.ensemble is set
    if (ensemble)
    {
      for (int i = 0; i < max_ms_level; i++)
      {
        auto spec = MSSpectrum();
        spec.setMSLevel(i + 1);
        std::ostringstream name;
        name << "ensemble MS" << (i + 1) << " spectrum";
        spec.setName(name.str());
        ensemble_map.addSpectrum(spec);
      }
    }

    //-------------------------------------------------------------
    // reading input
    //-------------------------------------------------------------

    MSExperiment map;
    MzMLFile mzml;

    // all for measure elapsed cup wall time
    double elapsed_cpu_secs = 0, elapsed_wall_secs = 0;
    auto elapsed_deconv_cpu_secs = std::vector<double>(max_ms_level, .0);
    auto elapsed_deconv_wall_secs = std::vector<double>(max_ms_level, .0);

    auto begin = clock();
    auto t_start = chrono::high_resolution_clock::now();

    OPENMS_LOG_INFO << "Processing : " << in_file << endl;

    mzml.setLogType(log_type_);
    mzml.load(in_file, map);

    //      double rtDuration = map[map.size() - 1].getRT() - map[0].getRT();
    int ms1_cntr = 0;
    double ms2_cntr = .0; // for debug...
    current_max_ms_level = 0;

    // read input dataset once to count spectra and generate ensemble spectrum if necessary
    for (auto &it: map)
    {
      if (it.empty())
      {
        continue;
      }
      if (it.getMSLevel() > max_ms_level)
      {
        continue;
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
      if (ensemble)
      {
        auto &espec = ensemble_map[it.getMSLevel() - 1];
        for (auto &p: it)
        {
          espec.push_back(p);
        }
      }

      if (it.getMSLevel() == 1)
      {
        ms1_cntr++;
      }
      if (it.getMSLevel() == 2)
      {
        ms2_cntr++;
      }
    }

    // Max MS Level is adjusted according to the input dataset
    current_max_ms_level = current_max_ms_level > max_ms_level ? max_ms_level : current_max_ms_level;
    // if an ensemble spectrum is analyzed, replace the input dataset with the ensemble one
    if (ensemble)
    {
      for (int i = 0; i < current_max_ms_level; ++i)
      {
        ensemble_map[i].sortByPosition();
        //spacing_difference_gap
        PeakPickerHiRes pickerHiRes;
        auto pickParam = pickerHiRes.getParameters();
        //pickParam.setValue("spacing_difference_gap", 1e-1);
        //pickParam.setValue("spacing_difference", 2.0);
        //pickParam.setValue("missing", 0);

        pickerHiRes.setParameters(pickParam);
        auto tmp_spec = ensemble_map[i];
        pickerHiRes.pick(tmp_spec, ensemble_map[i]);

        // PeakPickerCWT picker;
        // tmp_spec = ensemble_map[i];
        //picker.pick(tmp_spec, ensemble_map[i]);
      }

      map = ensemble_map;
    }
    // Run FLASHDeconv here

    int scan_number = 0;
    float prev_progress = .0;
    int num_last_deconvoluted_spectra = getIntOption_("preceding_MS1_count");
    if (!in_log_file.empty())
    {
      num_last_deconvoluted_spectra = 50; // if FLASHIda log file exists, keep up to 50 survey scans.
    }

    auto last_deconvoluted_spectra = std::unordered_map<UInt, std::vector<DeconvolutedSpectrum>>();
    //auto lastlast_deconvoluted_spectra = std::unordered_map<UInt, DeconvolutedSpectrum>();
    MSExperiment exp;

    auto fd = FLASHDeconvAlgorithm();
    Param fd_param = getParam_().copy("Algorithm:", true);
    DoubleList tols = fd_param.getValue("tol");
    //fd_param.setValue("tol", getParam_().getValue("tol"));
    if (ensemble)
    {
      fd_param.setValue("min_rt", .0);
      fd_param.setValue("max_rt", .0);
    }
    fd.setParameters(fd_param);
    fd.calculateAveragine(use_RNA_averagine);
    auto avg = fd.getAveragine();
    auto mass_tracer = MassFeatureTrace();
    Param mf_param = getParam_().copy("FeatureTracing:", true);
    DoubleList isotope_cosines = fd_param.getValue("min_isotope_cosine");
    //mf_param.setValue("mass_error_ppm", ms1tol);
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

    unordered_map<int, PeakGroup> precursor_peak_groups; // MS2 scan number, peak group

    OPENMS_LOG_INFO << "Running FLASHDeconv ... " << endl;

    for (auto it = map.begin(); it != map.end(); ++it)
    {
      scan_number = SpectrumLookup::extractScanNumber(it->getNativeID(),
                                                      map.getSourceFiles()[0].getNativeIDTypeAccession());
      if (it->empty())
      {
        continue;
      }

      float progress = (float) (it - map.begin()) / map.size();
      if (progress > prev_progress + .05)
      {
        printProgress_(progress);
        prev_progress = progress;
      }

      int ms_level = it->getMSLevel();
      if (ms_level > current_max_ms_level)
      {
        continue;
      }
      spec_cntr[ms_level - 1]++;
      auto deconv_begin = clock();
      auto deconv_t_start = chrono::high_resolution_clock::now();

      //auto deconvoluted_spectrum = DeconvolutedSpectrum(*it, scan_number);
      // for MS>1 spectrum, register precursor
      std::vector<DeconvolutedSpectrum> precursor_specs;

      if (ms_level > 1 && last_deconvoluted_spectra.find(ms_level - 1) != last_deconvoluted_spectra.end())
      {
        precursor_specs = (last_deconvoluted_spectra[ms_level - 1]);
      }

      auto deconvoluted_spectrum = fd.getDeconvolutedSpectrum(*it,
                                                              precursor_specs,
                                                              scan_number,
                                                              precursor_map_for_real_time_acquisition);

      if (it->getMSLevel() > 1 && !deconvoluted_spectrum.getPrecursorPeakGroup().empty())
      {
        precursor_peak_groups[scan_number] = deconvoluted_spectrum.getPrecursorPeakGroup();
      }

      /*
            for (auto &pg: deconvoluted_spectrum)
            {
              if(pg.getIsotopeCosine()<0.99) continue;
              if(std::get<0>(pg.getAbsChargeRange()) <=10 &&  std::get<1>(pg.getAbsChargeRange()) >=40 ){
                for (int c = std::get<0>(pg.getAbsChargeRange()) ; c <= std::get<1>(pg.getAbsChargeRange())  ; ++c)
                {
                  auto mzr = pg.getMzRange(c);
                  auto itr = it->MZBegin(std::get<0>(mzr));
                  std::cout<<"iso"<<c<<"=[";
                  while(itr->getMZ() < std::get<1>(mzr)){
                    cout<<itr->getMZ() * c << "," <<itr->getIntensity()<<";";
                    itr++;
                  }
                  std::cout<<"];\n";
                }
              }
            }
      */
      if (it->getMSLevel() == 2 && !in_train_file.empty() && !out_train_file.empty()
          && !deconvoluted_spectrum.getPrecursorPeakGroup().empty()
          )
      {
        QScore::writeAttTsv(deconvoluted_spectrum,
                            top_pic_map[scan_number],
                            avg,
                            out_train_stream,
                            write_detail_qscore_att);
      }


      if (!out_mzml_file.empty())
      {
        if (it->getMSLevel() == 1 || !deconvoluted_spectrum.getPrecursorPeakGroup().empty())
        {
          exp.addSpectrum(deconvoluted_spectrum.toSpectrum(mzml_charge));
        }
      }
      elapsed_deconv_cpu_secs[ms_level - 1] += double(clock() - deconv_begin) / CLOCKS_PER_SEC;
      elapsed_deconv_wall_secs[ms_level - 1] += chrono::duration<double>(
          chrono::high_resolution_clock::now() - deconv_t_start).count();

      if (ms_level < current_max_ms_level)
      {
        if (last_deconvoluted_spectra[ms_level].size() >= num_last_deconvoluted_spectra)
        {
          last_deconvoluted_spectra.erase(last_deconvoluted_spectra.begin());
        }
        last_deconvoluted_spectra[ms_level].push_back(deconvoluted_spectrum);
      }

      if (!ensemble)
      {
        mass_tracer.storeInformationFromDeconvolutedSpectrum(
            deconvoluted_spectrum);// add deconvoluted mass in mass_tracer
      }

      if (deconvoluted_spectrum.empty())
      {
        continue;
      }

      qspec_cntr[ms_level - 1]++;
      mass_cntr[ms_level - 1] += deconvoluted_spectrum.size();

      DoubleList iso_intensities;
      /*
            if (ms_level> 1 && !iso_mzs.empty())
            {
              if (iso_option == 1)
              {
                if (deconvoluted_spectrum.getActivation_method() == "ETD")
                {
                  iso_mzs = DoubleList{114.127725, 115.124760, 116.134433, 117.131468, 118.141141, 119.138176};
                }
                else
                {
                  iso_mzs = DoubleList{126.127725, 127.124760, 128.134433, 129.131468, 130.141141, 131.138176};
                }
              }

              int current_ch = 0;
              iso_intensities = DoubleList(iso_mzs.size(), 0.0);
              for (auto &peak: *it)
              {
                if (current_ch >= iso_mzs.size())
                {
                  break;
                }
                if (peak.getMZ() < iso_mzs[current_ch] - iso_mzs[current_ch] * tols[ms_level - 1] * 1e-6)
                {
                  continue;
                }
                if (peak.getMZ() > iso_mzs[current_ch] + iso_mzs[current_ch] * tols[ms_level - 1] * 1e-6)
                {
                  current_ch++;
                  continue;
                }
                iso_intensities[current_ch] += peak.getIntensity();
              }
            }*/

      if (out_spec_streams.size() > ms_level - 1)
      {
        deconvoluted_spectrum
            .writeDeconvolutedMasses(out_spec_streams[ms_level - 1], in_file, avg, write_detail);
      }
      if (out_topfd_streams.size() > ms_level - 1)
      {
        deconvoluted_spectrum.writeTopFD(out_topfd_streams[ms_level - 1], avg, topFD_SNR_threshold);
        #ifdef DEBUG_EXTRA_PARAMTER
        if(ms_level ==2 && !deconvoluted_spectrum.getPrecursorPeakGroup().empty()){
            f_out_topfd_file_log << scan_number <<","<<deconvoluted_spectrum.getPrecursorPeakGroup().getMonoMass()
            <<","<<deconvoluted_spectrum.getPrecursorPeakGroup().getRepAbsCharge()<<","
            <<deconvoluted_spectrum.getPrecursorPeakGroup().getIntensity()<<"\n";
        }
        #endif
      }
    }

    printProgress_(1); //
    std::cout << std::endl;

    // mass_tracer run
    if (!ensemble)
    {
      mass_tracer.findFeatures(in_file, !out_promex_file.empty(), !out_topfd_feature_file.empty(),
                               precursor_peak_groups,
                               feature_cntr, feature_index, out_stream, out_promex_stream, out_topfd_feature_streams,
                               fd.getAveragine());
    }
    if (!out_mzml_file.empty())
    {
      MzMLFile mzml_file;
      mzml_file.store(out_mzml_file, exp);
    }

    for (int j = 0; j < (int) current_max_ms_level; j++)
    {
      if (spec_cntr[j] == 0)
      {
        continue;
      }

      if (ensemble)
      {
        OPENMS_LOG_INFO << "So far, FLASHDeconv found " << mass_cntr[j] << " masses in the ensemble MS"
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

    auto t_end = chrono::high_resolution_clock::now();
    auto end = clock();

    elapsed_cpu_secs = double(end - begin) / CLOCKS_PER_SEC;
    elapsed_wall_secs = chrono::duration<double>(t_end - t_start).count();

    OPENMS_LOG_INFO << "-- done [took " << elapsed_cpu_secs << " s (CPU), " << elapsed_wall_secs
                    << " s (Wall)] --"
                    << endl;

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

    #ifdef DEBUG_EXTRA_PARAMTER
    f_out_topfd_file_log.close();
    fi_out.close();
    #endif
    out_stream.close();

    if (!out_promex_file.empty())
    {
      out_promex_stream.close();
    }
    if (!out_topfd_feature_file.empty())
    {
      for (auto &out_topfd_feature_stream: out_topfd_feature_streams)
      {
        out_topfd_feature_stream.close();
      }
    }


    if (!out_topfd_file.empty())
    {
      for (auto &out_topfd_stream: out_topfd_streams)
      {
        out_topfd_stream.close();
      }
    }
    if (!out_spec_file.empty())
    {
      int j = 0;
      for (auto &out_spec_stream: out_spec_streams)
      {
        out_spec_stream.close();
        if (spec_cntr[j] <= 0)
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

  static void printProgress_(float progress)
  {
    float bar_width = 70;
    std::cout << "[";
    int pos = (int) (bar_width * progress);
    for (int i = 0; i < bar_width; ++i)
    {
      if (i < pos)
      {
        std::cout << "=";
      }
      else if (i == pos)
      {
        std::cout << ">";
      }
      else
      {
        std::cout << " ";
      }
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
  }
};

// the actual main function needed to create an executable
int main(int argc, const char **argv)
{
  TOPPFLASHDeconv tool;
  return tool.main(argc, argv);
}
