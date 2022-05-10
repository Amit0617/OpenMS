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
// $Authors: Hendrik Beschorner, Lenny Kovac, Virginia Rossow$
// --------------------------------------------------------------------------

#include <OpenMS/QC/MQMsmsExporter.h>

#include <OpenMS/QC/MQEvidenceExporter.h>
#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/KERNEL/Feature.h>
#include <OpenMS/KERNEL/FeatureMap.h>
#include <OpenMS/MATH/MISC/MathFunctions.h>
#include <OpenMS/SYSTEM/File.h>

#include <QtCore/QDir>
#include <cmath> // isnan
#include <fstream>


using namespace OpenMS;


MQMsms::MQMsms(const String& path)
{
  if (path.empty())
  {
    return;
  }
  filename_ = path + "/msms.txt";
  try
  {
    QString evi_path = QString::fromStdString(path);
    QDir().mkpath(evi_path);
    file_ = std::fstream(filename_, std::fstream::out);
  }
  catch (...)
  {
    OPENMS_LOG_FATAL_ERROR << filename_ << " wasn’t created" << std::endl;
    throw Exception::FileNotWritable(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "out_evd");
  }
  exportHeader_();
}


MQMsms::~MQMsms()
{
  file_.close();
}


bool MQMsms::isValid()
{
  return File::writable(filename_);
}


void MQMsms::exportHeader_()
{

  file_ << "Raw file" << "\t"; // ok
  file_ << "Scan number" << "\t";
  file_ << "Scan index" << "\t";
  file_ << "Sequence" << "\t"; // ok
  file_ << "Length" << "\t"; // ok
  file_ << "Missed cleavages" << "\t"; // vermutlich ok
  file_ << "Modifications" << "\t"; // ok
  file_ << "Modified sequence" << "\t"; // ok
  file_ << "Oxidation (M) Probabilities" << "\t";
  file_ << "Oxidation (M) Score diffs" << "\t";
  file_ << "Acetyl (Protein N-term)" << "\t"; // vermutlich ok
  file_ << "Oxidation (M)" << "\t"; // ok
  file_ << "Proteins" << "\t"; // ok
  file_ << "Charge" << "\t"; // ok
  file_ << "Fragmentation" << "\t";
  file_ << "Mass analyzer" << "\t";
  file_ << "Type" << "\t"; // ok
  file_ << "Scan event number" << "\t";
  file_ << "Isotope index" << "\t";
  file_ << "m/z" << "\t"; // ok
  file_ << "Mass" << "\t"; 
  file_ << "Mass error [ppm]" << "\t";
  file_ << "Mass error [Da]" << "\t";
  file_ << "Simple mass error [ppm]" << "\t";
  file_ << "Retention time" << "\t"; // vermutlich ok
  file_ << "PEP" << "\t";
  file_ << "Score" << "\t"; 
  file_ << "Delta score" << "\t";
  file_ << "Score diff" << "\t";
  file_ << "Localization prob" << "\t";
  file_ << "Combinatorics" << "\t";
  file_ << "PIF" << "\t";
  file_ << "Fraction of total spectrum" << "\t";
  file_ << "Base peak fraction" << "\t";
  file_ << "Precursor full scan number" << "\t";
  file_ << "Precursor Intensity" << "\t";
  file_ << "Precursor apex fraction" << "\t";
  file_ << "Precursor apex offset" << "\t";
  file_ << "Precursor apex offset time" << "\t";
  file_ << "Matches Intensities" << "\t";
  file_ << "Mass deviations [Da]" << "\t";
  file_ << "Mass deviations [ppm]" << "\t";
  file_ << "Masses" << "\t";
  file_ << "Number of matches" << "\t";
  file_ << "Intensity coverage" << "\t"; // vielleicht aus intensity in evidence.txt berechenbar?
  file_ << "Peak coverage" << "\t";
  file_ << "Neutral loss level" << "\t";
  file_ << "ETD identification type" << "\t";
  file_ << "Reverse" << "\t"; // ok
  file_ << "All scores" << "\t";
  file_ << "All sequences" << "\t";
  file_ << "All modified sequences" << "\t";
  file_ << "Reporter PIF" << "\t";
  file_ << "Reporter fraction" << "\t";
  file_ << "id" << "\t"; // ok
  file_ << "Protein group IDs" << "\t"; // ok
  file_ << "Peptide ID" << "\t";
  file_ << "Mod. peptide ID" << "\t";
  file_ << "Evidence ID" << "\t";
  file_ << "Oxidation (M) site IDs" << "\n";
  
}

// folgendes funktioniert nur, wenn MQEvidenceExporter nicht private ist, 
// sonst müsste man die restlichen Funktionen vermutlich kopieren

