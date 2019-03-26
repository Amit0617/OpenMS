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
// $Maintainer: Chris Bielow $
// $Authors: Tom Waschischeck $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/CONCEPT/Types.h>

namespace OpenMS
{
  /**
   * @brief This class serves as an abstract base class for all QC classes.
   *
   * It contains the important feature of encoding the input requirements
   * for a certain QC.
   */
  class OPENMS_DLLAPI QCBase
  {
  public:
    /**
     * @brief Enum to encode a file type as a bit.
     */
    enum class Requires :
        UInt64
    {
      FAIL = 0,
      RAWMZML = 1,
      POSTFDRFEAT = 2,
      PREFDRFEAT = 4,
      CONTAMINANTS = 8
    };
    /**
     * @brief Storing a status as a UInt64
     *
     * Only allows assignment and bit operations with itself and an object
     * of type Requires, i.e. not with any numeric types.
     */
    class Status
    {
    public:
      // Constructors
      Status() : value_(0)
      {}
      Status(const Requires& req)
      {
        value_ = UInt64(req);
      }
      Status(const Status& stat)
      {
        value_ = stat.value_;
      }
      // Assignment
      Status& operator=(const Requires& req)
      {
        value_ = UInt64(req);
        return *this;
      }
      // Equal
      bool operator==(const Status& stat)
      {
        return (value_ == stat.value_);
      }
      Status& operator=(const Status& stat) = default;
      // Bitwise operators
      Status operator&(const Requires& req) const
      {
        Status s = *this;
        s.value_ &= UInt64(req);
        return s;
      }
      Status operator&(const Status& stat) const
      {
        Status s = *this;
        s.value_ &= stat.value_;
        return s;
      }
      Status& operator&=(const Requires& req)
      {
        value_ &= UInt64(req);
        return *this;
      }
      Status& operator&=(const Status& stat)
      {
        value_ &= stat.value_;
        return *this;
      }
      Status operator|(const Requires& req) const
      {
        Status s = *this;
        s.value_ |= UInt64(req);
        return s;
      }
      Status operator|(const Status& stat) const
      {
        Status s = *this;
        s.value_ |= stat.value_;
        return s;
      }
      Status& operator|=(const Requires& req)
      {
        value_ |= UInt64(req);
        return *this;
      }
      Status& operator|=(const Status& stat)
      {
        value_ |= stat.value_;
        return *this;
      }

      /**
       * @brief Check if input status fulfills requirement status.
       */
      bool isSuperSetOf(const Status& stat)
      {
        return ((value_ & stat.value_) == stat.value_);
      }
    private:
      UInt64 value_;
    };
    /**
     *@brief Returns the input data requirements of the compute(...) function
     */
    virtual Status requires() const = 0;
  };
}