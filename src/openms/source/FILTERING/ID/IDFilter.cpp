// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2013.
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
// $Authors: Nico Pfeifer, Mathias Walzer$
// --------------------------------------------------------------------------

#include <OpenMS/FILTERING/ID/IDFilter.h>
#include <OpenMS/CONCEPT/LogStream.h>

#include <cmath>
#include <climits>

using namespace std;

namespace OpenMS
{
  IDFilter::IDFilter()
  {
  }

  IDFilter::~IDFilter()
  {
  }

  void IDFilter::filterIdentificationsUnique(const PeptideIdentification& identification,
                                             PeptideIdentification& filtered_identification)
  {
    vector<PeptideHit> hits;
    filtered_identification = identification;
    vector<PeptideHit> temp_hits = identification.getHits();

    for (vector<PeptideHit>::iterator it = temp_hits.begin();
         it != temp_hits.end();
         ++it)
    {
      if (find(hits.begin(), hits.end(), *it) == hits.end())
      {
        hits.push_back(*it);
      }
    }
    filtered_identification.setHits(hits);
  }

  void IDFilter::filterIdentificationsByMzError(const PeptideIdentification& identification, double mass_error, bool unit_ppm, PeptideIdentification& filtered_identification)
  {
    vector<PeptideHit> hits;
    filtered_identification = identification;
    vector<PeptideHit> temp_hits = identification.getHits();

    for (vector<PeptideHit>::iterator it = temp_hits.begin(); it != temp_hits.end(); ++it)
    {
      Int charge = it->getCharge();

      if (charge == 0)
      {
        charge = 1;
      }

	  double exp_mz = identification.getMZ();
      double theo_mz =  (it->getSequence().getMonoWeight() + (double)charge * Constants::PROTON_MASS_U) / (double)charge;
      double error(exp_mz - theo_mz);

      if (unit_ppm)
      {
        error = error / theo_mz * (double)1e6;
      }

      if (fabs(error) <= mass_error)
      {
        hits.push_back(*it);
      }
    }
    filtered_identification.setHits(hits);
  }

