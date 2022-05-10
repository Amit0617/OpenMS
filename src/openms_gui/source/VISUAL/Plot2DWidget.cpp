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
// $Authors: Marc Sturm $
// --------------------------------------------------------------------------

// OpenMS
#include <OpenMS/VISUAL/Plot1DWidget.h>
#include <OpenMS/VISUAL/Plot2DWidget.h>
#include <OpenMS/VISUAL/AxisWidget.h>
#include <OpenMS/VISUAL/DIALOGS/Plot2DGoToDialog.h>
#include <OpenMS/CONCEPT/UniqueIdInterface.h>
#include <OpenMS/KERNEL/OnDiscMSExperiment.h>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QCheckBox>
#include <QtCore/QTimer>

using namespace std;

namespace OpenMS
{
  using namespace Internal;
  using namespace Math;

  Plot2DWidget::Plot2DWidget(const Param& preferences, QWidget* parent) :
    PlotWidget(preferences, parent)
  {
    setCanvas_(new Plot2DCanvas(preferences, this), 1, 2);

    // add axes
    y_axis_->setMinimumWidth(50);

    // add projections
    grid_->setColumnStretch(2, 3);
    grid_->setRowStretch(1, 3);

    /*PlotCanvas::ExperimentSharedPtrType shr_ptr = PlotCanvas::ExperimentSharedPtrType(new PlotCanvas::ExperimentType());
    LayerDataBase::ODExperimentSharedPtrType od_dummy(new OnDiscMSExperiment());
    MSSpectrum dummy_spec;
    dummy_spec.push_back(Peak1D());
    shr_ptr->addSpectrum(dummy_spec);*/

    projection_vert_ = new Plot1DWidget(Param(), DIM::X, this);
    projection_vert_->hide();
    //projection_vert_->canvas()->addLayer(shr_ptr, od_dummy);
    grid_->addWidget(projection_vert_, 1, 3, 2, 1);

    projection_horz_ = new Plot1DWidget(Param(), DIM::Y, this);
    projection_horz_->hide();
    // projection_horz_->canvas()->addLayer(shr_ptr, od_dummy);
    grid_->addWidget(projection_horz_, 0, 1, 1, 2);

    // decide on default draw mode, depending on axis unit
    auto set_style = [&](const DIM_UNIT unit, Plot1DCanvas* canvas)
    {
      switch (unit)
      { // this may not be optimal for every unit. Feel free to change behavior.
        case DIM_UNIT::MZ:
          // to show isotope distributions as sticks
          canvas->setDrawMode(Plot1DCanvas::DM_PEAKS);
          canvas->setIntensityMode(PlotCanvas::IM_PERCENTAGE);
          break;
        // all other units
        default:
          canvas->setDrawMode(Plot1DCanvas::DM_CONNECTEDLINES);
          canvas->setIntensityMode(PlotCanvas::IM_SNAP);
          break;
      }
    };
    set_style(canvas_->getMapper().getDim(DIM::X).getUnit(), projection_horz_->canvas());
    set_style(canvas_->getMapper().getDim(DIM::Y).getUnit(), projection_vert_->canvas());

    connect(canvas(), SIGNAL(showProjectionHorizontal(ExperimentSharedPtrType)), this, SLOT(horizontalProjection(ExperimentSharedPtrType)));
    connect(canvas(), SIGNAL(showProjectionVertical(ExperimentSharedPtrType)), this, SLOT(verticalProjection(ExperimentSharedPtrType)));
    connect(canvas(), SIGNAL(showProjectionInfo(int, double, double)), this, SLOT(projectionInfo(int, double, double)));
    connect(canvas(), SIGNAL(toggleProjections()), this, SLOT(toggleProjections()));
    connect(canvas(), SIGNAL(visibleAreaChanged(DRange<2>)), this, SLOT(autoUpdateProjections()));
    // delegate signals from canvas
    connect(canvas(), SIGNAL(showSpectrumAsNew1D(int)), this, SIGNAL(showSpectrumAsNew1D(int)));
    connect(canvas(), SIGNAL(showChromatogramsAsNew1D(std::vector<int, std::allocator<int> >)), this, SIGNAL(showChromatogramsAsNew1D(std::vector<int, std::allocator<int> >)));
    connect(canvas(), SIGNAL(showCurrentPeaksAs3D()), this, SIGNAL(showCurrentPeaksAs3D()));
    // add projections box
    projection_box_ = new QGroupBox("Projections", this);
    projection_box_->hide();
    grid_->addWidget(projection_box_, 0, 3);
    QGridLayout* box_grid = new QGridLayout(projection_box_);

    QLabel* label = new QLabel("Peaks: ");
    box_grid->addWidget(label, 0, 0);
    projection_peaks_ = new QLabel("");
    box_grid->addWidget(projection_peaks_, 0, 1);

    label = new QLabel("Intensity sum: ");
    box_grid->addWidget(label, 1, 0);
    projection_sum_ = new QLabel("");
    box_grid->addWidget(projection_sum_, 1, 1);

    label = new QLabel("Maximum intensity: ");
    box_grid->addWidget(label, 2, 0);
    projection_max_ = new QLabel("");
    box_grid->addWidget(projection_max_, 2, 1);

    box_grid->setRowStretch(3, 2);

    QPushButton* button = new QPushButton("Update", projection_box_);
    connect(button, SIGNAL(clicked()), canvas(), SLOT(updateProjections()));
    box_grid->addWidget(button, 4, 0);

    projections_auto_ = new QCheckBox("Auto-update", projection_box_);
    projections_auto_->setWhatsThis("When activated, projections are automatically updated one second after the last change of the visible area.");
    projections_auto_->setChecked(true);
    box_grid->addWidget(projections_auto_, 4, 1);

    //set up projections auto-update
    projections_timer_ = new QTimer(this);
    projections_timer_->setSingleShot(true);
    projections_timer_->setInterval(1000);
    connect(projections_timer_, SIGNAL(timeout()), this, SLOT(updateProjections()));
  }

