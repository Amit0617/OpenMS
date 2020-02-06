// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
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
#include <OpenMS/FILTERING/DATAREDUCTION/MassTraceDetection.h>
#include <QDirIterator>
#include <QFileInfo>
#include <OpenMS/FORMAT/FileTypes.h>
#include <OpenMS/FORMAT/MzMLFile.h>

using namespace OpenMS;
using namespace std;

//-------------------------------------------------------------
// Doxygen docu
//-------------------------------------------------------------
/**
  @page TOPP_FLASHDeconv TOPP_FLASHDeconv
  (Need to be modified)

  @brief  @ref
  @code
  @endcode
  @verbinclude
  @htmlinclude
*/
// We do not want this class to show up in the docu:
// NEED to fill this part later


class TOPPFLASHDeconv :
    public TOPPBase
{
public:
  TOPPFLASHDeconv() :
      TOPPBase("FLASHDeconv", "Ultra-fast high-quality deconvolution enables online processing of top-down MS data",
               false)
  {
  }

protected:
  typedef FLASHDeconvHelperStructs::PeakGroup PeakGroup;
  typedef FLASHDeconvHelperStructs::Parameter Parameter;

  // this function will be used to register the tool parameters
  // it gets automatically called on tool execution
  void registerOptionsAndFlags_() override
  {
    registerInputFile_("in", "<input_file/input_dir>", "", "Input file or directory");
    //    setValidFormats_("in", ListUtils::create<String>("mzML"),);
    registerOutputFile_("out", "<output_file_prefix/output_dir>", "",
                        "Output file prefix or output dir (if prefix, [prefix].tsv will be generated. "
                        "if dir, [dir]/[inputfile].tsv is generated per [inputfile])");

    registerDoubleList_("tol", "ms1_tol ms2_tol ... (e.g., 10.0 5.0 to specify 10.0 and 5.0 ppm for MS1 and MS2, respectively", {10.0, 5.0}, "ppm tolerance for MS1, 2, ...", false);

    //registerDoubleOption_("tol", "<tolerance>", 10.0, "ppm tolerance", false, false);
    registerIntOption_("minC", "<min_charge>", 1, "minimum charge state", false, false);
    registerIntOption_("maxC", "<max_charge>", 100, "maximum charge state", false, false);
    registerDoubleOption_("minM", "<min_mass>", 50.0, "minimum mass (Da)", false, false);
    registerDoubleOption_("maxM", "<max_mass>", 100000.0, "maximum mass (Da)", false, false);

    registerDoubleList_("minIC", "ms1_isotope_cos ms2_isotpe_cos ... (e.g., 0.8 0.6 to specify 0.8 and 0.6 for MS1 and MS2, respectively)",
        {.75, .8},
        "cosine threshold between avg. and observed isotope pattern for MS1, 2, ...", false, true);

    registerDoubleOption_("minCC",
                          "<charge_cosine>",
                          .5,
                          "cosine threshold between per-charge-intensity and fitted gaussian distribution (applies only to MS1)",
                          false,
                          true);


    registerIntList_("minCP",
                       "ms1_min_continuous_charge_peaks ms2_min_continuous_charge_peaks ... (e.g., 3 2 to specify 3 and 2 for MS1 and MS2, respectivly",
                     {3, 2},
                       "minimum number of peaks of continuous charges",
                       false,
                       true);

    registerIntOption_("maxMC", "<max_mass_count>", -1, "maximum mass count per spec", false, true);
    //
    registerDoubleOption_("minIT", "<min_intensity>", 0.0, "intensity threshold (default 0.0)", false, true);
    registerDoubleOption_("RTwindow", "<seconds>", 0.0, "RT window (if 0, 1% total gradient time)", false, true);
    registerDoubleOption_("minRTspan", "<seconds>", 10.0, "Min feature RT span", false, true);
    registerIntOption_("writeSpecDeconv",
                       "<1:true 0:false>",
                       0,
                       "to write per spectrum deconvoluted masses or not. If set, [prefix]PerSpecMasses.tsv is generated",
                       false,
                       true);

    registerIntOption_("maxMSL", "", 2, "maximum MS-level (inclusive) for deconvolution", false, true);

    // parameters for MSn

//    registerDoubleOption_("minCCS2",
//                          "<MSn cosine threshold 0 - 1>",
//                          0,
//                          "cosine threshold between per-charge-intensity and fitted gaussian distribution (spectrum level) for MSn (n>1)",
//                          false,
//                          true);

    //registerIntOption_("jitter", "<1:true 0:false>", 0, "jitter universal pattern to generate decoy features (output file will end with *Decoy.tsv)", false, true);
  }

  Parameter setParameter()
  {
    Parameter param;
    param.minCharge = getIntOption_("minC");
    param.currentChargeRange = param.chargeRange = getIntOption_("maxC") - param.minCharge + 1;
    param.currentMaxMass = param.maxMass = getDoubleOption_("maxM");
    param.minMass = getDoubleOption_("minM");
    param.tolerance = getDoubleList_("tol");

    for(int j=0;j<param.tolerance.size();j++){
      param.tolerance[j] *= 1e-6;
      param.binWidth.push_back(.5 / param.tolerance[j]);
    }

    param.intensityThreshold = getDoubleOption_("minIT");
    param.minContinuousChargePeakCount = getIntList_("minCP");
    param.minIsotopeCosine = getDoubleList_("minIC");
    param.minChargeCosine = getDoubleOption_("minCC");

    param.currentMaxMassCount = param.maxMassCount = getIntOption_("maxMC");
    //param.chargeDistributionScoreThreshold = getDoubleOption_("minCDScore");
    param.RTwindow = getDoubleOption_("RTwindow");
    param.minRTSpan = getDoubleOption_("minRTspan");
    param.threads = getIntOption_("threads");
    param.writeSpecTsv = getIntOption_("writeSpecDeconv");
    //param.jitter = getIntOption_("jitter");
    param.maxMSLevel = getIntOption_("maxMSL");

    return param;
  }

  FLASHDeconvHelperStructs::PrecalcularedAveragine calculateAveragines(Parameter &param)
  {
    auto generator = new CoarseIsotopePatternGenerator();
    auto maxIso = generator->estimateFromPeptideWeight(param.maxMass);
    maxIso.trimRight(0.01 * maxIso.getMostAbundant().getIntensity());
    param.maxIsotopeCount = (int) maxIso.size() - 1;
    generator->setMaxIsotope((Size) param.maxIsotopeCount);
    return FLASHDeconvHelperStructs::PrecalcularedAveragine(50, param.maxMass, 20, generator);
  }
  // the main_ function is called after all parameters are read
  ExitCodes main_(int, const char **) override
  {
    //-------------------------------------------------------------
    // parsing parameters
    //-------------------------------------------------------------
    String infilePath = getStringOption_("in");
    String outfilePath = getStringOption_("out");

    auto param = setParameter();
    auto avgine = calculateAveragines(param);

    int specCntr = 0, qspecCntr = 0, massCntr = 0, featureCntr = 0;
    int total_specCntr = 0, total_qspecCntr = 0, total_massCntr = 0, total_featureCntr = 0;
    double total_elapsed_cpu_secs = 0, total_elapsed_wall_secs = 0;
    fstream fs, fsf, fsm;

    //-------------------------------------------------------------
    // reading input file directory -> put that in array
    //-------------------------------------------------------------
    vector<QString> infileArray;
    QString path = QString::fromUtf8(infilePath.data(), (int) infilePath.size());
    QFileInfo check_file(path);
    if (check_file.isDir())
    {
      QDirIterator it(path, QStringList() << "*.mzml", QDir::Files, QDirIterator::Subdirectories);
      while (it.hasNext())
      {
        infileArray.push_back(it.next());
      }
    }
    else
    {
      infileArray.push_back(path);
    }
    sort(infileArray.begin(), infileArray.end());

    bool isOutPathDir = (QFileInfo(QString::fromUtf8(outfilePath.data(), (int) outfilePath.size())).isDir());

    OPENMS_LOG_INFO << "Initializing ... " << endl;

    if (!isOutPathDir)
    {
      if (param.writeSpecTsv > 0)
      {
        fs.open(outfilePath + "PerSpecMasses.tsv", fstream::out);
        fsm.open(outfilePath + "PerSpecMasses.m", fstream::out);
      }
      // if (param.RTwindow > 0)
      // {


      fsf.open(outfilePath + ".tsv", fstream::out);

      //  }

      writeHeader(fs, fsf, true);
      //  fsm.open(outfilePath + ".m", fstream::out);
      //  fsm << "m=[";

      // fsp.open(outfilePath + "peak.m", fstream::out);
    }

    //-------------------------------------------------------------
    // reading input
    //-------------------------------------------------------------
    for (auto &infile : infileArray)
    {
      if (isOutPathDir)
      {
        specCntr = qspecCntr = massCntr = featureCntr = 0;
      }
      MSExperiment map;
      MzMLFile mzml;

      double elapsed_cpu_secs = 0, elapsed_wall_secs = 0;
      double elapsed_deconv_cpu_secs = 0, elapsed_deconv_wall_secs = 0;

      auto begin = clock();
      auto t_start = chrono::high_resolution_clock::now();

      OPENMS_LOG_INFO << "Processing : " << infile.toStdString() << endl;

      mzml.setLogType(log_type_);
      mzml.load(infile, map);

      param.fileName = QFileInfo(infile).fileName().toStdString();

      double rtDuration = map[map.size() - 1].getRT() - map[0].getRT();
      auto msCntr = new int[1+param.maxMSLevel];
      fill_n(msCntr, 1+param.maxMSLevel,0);

      for (auto it = map.begin(); it != map.end(); ++it)
      {
        if (it->getMSLevel() > param.maxMSLevel)
        {
          continue;
        }
        msCntr[it->getMSLevel()]++;
      }

      for(int j=1;j<=param.maxMSLevel;j++){
        double rtDelta = rtDuration / msCntr[j];

        auto rw = param.RTwindow;
        if (rw <= 0)
        {
          rw = std::max(10.0, rtDuration * .01);
        }
        //OPENMS_LOG_INFO << rtDuration << " " << rtDelta << " " << rw <<  endl;

        auto count = max(param.minNumOverLappedScans, (UInt) (.5 + rw / rtDelta));
        param.numOverlappedScans.push_back(count);
        OPENMS_LOG_INFO <<  "# Overlapped MS" << j<< " scans:" << count << " (in RT " << rw
                        << " sec)" << endl;

      }
      delete[] msCntr;

      if (isOutPathDir)
      {
        std::string outfileName(param.fileName);
        std::size_t found = outfileName.find_last_of(".");
        outfileName = outfileName.substr(0, found);

        if (param.writeSpecTsv > 0)
        {
          fs.open(outfilePath + outfileName + "PerSpecMasses.tsv", fstream::out);
          fsm.open(outfilePath + outfileName + "PerSpecMasses.m", fstream::out);
        }

        fsf.open(outfilePath + outfileName + ".tsv", fstream::out);


        // fsm.open(outfilePath + outfileName + "Annotated.m", fstream::out); //
        writeHeader(fs, fsf, true);

        //outfileName.erase(std::remove(outfileName.begin(), outfileName.end(), '_'), outfileName.end());
        //outfileName.erase(std::remove(outfileName.begin(), outfileName.end(), '-'), outfileName.end());
        //   fsm.open(outfilePath + "m" + outfileName + ".m", fstream::out);
        //   fsm << "m=[";
        //   fsp.open(outfilePath + "m" + outfileName + "peak.m", fstream::out);
      }

      OPENMS_LOG_INFO << "Running FLASHDeconv ... " << endl;
      auto deconv_begin = clock();
      auto deconv_t_start = chrono::high_resolution_clock::now();
      //continue;
      auto fa = FLASHDeconvAlgorithm(map, param);
      auto peakGroups = fa.Deconvolution(specCntr, qspecCntr, massCntr, avgine);

      auto deconv_t_end = chrono::high_resolution_clock::now();
      auto deconv_end = clock();

      //writeAnnotatedSpectra(peakGroups,map,fsm);//

      if (!peakGroups.empty() && specCntr > 0 && map.size() > 1)
      {
        Param common_param = getParam_().copy("algorithm:common:", true);
        writeDebug_("Common parameters passed to sub-algorithms (mtd and ffm)", common_param, 3);

        Param mtd_param = getParam_().copy("algorithm:mtd:", true);
        writeDebug_("Parameters passed to MassTraceDetection", mtd_param, 3);

        mtd_param.insert("", common_param);
        mtd_param.remove("chrom_fwhm");

        MassFeatureTrace::findFeatures(peakGroups, qspecCntr, featureCntr, fsf, avgine, mtd_param, param); //
      }

      //cout<< "after running" << endl;

      if (param.writeSpecTsv)
      {
        OPENMS_LOG_INFO << endl << "writing per spec deconvolution results ...";
        OPENMS_LOG_INFO.flush();
        fsm << "monomasses=[";

        auto rtMassMap = std::map<double, vector<PeakGroup>>();

        for (auto &pg : peakGroups)
        {
          if (pg.spec->getMSLevel() == 1)
          {
            continue;
          }
          auto rt = pg.spec->getRT();

          vector<PeakGroup> pgs;
          if (rtMassMap.find(rt) != rtMassMap.end())
          {
            pgs = rtMassMap[rt];
          }
          pgs.push_back(pg);
          rtMassMap[rt] = pgs;

        }
        auto monoMassSet = std::set<int>();
        for (auto iter = rtMassMap.begin(); iter != rtMassMap.end(); ++iter)
        {
          auto pgs = iter->second;
          monoMassSet.clear();

          auto ints = vector<double>();
          for (auto &pg : pgs)
          {
            ints.push_back(pg.intensity);
          }
          sort(ints.begin(),ints.end());
          auto intmap = std::map<double, int>();
          for (int j = 0; j < ints.size(); ++j)
          {
            intmap[ints[j]] = j;
          }

          for (auto &pg : pgs)
          {
            auto intMass = (int) round(pg.monoisotopicMass);
            if (monoMassSet.find(intMass) != monoMassSet.end())
            {
              continue;
            }

            monoMassSet.insert(intMass);

            fsm << pg.monoisotopicMass << " " << pg.isotopeCosineScore << " " << pg.intensity << " " << pg.chargeCosineScore << ";";
            if (pg.spec->getMSLevel() > 1 && pg.monoisotopicMass > 2e4)
            {
              writePeakGroup(pg, param, fs);
            }
            //writePeakGroupMfile(pg, param, fsm);
          }
        }


        fsm << "];\n";

        OPENMS_LOG_INFO << "done" << endl;

      }

      // cout<<4.5<<endl;
      if (isOutPathDir)
      {
        OPENMS_LOG_INFO << "In this run, FLASHDeconv found " << massCntr << " masses in " << qspecCntr
                        << " MS1 spectra out of "
                        << specCntr << endl;
        if (featureCntr > 0)
        {
          OPENMS_LOG_INFO << "Mass tracer found " << featureCntr << " features" << endl;
        }

        //   fsm << "];";
        //   fsm.close();
        //   fsp.close();
        if (param.writeSpecTsv > 0)
        {
          fs.close();
          fsm.close();
        }

        fsf.close();

        //fsm.close();
        total_specCntr += specCntr;
        total_qspecCntr += qspecCntr;
        total_massCntr += massCntr;
        total_featureCntr += featureCntr;
      }
      else
      {
        OPENMS_LOG_INFO << "So far, FLASHDeconv found " << massCntr << " masses in " << qspecCntr
                        << " MS1 spectra out of "
                        << specCntr << endl;
        if (featureCntr > 0)
        {
          OPENMS_LOG_INFO << "Mass tracer found " << featureCntr << " features" << endl;
        }

        total_specCntr = specCntr;
        total_qspecCntr = qspecCntr;
        total_massCntr = massCntr;
        total_featureCntr = featureCntr;

      }

      //cout<<5<<endl;
      auto t_end = chrono::high_resolution_clock::now();
      auto end = clock();

      elapsed_deconv_cpu_secs = double(deconv_end - deconv_begin) / CLOCKS_PER_SEC;
      elapsed_deconv_wall_secs = chrono::duration<double>(deconv_t_end - deconv_t_start).count();

      elapsed_cpu_secs = double(end - begin) / CLOCKS_PER_SEC;
      elapsed_wall_secs = chrono::duration<double>(t_end - t_start).count();

      OPENMS_LOG_INFO << "-- done [took " << elapsed_cpu_secs << " s (CPU), " << elapsed_wall_secs
                      << " s (Wall)] --"
                      << endl;
      OPENMS_LOG_INFO << "-- deconv per spectrum (except spec loading, feature finding) [took "
                      << 1000.0 * elapsed_deconv_cpu_secs / specCntr
                      << " ms (CPU), " << 1000.0 * elapsed_deconv_wall_secs / specCntr << " ms (Wall)] --" << endl;

      total_elapsed_cpu_secs += elapsed_cpu_secs;
      total_elapsed_wall_secs += elapsed_wall_secs;

      //TODO remove
      if(false)
      {
        auto monoMassSet = set<int>();
        mzml.load("/Users/kyowonjeong/Documents/A4B/Results/MS2/xtract/myo_707_ETDReagentTarget_1e+06_.mzML", map);
        fsm.open(outfilePath + "/xtract5.m", fstream::out);

        fsm << "\nxmmy=[";
        for (auto it = map.begin(); it != map.end(); ++it)
        {
          for (auto p : *it)
          {
            auto intMass = round(p.getMZ());
            if (monoMassSet.find(intMass) != monoMassSet.end())
            {
              continue;
            }
            monoMassSet.insert(intMass);

            fsm << p.getMZ() << " " << p.getIntensity() << ";";
          }
          monoMassSet.clear();
        }
        fsm << "];\n";
        fsm.close();
      }
    }


    //-------------------------------------------------------------
    // calculations
    //-------------------------------------------------------------
    //-------------------------------------------------------------
    // writing output
    //-------------------------------------------------------------
    OPENMS_LOG_INFO << "Total elapsed time\n-- done [took " << total_elapsed_cpu_secs << " s (CPU), "
                    << total_elapsed_wall_secs
                    << " s (Wall)] --"
                    << endl;

    if (massCntr < total_massCntr)
    {
      OPENMS_LOG_INFO << "In total, FLASHDeconv found " << total_massCntr << " masses in " << total_qspecCntr
                      << " MS1 spectra out of "
                      << total_specCntr << endl;
      if (featureCntr > 0)
      {
        OPENMS_LOG_INFO << "Mass tracer found " << total_featureCntr << " features" << endl;
      }
    }

    if (!isOutPathDir)
    {
      // fsm << "];";
      //  fsm.close();
      //  fsp.close();
      if (param.writeSpecTsv > 0)
      {
        fs.close();
        fsm.close();
      }
      fsf.close();
    }

    return EXECUTION_OK;
  }

  static void writeAnnotatedSpectra(vector <PeakGroup> &pgs,
                                    MSExperiment &map,
                                    fstream &fs)//, fstream &fsm, fstream &fsp)
  {

    boost::unordered_map<double, vector < PeakGroup>>
    pgmap;

    for (auto &pg : pgs)
    {
      pgmap[pg.spec->getRT()].push_back(pg);
    }

    int index = 1;
    for (auto it = map.begin(); it != map.end(); ++it)
    {
      double rt = it->getRT();
      /*if (rt < 3250)
      {
        continue;
      }
      if (rt > 3350)
      {
        break;
      }*/

      if (pgmap.find(rt) == pgmap.end())
      {
        continue;
      }
      auto t = pgmap[rt];
      vector<double> masses;
      int mi = 1;
      for (auto &p : t)
      {
        fs << "apeaks" << index << "{" << mi++ << "}=[";
        for (auto &lp:p.peaks)
        {
          //auto &op = lp.orgPeak;
          fs << lp.mz << "," << lp.intensity << ";";
        }

        fs << "];\n";
      }
      fs << "aspec{" << index << "}=[";
      for (auto &p : t)
      {
        fs << p.monoisotopicMass << "," << p.intensity << ";";
      }
      fs << "];\n";

      fs << "spec{" << index << "}=[";
      for (auto &p : *it)
      {
        fs << p.getMZ() << "," << p.getIntensity() << ";";
      }
      fs << "];\n";


      index++;
      //if (my_hash_map.find(non-existent key) == my_hash_map.end())

    }


  }

  static void writePeakGroup(PeakGroup &pg, Parameter &param, fstream &fs)//, fstream &fsm, fstream &fsp)
  {
    //return;//
    if (pg.peaks.empty())
    {
      return;
    }
    double &m = pg.monoisotopicMass;
    double &am = pg.avgMass;
    double &intensity = pg.intensity;
    //int nm = getNominalMass(m);
    sort(pg.peaks.begin(), pg.peaks.end());
    int minCharge = param.chargeRange + param.minCharge;
    int maxCharge = -1;
    for (auto &p : pg.peaks)
    {
      minCharge = minCharge < p.charge ? minCharge : p.charge;
      maxCharge = maxCharge > p.charge ? maxCharge : p.charge;
    }
    //<< std::to_string(am) << "\t" << std::to_string(m) << "\t" << intensity << "\t"
    //       << (maxCharge - minCharge + 1) << "\t" << minCharge << "\t" << maxCharge << "\t"
    //       << std::to_string(pg.spec->getRT())
    //cout<<1<<endl;
    fs << pg.massIndex << "\t" << pg.specIndex << "\t" << param.fileName << "\t" << pg.spec->getNativeID() << "\t"
       << pg.spec->getMSLevel() << "\t"
       << pg.massCntr << "\t"
       << std::to_string(am) << "\t" << std::to_string(m) << "\t" << intensity << "\t"
       << (maxCharge - minCharge + 1) << "\t" << minCharge << "\t" << maxCharge << "\t"
       << std::to_string(pg.spec->getRT())
       << "\t" << pg.peaks.size() << "\t";

    fs << fixed << setprecision(2);
    for (auto &p : pg.peaks)
    {
      fs << p.mz << ";";
    }
    fs << "\t";
    for (auto &p : pg.peaks)
    {
      fs << p.charge << ";";
    }
    fs << "\t";
    for (auto &p : pg.peaks)
    {
      fs << p.getUnchargedMass() << ";";
    }
    fs << "\t";
    for (auto &p : pg.peaks)
    {
      fs << p.isotopeIndex << ";";
    }
    fs << "\t";

    for (auto &p : pg.peaks)
    {
      auto tm = pg.monoisotopicMass + p.isotopeIndex * Constants::ISOTOPE_MASSDIFF_55K_U;
      auto diff = tm / p.charge + Constants::PROTON_MASS_U - p.mz;

      fs << 100 * diff << ";";
    }
    fs << "\t";

    fs << fixed << setprecision(1);
    for (auto &p : pg.peaks)
    {
      fs << p.intensity << ";";
    }
    fs << fixed << setprecision(3);
    fs << "\t" << pg.isotopeCosineScore
       << "\t" << pg.chargeCosineScore
       << "\n";

    /*
    //cout<<1<<endl;

            fsp << "pg" << (int) (pg.monoisotopicMass * 10) << "rt" << (int) (pg.spec->getRT())
                << "=[";

            for (auto &p : pg.peaks) {
                fsp << p.charge << "," << p.isotopeIndex << "," << p.orgPeak->getIntensity() << ";";
            }

            fsp << "];\n";
            //cout<<3<<endl;
 */
    //fsm << m << "," << nm << "," << intensity << "," << pg.spec->getRT() << "\n";
    //cout<<4<<endl;


  }


  static void writePeakGroupMfile(PeakGroup &pg, Parameter &param, fstream &fs)//, fstream &fsm, fstream &fsp)
  {
    if (pg.peaks.empty())//
    {
      return;
    }
    //    double &m = pg.monoisotopicMass;
    //    double &am = pg.avgMass;
    //double &intensity = pg.intensity;
    //int nm = getNominalMass(m);
    sort(pg.peaks.begin(), pg.peaks.end());
    int minCharge = param.chargeRange + param.minCharge;
    int maxCharge = -1;

    int minIsotope = param.maxIsotopeCount;
    int maxIsotope = -1;

    for (auto &p : pg.peaks)
    {
      minCharge = minCharge < p.charge ? minCharge : p.charge;
      maxCharge = maxCharge > p.charge ? maxCharge : p.charge;


      minIsotope = minIsotope < p.isotopeIndex ? minIsotope : p.isotopeIndex;
      maxIsotope = maxIsotope > p.isotopeIndex ? maxIsotope : p.isotopeIndex;
    }

    auto iis = new float[maxIsotope + 1];
    auto cis = new float[maxCharge + 1];
    fill_n(iis, maxIsotope + 1, 0);
    fill_n(cis, maxCharge + 1, 0);

    fs << (pg.spec->getMSLevel() == 1 ? "sm" : "tm") << pg.specIndex << "_" << pg.massIndex << "="
       << pg.monoisotopicMass << ";\n";

    fs << (pg.spec->getMSLevel() == 1 ? "sa" : "ta") << pg.specIndex << "_" << pg.massIndex << "=[";
    for (auto &p : pg.peaks)
    {
      fs << p.mz << "," << p.intensity << ";";
      iis[p.isotopeIndex] += p.intensity;
      cis[p.charge] += p.intensity;
    }
    fs << "];\n";

    fs << (pg.spec->getMSLevel() == 1 ? "sc" : "tc") << pg.specIndex << "_" << pg.massIndex << "=[";
    for (auto i = 0; i <= maxCharge; i++)
    {
      if (cis[i] <= 0)
      {
        continue;
      }
      fs << i << "," << cis[i] << ";";
    }
    fs << "];\n";

    float **dist = new float *[maxCharge - minCharge + 1];
    for (int j = 0; j <= maxCharge - minCharge; ++j)
    {
      dist[j] = new float[maxIsotope - minIsotope + 1];
      fill_n(dist[j], maxIsotope - minIsotope + 1, 0);
    }

    for (auto &p : pg.peaks)
    {
      dist[p.charge - minCharge][p.isotopeIndex - minIsotope] += p.intensity;
    }

    fs << (pg.spec->getMSLevel() == 1 ? "sd" : "td") << pg.specIndex << "_" << pg.massIndex << "=[";

    for (int j = 0; j <= maxCharge - minCharge; ++j)
    {
      for (int i = 0; i <= maxIsotope - minIsotope; ++i)
      {
        fs << dist[j][i] << ",";
      }
      fs << ";";
    }
    fs << "];";

    fs << (pg.spec->getMSLevel() == 1 ? "si" : "ti") << pg.specIndex << "_" << pg.massIndex << "=[";
    for (auto i = 0; i <= maxIsotope; i++)
    {
      if (iis[i] <= 0)
      {
        continue;
      }
      fs << i << "," << iis[i] << ";";
    }

    fs << "];\n";

    delete[] iis;
    delete[] cis;
    for (int j = 0; j <= maxCharge - minCharge; ++j)
    {
      delete[] dist[j];
    }
    delete[] dist;
  }

  static void writeHeader(fstream &fs, fstream &fsf, bool featureOut = false)
  {
    fs
        << "MassIndex\tSpecIndex\tFileName\tSpecID\tMSLevel\tMassCountInSpec\tAvgMass\tMonoisotopicMass\t"
           "AggregatedIntensity\tPeakChargeRange\tPeakMinCharge\tPeakMaxCharge\t"
           "RetentionTime\tPeakCount\tPeakMZs\tPeakCharges\tPeakMasses\tPeakIsotopeIndices\tPeakMzErrors\t"
           "PeakIntensities\tIsotopeCosineScore\tChargeIntensityCosineScore\n";
    if (!featureOut)
    {
      return;
    }
    fsf << "ID\tFileName\tMonoisotopicMass\tAverageMass\tMassCount\tStartRetentionTime"
           "\tEndRetentionTime\tRetentionTimeDuration\tApexRetentionTime"
           "\tSumIntensity\tMaxIntensity\tMinCharge\tMaxCharge\tChargeCount\tIsotopeCosineScore\tChargeIntensityCosineScore"
           "\tPeakGroupMasses\tPeakGroupRTs\n";

    return;
  }


};


// the actual main function needed to create an executable
int main(int argc, const char **argv)
{
  TOPPFLASHDeconv tool;
  return tool.main(argc, argv);
}
