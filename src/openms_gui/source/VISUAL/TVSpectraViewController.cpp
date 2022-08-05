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
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/VISUAL/TVSpectraViewController.h>

#include <OpenMS/CONCEPT/RAIICleanup.h>
#include <OpenMS/KERNEL/ChromatogramTools.h>
#include <OpenMS/KERNEL/OnDiscMSExperiment.h>
#include <OpenMS/VISUAL/APPLICATIONS/TOPPViewBase.h>
#include <OpenMS/VISUAL/AxisWidget.h>
#include <OpenMS/VISUAL/Plot1DWidget.h>

#include <QtWidgets/QMessageBox>
#include <QtCore/QString>

using namespace OpenMS;
using namespace std;

namespace OpenMS
{
  TVSpectraViewController::TVSpectraViewController(TOPPViewBase* parent):
    TVControllerBase(parent)
  {  
  }

  void TVSpectraViewController::showSpectrumAsNew1D(int index)
  {
    // basic behavior 1
    LayerDataBase& layer = tv_->getActiveCanvas()->getCurrentLayer();

    // create new 1D widget; if we return due to error, the widget will be cleaned up automatically
    unique_ptr<Plot1DWidget> wp(new Plot1DWidget(tv_->getCanvasParameters(1), DIM::Y, (QWidget*)tv_->getWorkspace()));
    Plot1DWidget* w = wp.get();
    
    // copy data from current layer (keeps the TYPE and underlying data identical)
    if (!w->canvas()->addLayer(layer.to1DLayer()))
    {
      // Behavior if its neither (user may have clicked on an empty tree or a
      // dummy entry as drawn by SpectraTreeTab::updateEntries)
      QMessageBox::critical(w, "Error", "Cannot open data that is neither chromatogram nor spectrum data. Aborting!");
      return;
    }

    w->canvas()->activateSpectrum(index);

    // set visible area to visible area in 2D view
    w->canvas()->setVisibleArea(tv_->getActiveCanvas()->getVisibleArea());

    // set relative (%) view of visible area
    w->canvas()->setIntensityMode(PlotCanvas::IM_SNAP);

    tv_->showPlotWidgetInWindow(wp.release());
    tv_->updateLayerBar();
    tv_->updateViewBar();
    tv_->updateFilterBar();
    tv_->updateMenu();
  }

  void TVSpectraViewController::showChromatogramsAsNew1D(const std::vector<int>& indices)
  {
    // show multiple spectra together is only used for chromatograms directly
    // where multiple (SRM) traces are shown together
    auto layer_chrom = dynamic_cast<LayerDataChrom*>(&tv_->getActiveCanvas()->getCurrentLayer());
    if (!layer_chrom) return;

    auto exp_sptr = layer_chrom->getChromatogramData();
    auto ondisc_sptr = layer_chrom->getOnDiscPeakData();

    // open new 1D widget
    Plot1DWidget* w = new Plot1DWidget(tv_->getCanvasParameters(1), DIM::Y, (QWidget *)tv_->getWorkspace());
    // use RT + intensity mapping
    w->setMapper({{DIM_UNIT::RT, DIM_UNIT::INT}});


    for (const auto& index : indices)
    {
      // set layer name
      String chromatogram_caption = layer_chrom->getName() + "[" + index + "]";

      // add chromatogram data
      if (!w->canvas()->addChromLayer(exp_sptr, ondisc_sptr, layer_chrom->getChromatogramAnnotation(), index, layer_chrom->filename, chromatogram_caption, true))
      {
        return;
      }
    }

    //w->canvas()->setVisibleArea(tv_->getActiveCanvas()->getVisibleArea()); 
    // set relative (%) view of visible area
    w->canvas()->setIntensityMode(PlotCanvas::IM_SNAP);

    tv_->showPlotWidgetInWindow(w);
    tv_->updateBarsAndMenus();
  }

  // called by SpectraTreeTab::spectrumSelected()
  void TVSpectraViewController::activate1DSpectrum(int index)
  {
    Plot1DWidget* widget_1d = tv_->getActive1DWidget();

    // return if no active 1D widget is present or no layers are present (e.g. the addPeakLayer call failed)
    if (widget_1d == nullptr) return;
    if (widget_1d->canvas()->getLayerCount() == 0) return;

    widget_1d->canvas()->activateSpectrum(index);
  }

  // called by SpectraTreeTab::chromsSelected()
  void TVSpectraViewController::activate1DSpectrum(const std::vector<int>& indices)
  {
    Plot1DWidget * widget_1d = tv_->getActive1DWidget();

    // return if no active 1D widget is present or no layers are present (e.g. the addPeakLayer call failed)
    if (widget_1d == nullptr) return;
    if (widget_1d->canvas()->getLayerCount() == 0) return;

    const LayerDataChrom* layer = dynamic_cast<LayerDataChrom*>(&widget_1d->canvas()->getCurrentLayer());
    if (!layer) return;

    ExperimentSharedPtrType chrom_sptr = layer->getChromatogramData();

    const String fname = layer->filename;
    auto annotation = layer->getChromatogramAnnotation();
    auto ondisc_sptr = layer->getOnDiscPeakData();
    widget_1d->canvas()->removeLayers(); // this actually deletes layer
    layer = nullptr;                     // ... make sure its not used any more

    widget_1d->canvas()->blockSignals(true);
    RAIICleanup clean([&]() {widget_1d->canvas()->blockSignals(false); });
    for (const auto& index : indices)
    {
      // get caption (either chromatogram idx or peptide sequence, if available)
      String caption = fname;
      if (chrom_sptr->metaValueExists("peptide_sequence"))
      {
        caption = String(chrom_sptr->getMetaValue("peptide_sequence"));
      }
      ((caption += "[") += index) += "]";
      // add chromatogram data as peak spectrum
      widget_1d->canvas()->addChromLayer(chrom_sptr, ondisc_sptr, annotation, index, fname, caption, true);
    }

    tv_->updateBarsAndMenus(); // needed since we blocked update above (to avoid repeated layer updates for many layers!)
  }

  void TVSpectraViewController::deactivate1DSpectrum(int /* spectrum_index */)
  {
    // no special handling of spectrum deactivation needed
  }

} // OpenMS

