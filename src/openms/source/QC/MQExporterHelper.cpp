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
// $Maintainer: Chris Bielow$
// $Authors: Virginia Rossow, Lenny Kovac, Hendrik Beschorner$
// --------------------------------------------------------------------------

#include <OpenMS/QC/MQExporterHelper.h>

#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/KERNEL/Feature.h>
#include <OpenMS/KERNEL/FeatureMap.h>
#include <OpenMS/MATH/MISC/MathFunctions.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/KERNEL/MSExperiment.h>

#include <QtCore/QDir>
#include <cmath> // isnan
#include <fstream>

using namespace OpenMS;

Size MQExporterHelper::proteinGroupID_(std::map<OpenMS::String, OpenMS::Size>& protein_id,
                                       const String& protein_accession)
{
  auto it = protein_id.find(protein_accession);
  if (it == protein_id.end())
  {
    protein_id.emplace(protein_accession, protein_id.size() + 1);
    return protein_id.size();
  }
  else
  {
    return it->second;
  }
}

std::map<Size, Size> MQExporterHelper::makeFeatureUIDtoConsensusMapIndex_(const ConsensusMap& cmap)
{
  std::map<Size, Size> f_to_ci;
  for (Size i = 0; i < cmap.size(); ++i)
  {
    for (const auto& fh : cmap[i].getFeatures())
    {
      auto[it, was_created_newly] = f_to_ci.emplace(fh.getUniqueId(), i);
      if (!was_created_newly)
      {
        throw Exception::Precondition(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                      "Adding [" + String(it->first) + "," + String(it->second) +  "] failed. FeatureHandle exists twice in ConsensusMap!");
      }
      f_to_ci[fh.getUniqueId()] = i;
    }
  }
  return f_to_ci;
}


bool MQExporterHelper::hasValidPepID_(
  const Feature& f,
  const Size c_feature_number,
  const std::multimap<OpenMS::String, std::pair<OpenMS::Size, OpenMS::Size>>& UIDs,
  const ProteinIdentification::Mapping& mp_f)
{
  const std::vector<PeptideIdentification>& pep_ids_f = f.getPeptideIdentifications();
  if (pep_ids_f.empty())
  {
    return false;
  }
  const PeptideIdentification& best_pep_id = pep_ids_f[0]; // PeptideIdentifications are sorted
  String best_uid = PeptideIdentification::buildUIDFromPepID(best_pep_id, mp_f.identifier_to_msrunpath);
  const auto range = UIDs.equal_range(best_uid);
  for (std::multimap<OpenMS::String, std::pair<OpenMS::Size, OpenMS::Size>>::const_iterator it_pep = range.first;
       it_pep != range.second; ++it_pep)
  {
    if (c_feature_number == it_pep->second.first)
    {
      return !pep_ids_f[0].getHits().empty(); // checks if PeptideIdentification has at least one hit
    }
  }
  return false;
}

bool MQExporterHelper::hasPeptideIdentifications_(const ConsensusFeature& cf)
{
  const std::vector<PeptideIdentification>& pep_ids_c = cf.getPeptideIdentifications();
  if (!pep_ids_c.empty())
  {
    return !pep_ids_c[0].getHits().empty(); // checks if PeptideIdentification has at least one hit
  }
  return false;
}

bool MQExporterHelper::isValid(std::string filename)
{
  return File::writable(filename);
}

