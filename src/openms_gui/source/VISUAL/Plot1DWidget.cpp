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
// $Maintainer: Timo Sachsenberg$
// $Authors: Marc Sturm, Timo Sachsenberg $
// --------------------------------------------------------------------------

// OpenMS
#include <OpenMS/VISUAL/Plot1DWidget.h>

#include <OpenMS/VISUAL/AxisWidget.h>
#include <OpenMS/VISUAL/DIALOGS/Plot1DGoToDialog.h>

#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QFileDialog>
#include <QPainter>
#include <QPaintEvent>
#include <QtSvg/QtSvg>
#include <QtSvg/QSvgGenerator>

using namespace std;

namespace OpenMS
{
  using namespace Internal;
  using namespace Math;

  Plot1DWidget::Plot1DWidget(const Param& preferences, const DIM gravity_axis, QWidget* parent) :
    PlotWidget(preferences, parent)
  {
    x_axis_->setAllowShortNumbers(false);

    y_axis_->setAllowShortNumbers(true);
    y_axis_->setMinimumWidth(50);

    flipped_y_axis_ = new AxisWidget(AxisPainter::LEFT, "", this);
    flipped_y_axis_->setInverseOrientation(true);
    flipped_y_axis_->setAllowShortNumbers(true);
    flipped_y_axis_->setMinimumWidth(50);
    flipped_y_axis_->hide();

    spacer_ = new QSpacerItem(0, 0);

    // set the label mode for the axes  - side effect
    setCanvas_(new Plot1DCanvas(preferences, gravity_axis, this));

    // Delegate signals
    connect(canvas(), SIGNAL(showCurrentPeaksAs2D()), this, SIGNAL(showCurrentPeaksAs2D()));
    connect(canvas(), SIGNAL(showCurrentPeaksAs3D()), this, SIGNAL(showCurrentPeaksAs3D()));
    connect(canvas(), SIGNAL(showCurrentPeaksAsIonMobility()), this, SIGNAL(showCurrentPeaksAsIonMobility()));
    connect(canvas(), SIGNAL(showCurrentPeaksAsDIA()), this, SIGNAL(showCurrentPeaksAsDIA()));
  }

  void Plot1DWidget::recalculateAxes_()
  {
    // set names
    x_axis_->setLegend(string(canvas()->getMapper().getDim(DIM::X).getDimName()));
    y_axis_->setLegend(string(canvas()->getMapper().getDim(DIM::Y).getDimName()));

    // determine which is the gravity axis (usually equals intensity axis (for LOG mode))
    AxisWidget* other_axis = x_axis_;
    AxisWidget* int_axis = y_axis_;
    auto vis_area_xy = canvas_->getVisibleArea().getAreaXY();
    auto all_area_xy = canvas_->getMapper().mapRange(canvas_->getDataRange());
    // in the unusual case: gravity is on X axis
    if (canvas()->getGravitator().getGravityAxis() == DIM::X)
    {
      swap(other_axis, int_axis);
      vis_area_xy.swapDimensions();
      all_area_xy.swapDimensions();
    }
    // from now on, we can assume X-dim = data; Y-dim = gravity=intensity

    // deal with log scaling for intensity axis
    int_axis->setLogScale(canvas()->getIntensityMode() == PlotCanvas::IM_LOG);

    // recalculate gridlines
    other_axis->setAxisBounds(vis_area_xy.minX(), vis_area_xy.maxX());
    switch (canvas()->getIntensityMode())
    {
      case PlotCanvas::IM_NONE:
      case PlotCanvas::IM_LOG:
        int_axis->setAxisBounds(vis_area_xy.minY(), vis_area_xy.maxY());
        break;

      case PlotCanvas::IM_PERCENTAGE:
        int_axis->setAxisBounds(vis_area_xy.minY() / all_area_xy.maxY() * 100.0,
                                vis_area_xy.maxY() / all_area_xy.maxY() * Plot1DCanvas::TOP_MARGIN * 100.0);
        break;

    case PlotCanvas::IM_SNAP:
        int_axis->setAxisBounds(vis_area_xy.minY() / canvas()->getSnapFactor(), vis_area_xy.maxY() / canvas()->getSnapFactor());
      break;


    default:
      throw Exception::NotImplemented(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION);
    }

    // assume flipped-y-axis is identical
    flipped_y_axis_->setLegend(y_axis_->getLegend());
    flipped_y_axis_->setLogScale(y_axis_->isLogScale());
    flipped_y_axis_->setAxisBounds(y_axis_->getAxisMinimum(), y_axis_->getAxisMaximum());
  }

  Plot1DWidget::~Plot1DWidget()
  {
    delete spacer_;
  }

