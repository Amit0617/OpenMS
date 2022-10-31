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
// $Authors: Stephan Aiche, Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/SIMULATION/LABELING/BaseLabeler.h>
#include <OpenMS/SIMULATION/LABELING/BaseLabeler_impl.h>

#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/DATASTRUCTURES/ListUtils.h>
#include <OpenMS/DATASTRUCTURES/ListUtilsIO.h>

#include <map>
#include <utility>

using std::vector;
using std::pair;
using std::set;

namespace OpenMS
{

  BaseLabeler::BaseLabeler() :
    DefaultParamHandler("BaseLabeler"),
    rng_(),
    channel_description_()
  {
    warn_empty_defaults_ = false;
  }

  BaseLabeler::~BaseLabeler()
  = default;

  Param BaseLabeler::getDefaultParameters() const
  {
    return this->defaults_;
  }

  void BaseLabeler::setRnd(SimTypes::MutableSimRandomNumberGeneratorPtr rng)
  {
    rng_ = std::move(rng);
  }

  String BaseLabeler::getChannelIntensityName(const Size channel_index) const
  {
    return String("channel_") + String(channel_index) + "_intensity";
  }

  SimTypes::FeatureMapSim BaseLabeler::mergeProteinIdentificationsMaps_(const SimTypes::FeatureMapSimVector& maps)
  {
    // we do not have any features yet (or at least we ignore them), so simply iterate over the protein
    // identifications
    std::map<String, ProteinHit> prot_hits;
    Size channel_index = 1;
    for (const FeatureMap& mapit : maps)
    {
      if (mapit.getProteinIdentifications().empty())
      {
        continue;
      }
      for (const ProteinHit& protein_hit : mapit.getProteinIdentifications()[0].getHits())
      {
        if (prot_hits.count(protein_hit.getSequence())) // we already know this protein -- sum up abundances
        {
          SimTypes::SimIntensityType new_intensity = prot_hits[protein_hit.getSequence()].getMetaValue("intensity");

          // remember channel intensity
          prot_hits[protein_hit.getSequence()].setMetaValue("intensity_" + String(channel_index), new_intensity);

          new_intensity += static_cast<SimTypes::SimIntensityType>(protein_hit.getMetaValue("intensity"));
          prot_hits[protein_hit.getSequence()].setMetaValue("intensity", new_intensity);
        }
        else // new protein hit .. remember
        {
          ProteinHit protHit(protein_hit);
          protHit.setMetaValue("intensity_" + String(channel_index), protHit.getMetaValue("intensity"));
          prot_hits.insert(std::pair<String, ProteinHit>(protein_hit.getSequence(), protHit));
        }
      }
      ++channel_index;
    }

    SimTypes::FeatureMapSim final_map;
    ProteinIdentification protIdent;

    for (std::map<String, ProteinHit>::iterator prot_hit_iter = prot_hits.begin(); prot_hit_iter != prot_hits.end(); ++prot_hit_iter)
    {
      protIdent.insertHit(prot_hit_iter->second);
    }
    std::vector<ProteinIdentification> protIdents;
    protIdents.push_back(protIdent);
    final_map.setProteinIdentifications(protIdents);

    return final_map;
  }

  void BaseLabeler::mergeProteinAccessions_(Feature& target, const Feature& source) const
  {
    std::set<String> target_acc = target.getPeptideIdentifications()[0].getHits()[0].extractProteinAccessionsSet();
    std::set<String> source_acc = source.getPeptideIdentifications()[0].getHits()[0].extractProteinAccessionsSet();

    // merge
    target_acc.insert(source_acc.begin(), source_acc.end());

    PeptideHit pepHit(target.getPeptideIdentifications()[0].getHits()[0]);

    for (std::set<String>::const_iterator a_it = target_acc.begin(); a_it != target_acc.end(); ++a_it)
    {
      PeptideEvidence pe;
      pe.setProteinAccession(*a_it);
      pepHit.addPeptideEvidence(pe);
    }

    std::vector<PeptideHit> pepHits;
    pepHits.push_back(pepHit);

    target.getPeptideIdentifications()[0].setHits(pepHits);
  }