MQExporterHelper::MQCommonOutputs::MQCommonOutputs(
  const OpenMS::Feature& f,
  const OpenMS::ConsensusMap& cmap,
  const OpenMS::Size c_feature_number,
  const std::multimap<OpenMS::String, std::pair<OpenMS::Size, OpenMS::Size>>& UIDs,
  const OpenMS::ProteinIdentification::Mapping& mp_f,
  const OpenMS::MSExperiment& exp,
  const std::map<OpenMS::String,OpenMS::String>& prot_mapper)
{
  const OpenMS::PeptideHit* ptr_best_hit; // the best hit referring to score
  const OpenMS::ConsensusFeature& cf = cmap[c_feature_number];
  if (MQExporterHelper::hasValidPepID_(f, c_feature_number, UIDs, mp_f))
  {
    ptr_best_hit = &f.getPeptideIdentifications()[0].getHits()[0];
  }
  else if (MQExporterHelper::hasPeptideIdentifications_(cf))
  {
    ptr_best_hit = &cf.getPeptideIdentifications()[0].getHits()[0];
  }
  else
  {
    return; // no valid PepID; nothing to export
  }

  const OpenMS::AASequence& pep_seq = ptr_best_hit->getSequence();
  if (pep_seq.empty())
  {
    return; // empty AASequence; nothing to export
  }

  std::map<OpenMS::String, OpenMS::Size> modifications_temp;
  if (pep_seq.hasNTerminalModification())
  {
    const OpenMS::String& n_terminal_modification = pep_seq.getNTerminalModificationName();
    modifications_temp.emplace(std::make_pair(n_terminal_modification, 1));
  }
  if (pep_seq.hasCTerminalModification())
  {
    modifications_temp.emplace(std::make_pair(pep_seq.getCTerminalModificationName(), 1));
  }
  for (OpenMS::Size i = 0; i < pep_seq.size(); ++i)
  {
    if (pep_seq.getResidue(i).isModified())
    {
      ++modifications_temp[pep_seq.getResidue(i).getModification()->getFullId()];
    }
  }

  if (modifications_temp.empty())
  {
    modifications.str("Unmodified");
  }
  else
  {
    auto it = modifications_temp.begin();
    modifications.str(it->first);
    ++it;
    for (; it != modifications_temp.end(); ++it)
    {
      modifications << ";" << it->first;
    }
  }

  if (pep_seq.hasNTerminalModification())
  {
    const OpenMS::String& n_terminal_modification = pep_seq.getNTerminalModificationName();
    n_terminal_modification.hasSubstring("Acetyl") ? acetyl.str("1") : acetyl.str("0"); // Acetyl (Protein N-term)
  }
  else
  {
    acetyl.str("0");
  }
  oxidation.clear();
  modifications_temp.find("Oxidation (M)") == modifications_temp.end() ? oxidation << "0" : oxidation << modifications_temp.find("Oxidation (M)")->second;
  const std::set<String>& accessions = ptr_best_hit->extractProteinAccessionsSet();
  std::vector<String> gene_names_temp;
  std::vector<String> protein_names_temp;
  for(const auto& prot_access : accessions)
  {
    const auto& prot_mapper_it = prot_mapper.find(prot_access);
    if(prot_mapper_it == prot_mapper.end())
    {
      continue;
    }
    else
    {
      auto protein_description = prot_mapper_it->second;
      auto gene_name = protein_description.substr(protein_description.find("GN=") + 3);
      gene_name = gene_name.substr(0,gene_name.find(" "));
      gene_names_temp.push_back(std::move(gene_name));
      protein_names_temp.push_back(std::move(protein_description));
    }

  }
  //gene_names.clear();
  //protein_names.clear();
  gene_names.str(ListUtils::concatenate(gene_names_temp, ';'));     //Gene Names
  protein_names.str(ListUtils::concatenate(protein_names_temp, ';'));  //Protein Names

  msms_mz.clear();
  if (f.metaValueExists("spectrum_index") && !exp.empty() && exp.getNrSpectra() >= (OpenMS::Size)f.getMetaValue("spectrum_index") && !exp[f.getMetaValue("spectrum_index")].empty())
  {
    const OpenMS::MSSpectrum& ms2_spec = exp[f.getMetaValue("spectrum_index")];
    if (!ms2_spec.getPrecursors().empty())
    {
      msms_mz << ms2_spec.getPrecursors()[0].getMZ(); // MS/MS m/z
    }
    else
    {
      msms_mz.str("");
    }
  }
  else
  {
    msms_mz.str("");
  }
  const double& uncalibrated_mz_error_ppm = ptr_best_hit->getMetaValue("uncalibrated_mz_error_ppm", NAN);
  const double& calibrated_mz_error_ppm = ptr_best_hit->getMetaValue("calibrated_mz_error_ppm", NAN);

  if (std::isnan(uncalibrated_mz_error_ppm) && std::isnan(calibrated_mz_error_ppm))
  {
    uncalibrated_calibrated_mz_ppm.str("NA"); // Uncalibrated - Calibrated m/z [ppm]
    uncalibrated_calibrated_mz_mda.str("NA"); // Uncalibrated - Calibrated m/z [mDa]
    mass_error_ppm.str("NA");                 // Mass error [ppm]
    mass_error_da.str("NA");                  // Mass error [Da]
    uncalibrated_mass_error_ppm.str("NA");    // Uncalibrated Mass error [ppm]
    uncalibrated_mass_error_da.str("NA");     // Uncalibrated Mass error [Da]
  }
  else if (std::isnan(calibrated_mz_error_ppm))
  {
    uncalibrated_calibrated_mz_ppm.str("NA"); // Uncalibrated - Calibrated m/z [ppm]
    uncalibrated_calibrated_mz_mda.str("NA"); // Uncalibrated - Calibrated m/z [mDa]
    mass_error_ppm.str("NA");                 // Mass error [ppm]
    mass_error_da.str("NA");                  // Mass error [Da]
    uncalibrated_mass_error_ppm.clear();
    uncalibrated_mass_error_da.clear();
    uncalibrated_mass_error_ppm << uncalibrated_mz_error_ppm;                                    // Uncalibrated Mass error [ppm]
    uncalibrated_mass_error_da << OpenMS::Math::ppmToMass(uncalibrated_mz_error_ppm, f.getMZ()); // Uncalibrated Mass error [Da]
  }
  else if (std::isnan(uncalibrated_mz_error_ppm))
  {
    uncalibrated_calibrated_mz_ppm.str("NA"); // Uncalibrated - Calibrated m/z [ppm]
    uncalibrated_calibrated_mz_mda.str("NA"); // Uncalibrated - Calibrated m/z [mDa]
    mass_error_ppm.clear();
    mass_error_da.clear();
    mass_error_ppm << calibrated_mz_error_ppm;                                    // Mass error [ppm]
    mass_error_da << OpenMS::Math::ppmToMass(calibrated_mz_error_ppm, f.getMZ()); // Mass error [Da]
    uncalibrated_mass_error_ppm.str("NA");                                        // Uncalibrated Mass error [ppm]
    uncalibrated_mass_error_da.str("NA");                                         // Uncalibrated Mass error [Da]
  }
  else
  {
    uncalibrated_calibrated_mz_ppm.clear(); // Uncalibrated - Calibrated m/z [ppm]
    uncalibrated_calibrated_mz_mda.clear(); // Uncalibrated - Calibrated m/z [mDa]
    mass_error_ppm.clear();                 // Mass error [ppm]
    mass_error_da.clear();                  // Mass error [Da]
    uncalibrated_mass_error_ppm.clear();    // Uncalibrated Mass error [ppm]
    uncalibrated_mass_error_da.clear();     // Uncalibrated Mass error [Da]

    uncalibrated_calibrated_mz_ppm << uncalibrated_mz_error_ppm - calibrated_mz_error_ppm;                                       // Uncalibrated - Calibrated m/z [ppm]
    uncalibrated_calibrated_mz_mda << OpenMS::Math::ppmToMass((uncalibrated_mz_error_ppm - calibrated_mz_error_ppm), f.getMZ()); // Uncalibrated - Calibrated m/z [Da]
    mass_error_ppm << calibrated_mz_error_ppm;                                                                                   // Mass error [ppm]
    mass_error_da << OpenMS::Math::ppmToMass(calibrated_mz_error_ppm, f.getMZ());                                                // Mass error [Da]
    uncalibrated_mass_error_ppm << uncalibrated_mz_error_ppm;                                                                    // Uncalibrated Mass error [ppm]
    uncalibrated_mass_error_da << OpenMS::Math::ppmToMass(uncalibrated_mz_error_ppm, f.getMZ());                                 // Uncalibrated Mass error [Da]
  }

  if(f.metaValueExists("spectrum_index") && f.metaValueExists("base_peak_intensity") && !exp.empty())
  {
    const MSSpectrum& ms2_spec = exp[f.getMetaValue("spectrum_index")];
    if (!ms2_spec.getPrecursors().empty())
    {
      base_peak_fraction.clear();
      base_peak_fraction << (ms2_spec.getPrecursors()[0].getIntensity() / (double)f.getMetaValue("base_peak_intensity")); // Base peak fraction
    }
    else
    {
      base_peak_fraction.str(""); // Base peak fraction
    }
  }
  else
  {
    base_peak_fraction.str(""); // Base peak fraction
  }
}