  Plot2DWidget::~Plot2DWidget()
  {
  }

  void Plot2DWidget::projectionInfo(int peaks, double intensity, double max)
  {
    projection_peaks_->setText(QString::number(peaks));
    projection_sum_->setText(QString::number(intensity, 'f', 1));
    projection_max_->setText(QString::number(max, 'f', 1));
  }

  void Plot2DWidget::recalculateAxes_()
  {
    const auto& area = canvas()->getVisibleArea().getAreaXY();
    x_axis_->setAxisBounds(area.minX(), area.maxX());
    y_axis_->setAxisBounds(area.minY(), area.maxY());
  }

  void Plot2DWidget::updateProjections()
  {
    canvas()->updateProjections();
  }

  void Plot2DWidget::toggleProjections()
  {
    if (projectionsVisible())
    {
      setMinimumSize(250, 250);
      projection_box_->hide();
      projection_horz_->hide();
      projection_vert_->hide();
      grid_->setColumnStretch(3, 0);
      grid_->setRowStretch(0, 0);
    }
    else
    {
      setMinimumSize(500, 500);
      updateProjections();
    }
  }

  //  projection above the 2D area
  void Plot2DWidget::horizontalProjection(ExperimentSharedPtrType exp)
  {
    LayerDataBase::ODExperimentSharedPtrType od_dummy(new OnDiscMSExperiment());

    projection_horz_->showLegend(false);

    projection_horz_->canvas()->removeLayers();
    projection_horz_->canvas()->addLayer(exp, od_dummy);

    grid_->setColumnStretch(3, 2);

    projection_horz_->show();
    projection_box_->show();
  }

  // projection on the right side of the 2D area
  void Plot2DWidget::verticalProjection(ExperimentSharedPtrType exp)
  {
    LayerDataBase::ODExperimentSharedPtrType od_dummy(new OnDiscMSExperiment());

    projection_vert_->showLegend(false);

    projection_vert_->canvas()->removeLayers();
    projection_vert_->canvas()->addLayer(exp, od_dummy);

    grid_->setRowStretch(0, 2);
    
    projection_box_->show();
    projection_vert_->show();
  }

  const Plot1DWidget* Plot2DWidget::getHorizontalProjection() const
  {
    return projection_horz_;
  }

  const Plot1DWidget* Plot2DWidget::getVerticalProjection() const
  {
    return projection_vert_;
  }