  void BaseLabeler::recomputeConsensus_(const SimTypes::FeatureMapSim& simulated_features)
  {
    // iterate over all given features stored in the labeling consensus and try to find the corresponding feature in
    // in the feature map

    // build index for faster access
    std::map<String, IntList> id_map;
    std::map<UInt64, Size> features_per_labeled_map;
    for (Size i = 0; i < simulated_features.size(); ++i)
    {
      if (simulated_features[i].metaValueExists("parent_feature"))
      {
        OPENMS_LOG_DEBUG << "Checking [" << i << "]: " << simulated_features[i].getPeptideIdentifications()[0].getHits()[0].getSequence().toString()
                  << " with charge " << simulated_features[i].getCharge() << " (" << simulated_features[i].getMetaValue("charge_adducts") << ")"
                  << " parent was " << simulated_features[i].getMetaValue("parent_feature") << std::endl;
        id_map[simulated_features[i].getMetaValue("parent_feature")].push_back((Int)i);

        UInt64 map_index = 0;
        if (simulated_features[i].metaValueExists("map_index"))
        {
          map_index = simulated_features[i].getMetaValue("map_index");
        }
        ++features_per_labeled_map[map_index];
      }
    }

    for (std::map<String, IntList>::iterator it = id_map.begin(); it != id_map.end(); ++it)
    {
      OPENMS_LOG_DEBUG << it->first << " " << it->second << std::endl;
    }

    // new consensus map
    ConsensusMap new_cm;

    // initialize sub maps in consensus map
    for (std::map<UInt64, Size>::iterator it = features_per_labeled_map.begin(); it != features_per_labeled_map.end(); ++it)
    {
      new_cm.getColumnHeaders()[it->first].size = it->second;
      new_cm.getColumnHeaders()[it->first].unique_id = simulated_features.getUniqueId();
    }

    for (ConsensusFeature& cm : consensus_)
    {
      bool complete = true;

      OPENMS_LOG_DEBUG << "Checking consensus feature containing: " << std::endl;

      // check if we have all elements of current CF in the new feature map (simulated_features)
      for (const FeatureHandle& cf : cm)
      {
        complete &= id_map.find(String(cf.getUniqueId())) != id_map.end();
        OPENMS_LOG_DEBUG << "\t" << String(cf.getUniqueId()) << std::endl;
      }

      if (complete)
      {
        // get all elements sorted by charge state; since the same charge can be achieved by different
        // adduct compositions we use the adduct-string as indicator to find the groups
        std::map<String, std::set<FeatureHandle, FeatureHandle::IndexLess> > charge_mapping;

        for (const FeatureHandle& cf : cm)
        {
          IntList feature_indices = id_map[String(cf.getUniqueId())];

          for (const int f_index : feature_indices)
          {
            UInt64 map_index = 0;
            if (simulated_features[f_index].metaValueExists("map_index"))
            {
              map_index = simulated_features[f_index].getMetaValue("map_index");
            }

            if (charge_mapping.find(simulated_features[f_index].getMetaValue("charge_adducts")) != charge_mapping.end() )
            {
              charge_mapping[simulated_features[f_index].getMetaValue("charge_adducts")].insert(FeatureHandle(map_index, simulated_features[f_index]));
            }
            else
            {
              OPENMS_LOG_DEBUG << "Create new set with charge composition " << simulated_features[f_index].getMetaValue("charge_adducts") << std::endl;
              std::set<FeatureHandle, FeatureHandle::IndexLess> fh_set;

              fh_set.insert(FeatureHandle(map_index, simulated_features[f_index]));
              charge_mapping.insert(std::make_pair(simulated_features[f_index].getMetaValue("charge_adducts"), fh_set));
            }
          }
        }

        // create new consensus feature from derived features (separated by charge, if charge != 0)
        for (std::map<String, std::set<FeatureHandle, FeatureHandle::IndexLess> >::const_iterator charge_group_it = charge_mapping.begin();
             charge_group_it != charge_mapping.end();
             ++charge_group_it)
        {
          ConsensusFeature cf;
          cf.setCharge((*(*charge_group_it).second.begin()).getCharge());
          cf.setMetaValue("charge_adducts", charge_group_it->first);

          std::vector<PeptideIdentification> ids;
          for (std::set<FeatureHandle, FeatureHandle::IndexLess>::const_iterator fh_it = (charge_group_it->second).begin(); fh_it != (charge_group_it->second).end(); ++fh_it)
          {
            cf.insert(*fh_it);
            // append identifications
            Size f_index = simulated_features.uniqueIdToIndex(fh_it->getUniqueId());
            std::vector<PeptideIdentification> ids_feature = simulated_features[f_index].getPeptideIdentifications();
            ids.insert(ids.end(), ids_feature.begin(), ids_feature.end());
          }

          cf.computeMonoisotopicConsensus();
          cf.setPeptideIdentifications(ids);

          new_cm.push_back(cf);
        }

      }
    }

    new_cm.setProteinIdentifications(simulated_features.getProteinIdentifications());

    consensus_.swap(new_cm);
    consensus_.applyMemberFunction(&UniqueIdInterface::ensureUniqueId);
  }

  ConsensusMap& BaseLabeler::getConsensus()
  {
    return consensus_;
  }

  const String& BaseLabeler::getDescription() const
  {
    return channel_description_;
  }

} // namespace OpenMS