void MQEvidence::exportRowFromFeature_(
        const Feature& f,
        const ConsensusMap& cmap,
        const Size c_feature_number,
        const String& raw_file,
        const std::multimap<String, std::pair<Size, Size>>& UIDs,
        const ProteinIdentification::Mapping& mp_f)
{

  const PeptideHit* ptr_best_hit; // the best hit referring to score
  const ConsensusFeature& cf = cmap[c_feature_number];
  Size pep_ids_size = 0;
  String type;
  if (MQEvidence::hasValidPepID_(f, c_feature_number, UIDs, mp_f))
  {
    for (Size i = 1; i < f.getPeptideIdentifications().size(); ++i) // for msms-count
    {
      if (!f.getPeptideIdentifications()[i].getHits().empty())
      {
        if (f.getPeptideIdentifications()[i].getHits()[0].getSequence() == f.getPeptideIdentifications()[0].getHits()[0].getSequence())
        {
          ++pep_ids_size;
        }
        else
          break;
      }
    }
    type = "MULTI-MSMS";
    ptr_best_hit = &f.getPeptideIdentifications()[0].getHits()[0];
  }
    else if (MQEvidence::hasPeptideIdentifications_(cf))
  {
    type = "MULTI-MATCH";
    ptr_best_hit = &cf.getPeptideIdentifications()[0].getHits()[0];
  }
  else
  {
    return; // no valid PepID; nothing to export
  }
  
  const double& max_score = ptr_best_hit->getScore(); // entfällt, falls score in MQEvidence falsch ist (also vermutlich)
  const AASequence& pep_seq = ptr_best_hit->getSequence();

  if (pep_seq.empty())
  {
    return;
  }

  std::map<String, Size> modifications;
  if (pep_seq.hasNTerminalModification())
  {
    const String& n_terminal_modification = pep_seq.getNTerminalModificationName();
    modifications.emplace(std::make_pair(n_terminal_modification, 1));
  }
  if (pep_seq.hasCTerminalModification())
  {
    modifications.emplace(std::make_pair(pep_seq.getCTerminalModificationName(), 1));
  }
  for (Size i = 0; i < pep_seq.size(); ++i)
  {
    if (pep_seq.getResidue(i).isModified())
    {
      ++modifications[pep_seq.getResidue(i).getModification()->getFullId()];
    }
  }

// what is written in the file in this exact order 

  file_ << raw_file << "\t"; // raw file
  file_ << "Scan number" << "\t";
  file_ << "Scan index" << "\t";
  file_ << pep_seq.toUnmodifiedString() << "\t"; // Sequence
  file_ << pep_seq.size() << "\t";               // Length
  file_ << ptr_best_hit->getMetaValue("missed_cleavages", "NA") << "\t"; // missed cleavages

  if (modifications.empty())
  {
    file_ << "Unmodified"
          << "\t";
  }
  else
  {
    for (const auto& m : modifications)
    {
      file_ << m.first << ";"; // Modification
    }
    file_ << "\t";
  }
  file_ << "_" << pep_seq << "_" << "\t"; // Modified Sequence
  
  file_ << "Oxidation (M) Probabilities" << "\t"; // Oxidation (M) Probabilities
  file_ << "Oxidation (M) Score diffs" << "\t"; // Oxidation (M) Score diffs
  
  if (pep_seq.hasNTerminalModification())
  {
  const String& n_terminal_modification = pep_seq.getNTerminalModificationName();
  n_terminal_modification.hasSubstring("Acetyl") ? file_ << 1 << "\t" : file_ << 0 << "\t"; // Acetyl (Protein N-term)
  }
  else
  {
    file_ << 0 << "\t"; // Acetyl (Protein N-term)
  }
  
  modifications.find("Oxidation (M)") == modifications.end() ? file_ << "0"
																     << "\t" :
															   file_ << modifications.find("Oxidation (M)")->second << "\t"; // Oxidation (M)
  
  const std::set<String>& accessions = ptr_best_hit->extractProteinAccessionsSet();
  for (const String& p : accessions)
  {
    file_ << p << ";"; // Proteins
  }
  file_ << "\t";
  
  file_ << f.getCharge() << "\t";           // Charge
  
  file_ << "Fragmentation" << "\t";
  file_ << "Mass analyzer" << "\t";
  file_ << type << "\t"; // type
  file_ << "Scan event number" << "\t";
  file_ << "Isotope index" << "\t";
  file_ << f.getMZ() << "\t";               // M/Z
  file_ << "Mass" << "\t"; // Mass
  file_ << "Mass error [ppm]" << "\t";
  file_ << "Mass error [Da]" << "\t";
  file_ << "Simple mass error [ppm]" << "\t";
  file_ << "Retention time" << "\t"; 
  file_ << "PEP" << "\t";
  file_ << "Score" << "\t"; 
  file_ << "Delta score" << "\t";
  file_ << "Score diff" << "\t";
  file_ << "Localization prob" << "\t";
  file_ << "Combinatorics" << "\t";
  file_ << "PIF" << "\t";
  file_ << "Fraction of total spectrum" << "\t";
  file_ << "Base peak fraction" << "\t";
  file_ << "Precursor full scan number" << "\t";
  file_ << "Precursor Intensity" << "\t";
  file_ << "Precursor apex fraction" << "\t";
  file_ << "Precursor apex offset" << "\t";
  file_ << "Precursor apex offset time" << "\t";
  file_ << "Matches Intensities" << "\t";
  file_ << "Mass deviations [Da]" << "\t";
  file_ << "Mass deviations [ppm]" << "\t";
  file_ << "Masses" << "\t";
  file_ << "Number of matches" << "\t";
  file_ << "Intensity coverage" << "\t";
  file_ << "Peak coverage" << "\t";
  file_ << "Neutral loss level" << "\t";
  file_ << "ETD identification type" << "\t";
  ptr_best_hit->getMetaValue("target_decoy") == "decoy" ? file_ << "1"
                                                                << "\t" :
                                                          file_ << "\t"; // reverse
                                                          
  file_ << "All scores" << "\t";
  file_ << "All sequences" << "\t";
  file_ << "All modified sequences" << "\t";
  file_ << "Reporter PIF" << "\t";
  file_ << "Reporter fraction" << "\t"; 
  
  file_ << id_ << "\t"; // ID
  ++id_;
  
  file_ << MQEvidence::proteinGroupID_(acessions(0)); // nicht sicher
  for (const String& p : accessions)
  {
    file_ << ";" << MQEvidence::proteinGroupID_(p+1); // Protein group ids
  }
  file_ << "\t";
  
  file_ << "Peptide ID" << "\t";
  file_ << "Mod. peptide ID" << "\t";
  file_ << "Evidence ID" << "\t";
  file_ << "Oxidation (M) site IDs" << "\n";

  
  


}