  void IDFilter::filterIdentificationsByBestHits(const PeptideIdentification& identification,
                                                 PeptideIdentification& filtered_identification,
                                                 bool strict)
  {
    vector<PeptideHit> filtered_peptide_hits;
    PeptideHit temp_peptide_hit;
    vector<Size> new_peptide_indices;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    if (!identification.getHits().empty())
    {
      float optimal_value = identification.getHits()[0].getScore();
      new_peptide_indices.push_back(0);

      // searching for peptide(s) with maximal score
      for (Size i = 1; i < identification.getHits().size(); i++)
      {
        float temp_score = identification.getHits()[i].getScore();
        bool new_leader = false;
        if ((identification.isHigherScoreBetter() && (temp_score > optimal_value))
           || (!identification.isHigherScoreBetter() && (temp_score < optimal_value)))
          new_leader = true;

        if (new_leader)
        {
          optimal_value = temp_score;
          new_peptide_indices.clear();
          new_peptide_indices.push_back(i);
        }
        else if (temp_score == optimal_value)
        {
          new_peptide_indices.push_back(i);
        }
      }
      if (!strict || new_peptide_indices.size() == 1)
      {
        for (Size i = 0; i < new_peptide_indices.size(); i++)
        {
          filtered_peptide_hits.push_back(identification.getHits()[new_peptide_indices[i]]);
        }
      }
    }

    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByLength(const PeptideIdentification& identification,
                                               PeptideIdentification& filtered_identification,
                                               Size min_length,
                                               Size max_length)
  {
    vector<Size> new_peptide_indices;
    vector<PeptideHit> filtered_peptide_hits;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());
    Size ml = max_length;
    if (max_length < min_length)
    {
      ml = UINT_MAX;
    }

    const vector<PeptideHit>& temp_peptide_hits = identification.getHits();

    for (Size i = 0; i < temp_peptide_hits.size(); i++)
    {
      if (temp_peptide_hits[i].getSequence().size() >= min_length && temp_peptide_hits[i].getSequence().size() <= ml)
      {
        new_peptide_indices.push_back(i);
      }
    }

    for (Size i = 0; i < new_peptide_indices.size(); i++)
    {
      filtered_peptide_hits.push_back(identification.getHits()[new_peptide_indices[i]]);
    }
    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByCharge(const PeptideIdentification& identification,
                                               Int min_charge,
                                               PeptideIdentification& filtered_identification)
  {
    vector<Size> new_peptide_indices;
    vector<PeptideHit> filtered_peptide_hits;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    const vector<PeptideHit>& temp_peptide_hits = identification.getHits();

    for (Size i = 0; i < temp_peptide_hits.size(); i++)
    {
      if (temp_peptide_hits[i].getCharge() >= min_charge)
      {
        new_peptide_indices.push_back(i);
      }
    }

    for (Size i = 0; i < new_peptide_indices.size(); i++)
    {
      filtered_peptide_hits.push_back(identification.getHits()[new_peptide_indices[i]]);
    }
    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByVariableModifications(const PeptideIdentification& identification,
                                                              const vector<String>& fixed_modifications,
                                                              PeptideIdentification& filtered_identification)
  {
    vector<Size> new_peptide_indices;
    vector<PeptideHit> filtered_peptide_hits;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    const vector<PeptideHit>& temp_peptide_hits = identification.getHits();

    for (Size i = 0; i < temp_peptide_hits.size(); i++)
    {
      const AASequence& aa_seq = temp_peptide_hits[i].getSequence();

      /*
       TODO: check these cases
      // check terminal modifications
      if (aa_seq.hasNTerminalModification())
      {
        String unimod_name = aa_seq.getNTerminalModification();
        if (find(fixed_modifications.begin(), fixed_modifications.end(), unimod_name) == fixed_modifications.end())
        {
          new_peptide_indices.push_back(i);
          continue;
        }
      }

      if (aa_seq.hasCTerminalModification())
      {
        String unimod_name = aa_seq.getCTerminalModification();
        if (find(fixed_modifications.begin(), fixed_modifications.end(), unimod_name) == fixed_modifications.end())
        {
          new_peptide_indices.push_back(i);
          continue;
        }
      }
      */
      // check internal modifications
      for (Size j = 0; j != aa_seq.size(); ++j)
      {
        if (aa_seq[j].isModified())
        {
          String unimod_name = aa_seq[j].getModification() + " (" + aa_seq[j].getOneLetterCode() + ")";
          if (find(fixed_modifications.begin(), fixed_modifications.end(), unimod_name) == fixed_modifications.end())
          {
            new_peptide_indices.push_back(i);
            continue;
          }
        }
      }
    }

    for (Size i = 0; i < new_peptide_indices.size(); i++)
    {
      const PeptideHit& ph = temp_peptide_hits[new_peptide_indices[i]];
      filtered_peptide_hits.push_back(ph);
    }
    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByProteins(const PeptideIdentification& identification,
                                                 const vector<FASTAFile::FASTAEntry>& proteins,
                                                 PeptideIdentification& filtered_identification,
                                                 bool no_protein_identifiers)
  {
    // TODO: this is highly inefficient! the Protein-Index should be build once for all peptide-identifications instead of
    //       doing this once for every ID. Furthermore the index itself is inefficient (use seqan instead)
    String protein_sequences;
    String accession_sequences;
    vector<PeptideHit> filtered_peptide_hits;
    PeptideHit temp_peptide_hit;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    for (Size i = 0; i < proteins.size(); i++)
    {
      if (proteins[i].identifier != "")
      {
        accession_sequences.append("*" + proteins[i].identifier);
      }
      if (proteins[i].sequence != "")
      {
        protein_sequences.append("*" + proteins[i].sequence);
      }
    }
    accession_sequences.append("*");
    protein_sequences.append("*");

    for (Size i = 0; i < identification.getHits().size(); i++)
    {
      if (no_protein_identifiers || accession_sequences == "*") // filter by sequence alone if no protein accesssions are available
      {
        if (protein_sequences.find(identification.getHits()[i].getSequence().toUnmodifiedString()) != String::npos)
        {
          filtered_peptide_hits.push_back(identification.getHits()[i]);
        }
      }
      else // filter by protein accessions
      {
        for (vector<String>::const_iterator ac_it = identification.getHits()[i].getProteinAccessions().begin();
             ac_it != identification.getHits()[i].getProteinAccessions().end();
             ++ac_it)
        {
          if (accession_sequences.find("*" + *ac_it) != String::npos)
          {
            filtered_peptide_hits.push_back(identification.getHits()[i]);
            break; // we found a matching protein, the peptide is valid -> exit
          }
        }
      }
    }

    filtered_identification.setHits(filtered_peptide_hits);
    filtered_identification.assignRanks();
  }

  void IDFilter::filterIdentificationsByProteins(const ProteinIdentification& identification,
                                                 const vector<FASTAFile::FASTAEntry>& proteins,
                                                 ProteinIdentification& filtered_identification)
  {
    String protein_sequences;
    String accession_sequences;
    vector<ProteinHit> filtered_protein_hits;
    ProteinHit temp_protein_hit;

    filtered_identification = identification;
    filtered_identification.setHits(vector<ProteinHit>());

    for (Size i = 0; i < proteins.size(); i++)
    {
      accession_sequences.append("*" + proteins[i].identifier);
    }
    accession_sequences.append("*");

    for (Size i = 0; i < identification.getHits().size(); i++)
    {
      if (accession_sequences.find("*" + identification.getHits()[i].getAccession()) != String::npos)
      {
        filtered_protein_hits.push_back(identification.getHits()[i]);
      }
    }

    filtered_identification.setHits(filtered_protein_hits);
    filtered_identification.assignRanks();
  }

  void IDFilter::filterIdentificationsByExclusionPeptides(const PeptideIdentification& identification,
                                                          const set<String>& peptides,
                                                          PeptideIdentification& filtered_identification)
  {
    String protein_sequences;
    String accession_sequences;
    vector<PeptideHit> filtered_peptide_hits;
    PeptideHit temp_peptide_hit;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    for (Size i = 0; i < identification.getHits().size(); i++)
    {
      if (find(peptides.begin(), peptides.end(), identification.getHits()[i].getSequence().toString()) == peptides.end())
      {
        filtered_peptide_hits.push_back(identification.getHits()[i]);
      }
    }
    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByRTFirstDimPValues(const PeptideIdentification& identification,
                                                          PeptideIdentification& filtered_identification,
                                                          double p_value)
  {
    double border = 1 - p_value;
    vector<PeptideHit> filtered_peptide_hits;
    PeptideHit temp_peptide_hit;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    Size missing_meta_value = 0;

    for (Size i = 0; i < identification.getHits().size(); ++i)
    {
      if (identification.getHits()[i].metaValueExists("predicted_RT_p_value_first_dim"))
      {
        if ((double)(identification.getHits()[i].getMetaValue("predicted_RT_p_value_first_dim")) <= border)
        {
          filtered_peptide_hits.push_back(identification.getHits()[i]);
        }
      }
      else
        ++missing_meta_value;
    }
    if (missing_meta_value > 0)
      LOG_WARN << "Filtering identifications by p-value did not work on " << missing_meta_value << " of " << identification.getHits().size() << " hits. Your data is missing a meta-value ('predicted_RT_p_value_first_dim') from RTPredict!\n";

    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::filterIdentificationsByRTPValues(const PeptideIdentification& identification,
                                                  PeptideIdentification& filtered_identification,
                                                  double p_value)
  {
    double border = 1 - p_value;
    vector<PeptideHit> filtered_peptide_hits;
    PeptideHit temp_peptide_hit;

    filtered_identification = identification;
    filtered_identification.setHits(vector<PeptideHit>());

    Size missing_meta_value = 0;

    for (Size i = 0; i < identification.getHits().size(); i++)
    {
      if (identification.getHits()[i].metaValueExists("predicted_RT_p_value"))
      {
        if ((double)(identification.getHits()[i].getMetaValue("predicted_RT_p_value")) <= border)
        {
          filtered_peptide_hits.push_back(identification.getHits()[i]);
        }
      }
      else
        ++missing_meta_value;
    }
    if (missing_meta_value > 0)
      LOG_WARN << "Filtering identifications by p-value did not work on " << missing_meta_value << " of " << identification.getHits().size() << " hits. Your data is missing a meta-value ('predicted_RT_p_value') from RTPredict!\n";

    if (!filtered_peptide_hits.empty())
    {
      filtered_identification.setHits(filtered_peptide_hits);
      filtered_identification.assignRanks();
    }
  }

  void IDFilter::removeUnreferencedProteinHits(const ProteinIdentification& identification, const vector<PeptideIdentification> peptide_identifications, ProteinIdentification& filtered_identification)
  {
    const String& run_identifier = identification.getIdentifier();

    // build set of protein accessions that are referenced by peptides
    set<String> proteinaccessions_with_peptides;
    for (Size i = 0; i != peptide_identifications.size(); ++i)
    {
      // run id of protein and peptide identification must match
      if (run_identifier == peptide_identifications[i].getIdentifier())
      {
        const vector<PeptideHit>& tmp_pep_hits = peptide_identifications[i].getHits();
        // extract protein accessions of each peptide hit
        for (Size j = 0; j != tmp_pep_hits.size(); ++j)
        {
          const std::vector<String>& protein_accessions = tmp_pep_hits[j].getProteinAccessions();
          for (Size k = 0; k != protein_accessions.size(); ++k)
          {
            String key = protein_accessions[k];
            proteinaccessions_with_peptides.insert(key);
          }
        }
      }
    }

    // add all protein hits referenced by a peptide
    const vector<ProteinHit>& temp_protein_hits = identification.getHits();
    vector<ProteinHit> filtered_protein_hits;
    for (Size j = 0; j != temp_protein_hits.size(); ++j)
    {
      const String& protein_accession = temp_protein_hits[j].getAccession();
      if (proteinaccessions_with_peptides.find(protein_accession) != proteinaccessions_with_peptides.end())
      {
        filtered_protein_hits.push_back(temp_protein_hits[j]);
      }
    }

    // copy identification
    filtered_identification = identification;

    // assign filtered hits to protein identification
    filtered_identification.setHits(filtered_protein_hits);
  }

  void IDFilter::removeUnreferencedPeptideHits(const ProteinIdentification& identification, 
                                               std::vector<PeptideIdentification>& peptide_identifications,
                                               bool delete_unreferenced_peptide_hits /* = false */)
  {
    const String& run_identifier = identification.getIdentifier();

    // build set of protein accessions
    set<String> all_prots;
    const vector<ProteinHit>& temp_protein_hits = identification.getHits();
    for (Size j = 0; j != temp_protein_hits.size(); ++j)
    {
      all_prots.insert(temp_protein_hits[j].getAccession());
    }

    std::vector<PeptideIdentification> filtered_peptide_identifications;
    // remove peptides which are not referenced
    for (Size i = 0; i != peptide_identifications.size(); ++i)
    {
      // run id of protein and peptide identification must match
      if (run_identifier == peptide_identifications[i].getIdentifier())
      {
        const vector<PeptideHit>& tmp_pep_hits = peptide_identifications[i].getHits();
        vector<PeptideHit> filtered_pep_hits;
        // check protein accessions of each peptide hit
        for (Size j = 0; j != tmp_pep_hits.size(); ++j)
        {
          const std::vector<String>& acc = tmp_pep_hits[j].getProteinAccessions();
          std::vector<String> valid_prots;
          for (Size k = 0; k != acc.size(); ++k)
          { // find valid proteins
            if (all_prots.find(acc[k]) != all_prots.end())
            {
              valid_prots.push_back(acc[k]);
            }
          }
          if (valid_prots.size() > 0 || !delete_unreferenced_peptide_hits)
          { // if present, copy the hit
            filtered_pep_hits.push_back(tmp_pep_hits[j]);
            filtered_pep_hits.back().setProteinAccessions(valid_prots);
          }
        }
        // if the peptide has hits, we use it
        if (filtered_pep_hits.size() > 0)
        {
          filtered_peptide_identifications.push_back(peptide_identifications[i]);
          filtered_peptide_identifications.back().setHits(filtered_pep_hits);
        }
      }
    }

    // exchange with new hits
    filtered_peptide_identifications.swap(peptide_identifications);
  }


  bool IDFilter::filterIdentificationsByMetaValueRange(const PeptideIdentification& identification, const String& key, double low, double high, bool missing)
  {
    if (!identification.metaValueExists(key)) return missing;

    double value = identification.getMetaValue(key);
    return (value >= low) && (value <= high);
  }

  void IDFilter::filterIdentificationsByRT(const std::vector<PeptideIdentification>& identifications, double min_rt, double max_rt, std::vector<PeptideIdentification>& filtered_identifications)
  {
	  filtered_identifications.clear();

	  for (Size i = 0; i < identifications.size(); ++i)
	  {
		  if (identifications[i].getRT() >= min_rt && identifications[i].getRT() <= max_rt)
		  {
			  filtered_identifications.push_back(identifications[i]);
		  }
	  }
  }



  
  void IDFilter::filterIdentificationsByMZ(const std::vector<PeptideIdentification>& identifications, double min_mz, double max_mz, std::vector<PeptideIdentification>& filtered_identifications)
  {
	  filtered_identifications.clear();

	  for (Size i = 0; i < identifications.size(); ++i)
	  {
		  if (identifications[i].getMZ() >= min_mz && identifications[i].getMZ() <= max_mz)
		  {
			  filtered_identifications.push_back(identifications[i]);
		  }
	  }
  }

} // namespace OpenMS
