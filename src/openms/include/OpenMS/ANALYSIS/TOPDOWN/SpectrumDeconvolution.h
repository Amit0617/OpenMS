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

#pragma once

#include <OpenMS/APPLICATIONS/TOPPBase.h>
#include <OpenMS/CONCEPT/Types.h>
#include <OpenMS/CHEMISTRY/ISOTOPEDISTRIBUTION/IsotopeDistribution.h>
#include <OpenMS/CHEMISTRY/ISOTOPEDISTRIBUTION/CoarseIsotopePatternGenerator.h>
#include <OpenMS/MATH/STATISTICS/StatisticFunctions.h>
#include "boost/dynamic_bitset.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvHelperStructs.h>
#include <OpenMS/ANALYSIS/TOPDOWN/PeakGroupScoring.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvAlgorithm.h>
#include <OpenMS/ANALYSIS/TOPDOWN/PeakGroup.h>
#include <OpenMS/DATASTRUCTURES//Matrix.h>

namespace OpenMS
{
  //This class performs spectrum level deconvolution.
  class OPENMS_DLLAPI SpectrumDeconvolution
  {
  public:
    //FLASHDeconv parameter
    typedef FLASHDeconvHelperStructs::Parameter Parameter;
    //Precalculated averagine
    typedef FLASHDeconvHelperStructs::PrecalculatedAveragine PrecalculatedAveragine;
    //Peaks with log mz transformation
    typedef FLASHDeconvHelperStructs::LogMzPeak LogMzPeak;

    /// default constructor
    SpectrumDeconvolution(MSSpectrum &spec, Parameter &param);

    /// default destructor
    ~SpectrumDeconvolution();

    bool empty();

    //Main function
    std::vector<PeakGroup> &getPeakGroupsFromSpectrum(std::vector<std::vector<Size>> &prevMassBinVector,
                                                      std::vector<double> &prevMinBinLogMassVector,
                                                      FLASHDeconvHelperStructs::PrecalculatedAveragine &avg,
                                                      unsigned int msLevel);

    //Stores log mz peaks
    std::vector<LogMzPeak> logMzPeaks;

  protected:
    MSSpectrum &spec;
    Parameter &param;

    //Bins for mass and mz. Bin size is deteremine by Parameter.tolerance
    //massBinsForThisSpectrum stores the selected bins only for this spectrum
    boost::dynamic_bitset<> massBinsForThisSpectrum;
    //massBins stores the selected bins for this spectrum + overlapped spectrum (previous a few spectra).
    boost::dynamic_bitset<> massBins;
    //mzBins stores the binned log mz peaks
    boost::dynamic_bitset<> mzBins;
    //peakGroups stores the decovnoluted mass peak groups
    std::vector<PeakGroup> peakGroups;

    //This stores the "universal pattern"
    std::vector<double> filter;
    //This stores the patterns for harmonic reduction
    Matrix<double> harmonicFilter;

    //This stores the "universal pattern" in binned dimenstion
    std::vector<int> binOffsets;
    //This stores the patterns for harmonic reduction in binned dimenstion
    Matrix<int> hBinOffsets;

    //static fucntion that converts bin to value
    static double getBinValue(Size bin, double minV, double binWidth);

    //static function that converts value to bin
    static Size getBinNumber(double v, double minV, double binWidth);

    static void pringMasses(boost::dynamic_bitset<> &massBins, double &minMass, double binWidth);

    //generate log mz peaks from the input spectrum
    void updateLogMzPeaks(double chargeMass);

    //generate mz bins from log mz peaks
    void updateMzBins(double &mzBinMinValue, Size &binNumber, double binWidth,
                      float *mzBinIntensities);

    //this function takes the previous deconvolution results (from ovelapped spectra) for sensitive deconvolution of the current spectrum
    void unionPrevMassBins(double &massBinMinValue,
                           std::vector<std::vector<Size>> &prevMassBinVector,
                           std::vector<double> &prevMassBinMinValue,
                           UInt msLevel);

    //Update mass bins from mz bins and universal pattern. It select candidate mass bins using the pattern, eliminate possible harmonic masses
    Matrix<Byte> updateMassBins(double &massBinMinValue,
                                double &mzBinMinValue,
                                float *massIntensities,
                                float *mzIntensities,
                                unsigned int &msLevel);

    //Subfunction of updateMassBins.
    Matrix<Byte> updateMassBins_(boost::dynamic_bitset<> &candidateMassBinsForThisSpectrum,
                                 float *massIntensities,
                                 long &binStart, long &binEnd,
                                 unsigned int &msLevel);

    //Subfunction of updateMassBins.
    boost::dynamic_bitset<> getCandidateMassBinsForThisSpectrum(float *massIntensitites,
                                                                float *mzIntensities,
                                                                double &mzMinValue,
                                                                unsigned int &msLevel);

    //From selected candidate mass bins, reselect the peaks. It researches the original spectrum to select peaks.
    //Also isotopic peaks are selected in this function.
    void getCandidatePeakGroups(double &mzBinMinValue,
                                double &massBinMinValue,
                                float *sumLogIntensities,
                                Matrix<Byte> chargeRanges,
                                FLASHDeconvHelperStructs::PrecalculatedAveragine &avg,
                                unsigned int &msLevel);

    //Make the universal pattern here..
    void setFilters();
  };
}