  void Plot1DWidget::showGoToDialog()
  {
    Plot1DGoToDialog goto_dialog(this);
    auto vis_area_xy = canvas_->getVisibleArea().getAreaXY();
    auto all_area_xy = canvas_->getMapper().mapRange(canvas_->getDataRange());
    // in the unusual case: gravity is on X axis
    if (canvas()->getGravitator().getGravityAxis() == DIM::X)
    {
      vis_area_xy.swapDimensions();
      all_area_xy.swapDimensions();
    }
    goto_dialog.setRange(vis_area_xy.minX(), vis_area_xy.maxX());
    goto_dialog.setMinMaxOfRange(all_area_xy.minX(), all_area_xy.maxX());
    if (goto_dialog.exec())
    {
      goto_dialog.fixRange();
      PlotCanvas::AreaXYType area(goto_dialog.getMin(), 0, goto_dialog.getMax(), 0);
      if (canvas()->getGravitator().getGravityAxis() == DIM::X)
      {
        area.swapDimensions();
      }
      auto va_new = canvas_->getVisibleArea().cloneWith(area);
      canvas()->setVisibleArea(va_new);
    }
  }

  void Plot1DWidget::showLegend(bool show)
  {
    y_axis_->showLegend(show);
    flipped_y_axis_->showLegend(show);
    x_axis_->showLegend(show);
    update();
  }

  void Plot1DWidget::hideAxes()
  {
    y_axis_->hide();
    flipped_y_axis_->hide();
    x_axis_->hide();
  }

  void Plot1DWidget::toggleMirrorView(bool mirror)
  {
    if (mirror)
    {
      grid_->addItem(spacer_, 1, 1);
      grid_->addWidget(flipped_y_axis_, 2, 1);
      grid_->removeWidget(canvas());
      grid_->removeWidget(x_axis_);
      grid_->removeWidget(x_scrollbar_);
      grid_->addWidget(canvas(), 0, 2, 3, 1); // rowspan = 3
      grid_->addWidget(x_axis_, 3, 2);
      grid_->addWidget(x_scrollbar_, 4, 2);
      flipped_y_axis_->show();
    }
    else
    {
      grid_->removeWidget(canvas());
      grid_->removeWidget(flipped_y_axis_);
      flipped_y_axis_->hide();
      grid_->removeItem(spacer_);
      grid_->removeWidget(x_axis_);
      grid_->removeWidget(x_scrollbar_);
      grid_->addWidget(canvas(), 0, 2);
      grid_->addWidget(x_axis_, 1, 2);
      grid_->addWidget(x_scrollbar_, 2, 2);
    }
  }

  void Plot1DWidget::performAlignment(Size layer_index_1, Size layer_index_2, const Param& param)
  {
    spacer_->changeSize(0, 10);
    grid_->removeWidget(y_axis_);
    grid_->removeWidget(flipped_y_axis_);
    grid_->addWidget(y_axis_, 0, 1);
    grid_->addWidget(flipped_y_axis_, 2, 1);

    canvas()->performAlignment(layer_index_1, layer_index_2, param);
  }

  void Plot1DWidget::resetAlignment()
  {
    spacer_->changeSize(0, 0);
    grid_->removeWidget(y_axis_);
    grid_->removeWidget(flipped_y_axis_);
    grid_->addWidget(y_axis_, 0, 1);
    grid_->addWidget(flipped_y_axis_, 2, 1);
  }

  void Plot1DWidget::renderForImage(QPainter& painter)
  {
    bool x_visible = x_scrollbar_->isVisible();
    bool y_visible = y_scrollbar_->isVisible();
    x_scrollbar_->hide();
    y_scrollbar_->hide();
    this->render(&painter);
    x_scrollbar_->setVisible(x_visible);
    y_scrollbar_->setVisible(y_visible);
  }

  void Plot1DWidget::saveAsImage()
  {
    QString filter = "Raster images *.bmp *.png *.jpg *.gif (*.bmp *.png *.jpg *.gif);;Vector images *.svg (*.svg)";
    QString sel_filter;
    QString file_name = QFileDialog::getSaveFileName(this, "Save File", "", filter, &sel_filter);
    
    bool x_visible = x_scrollbar_->isVisible();
    bool y_visible = y_scrollbar_->isVisible();
    x_scrollbar_->hide();
    y_scrollbar_->hide();

    if (sel_filter.contains(".svg", Qt::CaseInsensitive)) // svg vector format
    {
      QSvgGenerator generator;
      generator.setFileName(file_name);
      generator.setSize(QSize(this->width(), this->height()));
      generator.setViewBox(QRect(0, 0, this->width() - 1, this->height() - 1));
      generator.setTitle(file_name);
      generator.setDescription("TOPPView generated SVG");
      QPainter painter;
      painter.begin(&generator);

      painter.save();
      painter.translate(QPoint(y_axis_->pos()));
      y_axis_->paint(&painter, new QPaintEvent(y_axis_->contentsRect()));
      painter.restore();

      painter.save();
      painter.translate(QPoint(canvas_->pos()));
      dynamic_cast<Plot1DCanvas*>(canvas_)->paint(&painter, new QPaintEvent(canvas_->contentsRect()));
      painter.restore();

      painter.save();
      painter.translate(QPoint(x_axis_->pos()));
      x_axis_->paint(&painter, new QPaintEvent(x_axis_->contentsRect()));
      painter.restore();

      painter.end();
      x_scrollbar_->setVisible(x_visible);
      y_scrollbar_->setVisible(y_visible);
    }
    else // raster graphics formats
    {
      QPixmap pixmap = this->grab();
      x_scrollbar_->setVisible(x_visible);
      y_scrollbar_->setVisible(y_visible);
      pixmap.save(file_name);
    }
  }

} //namespace
