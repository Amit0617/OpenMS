//--------------------------------------------------------------------------
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
// $Maintainer: Kyowon Jeong$
// $Authors: Kyowon Jeong$
// --------------------------------------------------------------------------

#include "OpenMS/ANALYSIS/TOPDOWN/QScore.h"
#include <OpenMS/ANALYSIS/TOPDOWN/PeakGroup.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvHelperStructs.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvAlgorithm.h>
#include <iomanip>

namespace OpenMS
{

  double QScore::getQScore(const PeakGroup *pg, const int abs_charge)
  {
      if (pg == nullptr) { // all zero
          return .0;
      }
      //const std::vector<double> weights_vh({1.3522, -1.0877, -16.4956, -2.036, -0.9439, 18.251});
      const std::vector<double> weights_h({-1.4105, -1.514, -2.2335, -1.4643, 0.1329, 0.262, 4.3052});
      //const std::vector<double> weights_l({-3.203, -2.6899, 11.1909, -3.1146, -1.9595, -2.3368});

      //
      //ChargeCos       -4.8145
      //ChargeSNR       -2.0881
      //Cos            -21.4721
      //SNR             -0.6114
      //ChargeScore     -0.8793
      //AvgPPMerror      0.0418
      //Intercept       28.1305

      //ChargeCos      -1.4105
      //ChargeSNR       -1.514
      //Cos            -2.2335
      //SNR            -1.4643
      //ChargeScore     0.1329
      //AvgPPMerror      0.262
      //Intercept       4.3052

      const std::vector<double> &weights = weights_h;// (abs_charge > 6 ?
      //(pg->getMonoMass() > 30000.0 ? weights_vh : weights_h) :
      //weights_l);
      double score = weights[weights.size() - 1];
      auto fv = toFeatureVector_(pg, abs_charge);

      for (int i = 0; i < weights.size() - 1; i++) {
          score += fv[i] * weights[i];
      }
    return 1.0 / (1.0 + exp(score));
  }

  std::vector<double> QScore::toFeatureVector_(const PeakGroup *pg, const int abs_charge)
  {
    std::vector<double> fvector(6);

    double a = pg->getChargeIsotopeCosine(abs_charge);
    double d = 1;
    int index = 0;
    fvector[index++] = (log2(a + d));
    a = pg->getChargeSNR(abs_charge);
    fvector[index++] = (log2(d + a / (1 + a)));
    a = pg->getIsotopeCosine();
    fvector[index++] = (log2(a + d));
    a = pg->getSNR();
    fvector[index++] = (log2(d + a / (1 + a)));
    a = pg->getChargeScore();
    fvector[index++] = (log2(a + d));
    a = pg->getAvgPPMError();
    fvector[index++] = a;//(log2(a + d));
    return fvector;
  }

  void QScore::writeAttHeader(std::fstream &f, bool write_detail)
  {
    f
        << "ACC,FirstResidue,LastResidue,ProID,RT,ScanNumber,PrecursorScanNumber,PrecursorMonoMass,PrecursorAvgMass,Color,PrecursorMz,PrecursorIntensity,"
           "MassIntensity,FeatureIntensity,PrecursorCharge,PTM,PTMMass1,PTMMass2,PTMMass3,ChargeCos,ChargeSNR,Cos,SNR,ChargeScore,AvgPPMerror,Qscore,Evalue,";
    if (write_detail)
    {
      f << "PeakMZs,PeakIntensities,PeakMasses,PeakCharges,PeakIsotopeIndices,";
    }
    f << "Class\n";
  }

  void QScore::writeAttTsv(const int scan_number,
                           const String &acc,
                           const int proID,
                           const double rt,
                           const int pscan,
                           const double pmass,
                           const double pmz,
                           const DataValue color,
                           const double fintensity,
                           PeakGroup &pg,
                           const int fr,
                           const int lr,
                           const int charge,
                           const double precursor_intensity,
                           const std::vector<double> ptm_mass,
                           //const std::vector<int> ptm_start,
                           //const std::vector<int> ptm_end,
                           const bool is_identified,
                           const double e_value,
                           const FLASHDeconvHelperStructs::PrecalculatedAveragine &avg,
                           std::fstream &f,
                           bool write_detail)
  {
    auto avgpmass = avg.getAverageMassDelta(pmass) + pmass;
    if (pg.empty())
    {
      return;
    }
    else
    {
      auto fv = toFeatureVector_(&pg, charge);
      //if (pg.getChargeIsotopeCosine(charge) <= 0)
      //  return;
      double monomass = pmass <= .0? pg.getMonoMass() : pmass;
      double mass = pmass <= .0 ? avg.getAverageMassDelta(pg.getMonoMass()) + pg.getMonoMass() : avgpmass;
        f << acc << "," << fr << "," << lr << "," << proID << "," << rt << "," << scan_number << "," << pscan << ","
          << monomass << "," << mass << "," << color << "," << pmz << ","
          << precursor_intensity << ","
          << pg.getIntensity() << "," << fintensity << ","
          << charge << ","
          << (is_identified ? std::to_string(ptm_mass.size()) : "nan") << ",";
        for(int k=0;k<3;k++){
            if(k < ptm_mass.size()){
                f<<ptm_mass[k]<<",";
            }else{
                f<<"nan,";
            }
        }

      for (auto &item : fv)
      {
        f << item << ",";
      }

      //(pg.getIsotopeCosine() <=0 ? pg.getQScore(): getQScore(&pg, charge))
      f << pg.getQScore() << "," << e_value << ",";
      if (write_detail)
      {
        f << std::fixed << std::setprecision(2);
        for (auto &p : pg)
        {
          f << p.mz << " ";
        }
        f << ";,";

        f << std::fixed << std::setprecision(1);
        for (auto &p : pg)
        {
          f << p.intensity << " ";
        }
        f << ";,";
        f << std::setprecision(-1);


        for (auto &p : pg)
        {
          f << p.getUnchargedMass() << " ";
        }
        f << ";,";

        for (auto &p : pg)
        {
          f << (p.is_positive ? p.abs_charge : -p.abs_charge) << " ";
        }
        f << ";,";

        for (auto &p : pg)
        {
          f << p.isotopeIndex << " ";
        }
        f << ";,";
        f << std::fixed << std::setprecision(-1);
      }


      f << (is_identified ? "T" : "F") << "\n";
    }
  }
}
