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
// $Maintainer: Mathias Walzer $
// $Authors: $
// --------------------------------------------------------------------------
//

#include <OpenMS/COMPARISON/SPECTRA/BinnedSharedPeakCount.h>

using namespace std;

#include <Eigen/Sparse>

namespace OpenMS
{
  BinnedSharedPeakCount::BinnedSharedPeakCount() :
    BinnedSpectrumCompareFunctor()
  {
    setName(BinnedSharedPeakCount::getProductName());
    defaultsToParam_();
  }

  BinnedSharedPeakCount::BinnedSharedPeakCount(const BinnedSharedPeakCount& source) :
    BinnedSpectrumCompareFunctor(source)
  {
  }

  BinnedSharedPeakCount::~BinnedSharedPeakCount() = default;

  BinnedSharedPeakCount& BinnedSharedPeakCount::operator=(const BinnedSharedPeakCount& source)
  {
    if (this != &source)
    {
      BinnedSpectrumCompareFunctor::operator=(source);
    }
    return *this;
  }

  double BinnedSharedPeakCount::operator()(const BinnedSpectrum& spec) const
  {
    return operator()(spec, spec);
  }

  void BinnedSharedPeakCount::updateMembers_()
  {
  }

  double BinnedSharedPeakCount::operator()(const BinnedSpectrum& spec1, const BinnedSpectrum& spec2) const
  {
    OPENMS_PRECONDITION(BinnedSpectrum::isCompatible(spec1, spec2), "Binned spectra have different bin size or spread");

    size_t denominator(max(spec1.getBins()->nonZeros(), spec2.getBins()->nonZeros()));

    // Note: keep in single expression for faster computation via expression templates
    // Calculate coefficient-wise product and count non-zero entries
    BinnedSpectrum::SparseVectorType s = spec1.getBins()->cwiseProduct(*spec2.getBins());
    
    // resulting score normalized to interval [0,1]
    return static_cast<double>(s.nonZeros()) / denominator;
  }

}
