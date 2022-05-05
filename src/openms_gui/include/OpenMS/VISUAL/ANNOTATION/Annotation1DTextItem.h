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
// $Maintainer: Timo Sachsenberg $
// $Authors: Johannes Junker, Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/VISUAL/ANNOTATION/Annotation1DItem.h>

namespace OpenMS
{
  /** @brief An annotation item which represents an arbitrary text on the canvas.
          @see Annotation1DItem
  */
  template <class DataPoint>
  class Annotation1DTextItem :
    public Annotation1DItem
  {
public:

    /// Constructor
    Annotation1DTextItem(const DataPoint& position, const QString& text, const int flags = Qt::AlignCenter)
      : Annotation1DItem(text), position_(position), flags_(flags)
    {
    }
    /// Copy constructor
    Annotation1DTextItem(const Annotation1DTextItem & rhs) = default;

    /// Destructor
    ~Annotation1DTextItem() override = default;

    // Docu in base class
    void ensureWithinDataRange(Plot1DCanvas* const canvas, const int layer_index) override
    {
      canvas->pushIntoDataRange(position_, layer_index);
    }

    // Docu in base class
    void draw(Plot1DCanvas* const canvas, QPainter& painter, bool flipped = false) override
    {
      // translate units to pixel coordinates
      QPoint pos_text;
      canvas->dataToWidget(canvas->getMapper().map(position_), pos_text, flipped);

      // compute bounding box of text_item on the specified painter
      bounding_box_ = painter.boundingRect(QRectF(pos_text, pos_text), flags_, text_);

      painter.drawText(bounding_box_, flags_, text_);
      if (selected_)
      {
        drawBoundingBox_(painter);
      }
    }

    // Docu in base class
    void move(PointXYType delta, const Gravitator& gr, const DimMapper<2>& dim_mapper) override
    {
      auto pos_xy = dim_mapper.map(position_);
      pos_xy += delta;
      dim_mapper.fromXY(pos_xy, position_);
    }

    /// Sets the position of the item (in MZ / intensity coordinates)
    void setPosition(const DataPoint& position)
    {
      position_ = position;
    }

    /// Returns the position of the item (in MZ / intensity coordinates)
    const DataPoint& getPosition() const
    {
      return position_;
    }

    /// Set Qt flags (default: Qt::AlignCenter)
    void setFlags(int flags)
    {
      flags_ = flags;
    }

    /// Get Qt flags
    int getFlags() const
    {
      return flags_;
    }

    // Docu in base class
    Annotation1DItem* clone() const override
    {
      return new Annotation1DTextItem(*this);
    }

  protected:
    /// The position of the item as a datatype, e.g. Peak1D
    DataPoint position_;

    int flags_;
  };
} // namespace OpenMS