  void Plot2DWidget::showGoToDialog()
  {
    Plot2DGoToDialog goto_dialog(this);
    //set range
    const auto& area = canvas()->getVisibleArea().getAreaXY();
    goto_dialog.setRange(area.minY(), area.maxY(), area.minX(), area.maxX());

    auto all_area_xy = canvas_->getMapper().mapRange(canvas_->getDataRange());
    goto_dialog.setMinMaxOfRange(all_area_xy.minX(), all_area_xy.maxX(), all_area_xy.minY(), all_area_xy.maxY());
    // feature numbers only for consensus&feature maps
    goto_dialog.enableFeatureNumber(canvas()->getCurrentLayer().type == LayerDataBase::DT_FEATURE || canvas()->getCurrentLayer().type == LayerDataBase::DT_CONSENSUS);
    // execute
    if (goto_dialog.exec())
    {
      if (goto_dialog.showRange())
      {
        goto_dialog.fixRange();
        PlotCanvas::AreaXYType area_new(goto_dialog.getMinRT(), goto_dialog.getMinMZ(), goto_dialog.getMaxRT(), goto_dialog.getMaxMZ());
        canvas()->setVisibleArea(area_new);
      }
      else
      {
        String feature_id = goto_dialog.getFeatureNumber();
        //try to convert to UInt64 id
        UniqueIdInterface uid;
        uid.setUniqueId(feature_id);

        Size feature_index(-1); // TODO : not use -1
        if (canvas()->getCurrentLayer().type == LayerDataBase::DT_FEATURE)
        {
          feature_index = canvas()->getCurrentLayer().getFeatureMap()->uniqueIdToIndex(uid.getUniqueId());
        }
        else if (canvas()->getCurrentLayer().type == LayerDataBase::DT_CONSENSUS)
        {
          feature_index = canvas()->getCurrentLayer().getConsensusMap()->uniqueIdToIndex(uid.getUniqueId());
        }
        if (feature_index == Size(-1)) // UID does not exist
        {
          try
          {
            feature_index = feature_id.toInt(); // normal feature index as stored in map
          }
          catch (...) // we might still deal with a UID, so toInt() will throw as the number is too big
          {
            feature_index = Size(-1);
          }
        }

        //check if the feature index exists
        if ((canvas()->getCurrentLayer().type == LayerDataBase::DT_FEATURE && feature_index >= canvas()->getCurrentLayer().getFeatureMap()->size())
           || (canvas()->getCurrentLayer().type == LayerDataBase::DT_CONSENSUS && feature_index >= canvas()->getCurrentLayer().getConsensusMap()->size()))
        {
          QMessageBox::warning(this, "Invalid feature number", "Feature number too large/UniqueID not found.\nPlease select a valid feature!");
          return;
        }
        //display feature with a margin
        if (canvas()->getCurrentLayer().type == LayerDataBase::DT_FEATURE)
        {
          const FeatureMap& map = *canvas()->getCurrentLayer().getFeatureMap();
          const DBoundingBox<2> bb = map[feature_index].getConvexHull().getBoundingBox();
          RangeAllType range;
          range.RangeRT::operator=(RangeBase{bb.minPosition()[0], bb.maxPosition()[0]});
          range.RangeMZ::operator=(RangeBase{bb.minPosition()[1], bb.maxPosition()[1]});
          range.RangeRT::scaleBy(2);
          range.RangeRT::scaleBy(5);
          canvas()->setVisibleArea(range);
        }
        else // Consensus Feature
        {
          const ConsensusFeature& cf = (*canvas()->getCurrentLayer().getConsensusMap())[feature_index];
          auto range = canvas_->getMapper().fromXY(canvas_->getMapper().map(cf));
          range.RangeRT::extendLeftRight(30);
          range.RangeMZ::extendLeftRight(5);
          canvas()->setVisibleArea(range);
        }

      }
    }
  }

  bool Plot2DWidget::projectionsVisible() const
  {
    return projection_horz_->isVisible() || projection_vert_->isVisible();
  }

  void Plot2DWidget::autoUpdateProjections()
  {
    if (projectionsVisible() && projections_auto_->isChecked())
    {
      projections_timer_->start();
    }
  }

} //Namespace
