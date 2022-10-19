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
// $Authors: Stephan Aiche$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/ANALYSIS/QUANTITATION/TMTEighteenPlexQuantitationMethod.h>
///////////////////////////

#include <OpenMS/DATASTRUCTURES/Matrix.h>

using namespace OpenMS;
using namespace std;

START_TEST(TMTEighteenPlexQuantitationMethod, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

TMTEighteenPlexQuantitationMethod* ptr = nullptr;
TMTEighteenPlexQuantitationMethod* null_ptr = nullptr;
START_SECTION(TMTEighteenPlexQuantitationMethod())
{
  ptr = new TMTEighteenPlexQuantitationMethod();
	TEST_NOT_EQUAL(ptr, null_ptr)
}
END_SECTION

START_SECTION(~TMTEighteenPlexQuantitationMethod())
{
	delete ptr;
}
END_SECTION

START_SECTION((const String& getMethodName() const ))
{
  TMTEighteenPlexQuantitationMethod quant_meth;
  TEST_EQUAL(quant_meth.getMethodName(), "tmt18plex")
}
END_SECTION

START_SECTION((const IsobaricChannelList& getChannelInformation() const ))
{
  TMTEighteenPlexQuantitationMethod quant_meth;
  IsobaricQuantitationMethod::IsobaricChannelList channel_list = quant_meth.getChannelInformation();

  TEST_EQUAL(channel_list.size(), 18)
  ABORT_IF(channel_list.size() != 18)

  // descriptions are empty by default
  TEST_STRING_EQUAL(channel_list[0].description, "")
  TEST_STRING_EQUAL(channel_list[1].description, "")
  TEST_STRING_EQUAL(channel_list[2].description, "")
  TEST_STRING_EQUAL(channel_list[3].description, "")
  TEST_STRING_EQUAL(channel_list[4].description, "")
  TEST_STRING_EQUAL(channel_list[5].description, "")
  TEST_STRING_EQUAL(channel_list[6].description, "")
  TEST_STRING_EQUAL(channel_list[7].description, "")
  TEST_STRING_EQUAL(channel_list[8].description, "")
  TEST_STRING_EQUAL(channel_list[9].description, "")
  TEST_STRING_EQUAL(channel_list[10].description, "")
  TEST_STRING_EQUAL(channel_list[11].description, "")
  TEST_STRING_EQUAL(channel_list[12].description, "")
  TEST_STRING_EQUAL(channel_list[13].description, "")
  TEST_STRING_EQUAL(channel_list[14].description, "")
  TEST_STRING_EQUAL(channel_list[15].description, "")
  TEST_STRING_EQUAL(channel_list[16].description, "")
  TEST_STRING_EQUAL(channel_list[17].description, "")

  // check masses&co
  TEST_EQUAL(channel_list[0].name, "126")
  TEST_EQUAL(channel_list[0].id, 0)
  TEST_EQUAL(channel_list[0].center, 126.127726)
  TEST_EQUAL(channel_list[0].affected_channels[0], -1)
  TEST_EQUAL(channel_list[0].affected_channels[1], -1)
  TEST_EQUAL(channel_list[0].affected_channels[2], 2)
  TEST_EQUAL(channel_list[0].affected_channels[3], -1)

  TEST_EQUAL(channel_list[1].name, "127N")
  TEST_EQUAL(channel_list[1].id, 1)
  TEST_EQUAL(channel_list[1].center, 127.124761)
  TEST_EQUAL(channel_list[1].affected_channels[0], -1)
  TEST_EQUAL(channel_list[1].affected_channels[1], -1)
  TEST_EQUAL(channel_list[1].affected_channels[2], 3)
  TEST_EQUAL(channel_list[1].affected_channels[3], -1)

  TEST_EQUAL(channel_list[2].name, "127C")
  TEST_EQUAL(channel_list[2].id, 2)
  TEST_EQUAL(channel_list[2].center, 127.131081)
  TEST_EQUAL(channel_list[2].affected_channels[0], -1)
  TEST_EQUAL(channel_list[2].affected_channels[1], 0)
  TEST_EQUAL(channel_list[2].affected_channels[2], 4)
  TEST_EQUAL(channel_list[2].affected_channels[3], -1)

  TEST_EQUAL(channel_list[3].name, "128N")
  TEST_EQUAL(channel_list[3].id, 3)
  TEST_EQUAL(channel_list[3].center, 128.128116)
  TEST_EQUAL(channel_list[3].affected_channels[0], -1)
  TEST_EQUAL(channel_list[3].affected_channels[1], 1)
  TEST_EQUAL(channel_list[3].affected_channels[2], 5)
  TEST_EQUAL(channel_list[3].affected_channels[3], -1)

  TEST_EQUAL(channel_list[4].name, "128C")
  TEST_EQUAL(channel_list[4].id, 4)
  TEST_EQUAL(channel_list[4].center, 128.134436)
  TEST_EQUAL(channel_list[4].affected_channels[0], -1)
  TEST_EQUAL(channel_list[4].affected_channels[1], 2)
  TEST_EQUAL(channel_list[4].affected_channels[2], 6)
  TEST_EQUAL(channel_list[4].affected_channels[3], -1)

  TEST_EQUAL(channel_list[5].name, "129N")
  TEST_EQUAL(channel_list[5].id, 5)
  TEST_EQUAL(channel_list[5].center, 129.131471)
  TEST_EQUAL(channel_list[5].affected_channels[0], -1)
  TEST_EQUAL(channel_list[5].affected_channels[1], 3)
  TEST_EQUAL(channel_list[5].affected_channels[2], 7)
  TEST_EQUAL(channel_list[5].affected_channels[3], -1)

  TEST_EQUAL(channel_list[6].name, "129C")
  TEST_EQUAL(channel_list[6].id, 6)
  TEST_EQUAL(channel_list[6].center, 129.137790)
  TEST_EQUAL(channel_list[6].affected_channels[0], -1)
  TEST_EQUAL(channel_list[6].affected_channels[1], 4)
  TEST_EQUAL(channel_list[6].affected_channels[2], 8)
  TEST_EQUAL(channel_list[6].affected_channels[3], -1)

  TEST_EQUAL(channel_list[7].name, "130N")
  TEST_EQUAL(channel_list[7].id, 7)
  TEST_EQUAL(channel_list[7].center, 130.134825)
  TEST_EQUAL(channel_list[7].affected_channels[0], -1)
  TEST_EQUAL(channel_list[7].affected_channels[1], 5)
  TEST_EQUAL(channel_list[7].affected_channels[2], 9)
  TEST_EQUAL(channel_list[7].affected_channels[3], -1)

  TEST_EQUAL(channel_list[8].name, "130C")
  TEST_EQUAL(channel_list[8].id, 8)
  TEST_EQUAL(channel_list[8].center, 130.141145)
  TEST_EQUAL(channel_list[8].affected_channels[0], -1)
  TEST_EQUAL(channel_list[8].affected_channels[1], 6)
  TEST_EQUAL(channel_list[8].affected_channels[2], 10)
  TEST_EQUAL(channel_list[8].affected_channels[3], -1)

  TEST_EQUAL(channel_list[9].name, "131N")
  TEST_EQUAL(channel_list[9].id, 9)
  TEST_EQUAL(channel_list[9].center, 131.138180)
  TEST_EQUAL(channel_list[9].affected_channels[0], -1)
  TEST_EQUAL(channel_list[9].affected_channels[1], 7)
  TEST_EQUAL(channel_list[9].affected_channels[2], 11)
  TEST_EQUAL(channel_list[9].affected_channels[3], -1)

  TEST_EQUAL(channel_list[10].name, "131C")
  TEST_EQUAL(channel_list[10].id, 10)
  TEST_EQUAL(channel_list[10].center, 131.144500)
  TEST_EQUAL(channel_list[10].affected_channels[0], -1)
  TEST_EQUAL(channel_list[10].affected_channels[1], 8)
  TEST_EQUAL(channel_list[10].affected_channels[2], 12)
  TEST_EQUAL(channel_list[10].affected_channels[3], -1)

  TEST_EQUAL(channel_list[11].name, "132N")
  TEST_EQUAL(channel_list[11].id, 11)
  TEST_EQUAL(channel_list[11].center, 132.141535)
  TEST_EQUAL(channel_list[11].affected_channels[0], -1)
  TEST_EQUAL(channel_list[11].affected_channels[1], 9)
  TEST_EQUAL(channel_list[11].affected_channels[2], 13)
  TEST_EQUAL(channel_list[11].affected_channels[3], -1)

  TEST_EQUAL(channel_list[12].name, "132C")
  TEST_EQUAL(channel_list[12].id, 12)
  TEST_EQUAL(channel_list[12].center, 132.147855)
  TEST_EQUAL(channel_list[12].affected_channels[0], -1)
  TEST_EQUAL(channel_list[12].affected_channels[1], 10)
  TEST_EQUAL(channel_list[12].affected_channels[2], 14)
  TEST_EQUAL(channel_list[12].affected_channels[3], -1)

  TEST_EQUAL(channel_list[13].name, "133N")
  TEST_EQUAL(channel_list[13].id, 13)
  TEST_EQUAL(channel_list[13].center, 133.144890)
  TEST_EQUAL(channel_list[13].affected_channels[0], -1)
  TEST_EQUAL(channel_list[13].affected_channels[1], 11)
  TEST_EQUAL(channel_list[13].affected_channels[2], 15)
  TEST_EQUAL(channel_list[13].affected_channels[3], -1)

  TEST_EQUAL(channel_list[14].name, "133C")
  TEST_EQUAL(channel_list[14].id, 14)
  TEST_EQUAL(channel_list[14].center, 133.151210)
  TEST_EQUAL(channel_list[14].affected_channels[0], -1)
  TEST_EQUAL(channel_list[14].affected_channels[1], 12)
  TEST_EQUAL(channel_list[14].affected_channels[2], -1)
  TEST_EQUAL(channel_list[14].affected_channels[3], -1)

  TEST_EQUAL(channel_list[15].name, "134N")
  TEST_EQUAL(channel_list[15].id, 15)
  TEST_EQUAL(channel_list[15].center, 134.148245)
  TEST_EQUAL(channel_list[15].affected_channels[0], -1)
  TEST_EQUAL(channel_list[15].affected_channels[1], 13)
  TEST_EQUAL(channel_list[15].affected_channels[2], -1)
  TEST_EQUAL(channel_list[15].affected_channels[3], -1)

  TEST_EQUAL(channel_list[16].name, "134C")
  TEST_EQUAL(channel_list[16].id, 16)
  TEST_EQUAL(channel_list[16].center, 134.154565)
  TEST_EQUAL(channel_list[16].affected_channels[0], -1)
  TEST_EQUAL(channel_list[16].affected_channels[1], 14)
  TEST_EQUAL(channel_list[16].affected_channels[2], -1)
  TEST_EQUAL(channel_list[16].affected_channels[3], -1)

  TEST_EQUAL(channel_list[17].name, "135N")
  TEST_EQUAL(channel_list[17].id, 17)
  TEST_EQUAL(channel_list[17].center, 135.151600)
  TEST_EQUAL(channel_list[17].affected_channels[0], -1)
  TEST_EQUAL(channel_list[17].affected_channels[1], 15)
  TEST_EQUAL(channel_list[17].affected_channels[2], -1)
  TEST_EQUAL(channel_list[17].affected_channels[3], -1)
}
END_SECTION

START_SECTION((Size getNumberOfChannels() const ))
{
  TMTEighteenPlexQuantitationMethod quant_meth;
  TEST_EQUAL(quant_meth.getNumberOfChannels(), 18)
}
END_SECTION

START_SECTION((virtual Matrix<double> getIsotopeCorrectionMatrix() const ))
{
  double test_matrix[18][18] = {{0.9198,0,0.0071,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                {0,0.9186,0,0.0188,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                {0.0802,0,0.9235,0,0.0134,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                {0,0.0746,0,0.9145,0,0.0241,0,0,0,0,0,0,0,0,0,0,0,0},
                                {0,0,0.0694,0,0.9307,0,0.0234,0,0,0,0,0,0,0,0,0,0,0},
                                {0,0,0,0.0667,0,0.9211,0,0.0353,0,0,0,0,0,0,0,0,0,0},
                                {0,0,0,0,0.0559,0,0.9247,0,0.0267,0,0,0,0,0,0,0,0,0},
                                {0,0,0,0,0,0.0548,0,0.919,0,0.0392,0,0,0,0,0,0,0,0},
                                {0,0,0,0,0,0,0.0519,0,0.9317,0,0.0369,0,0,0,0,0,0,0},
                                {0,0,0,0,0,0,0,0.0457,0,0.9235,0,0.0322,0,0,0,0,0,0},
                                {0,0,0,0,0,0,0,0,0.0416,0,0.9317,0,0.0411,0,0,0,0,0},
                                {0,0,0,0,0,0,0,0,0,0.0373,0,0.9402,0,0.0385,0,0,0,0},
                                {0,0,0,0,0,0,0,0,0,0,0.0314,0,0.9389,0,0.0463,0,0,0},
                                {0,0,0,0,0,0,0,0,0,0,0,0.0276,0,0.9457,0,0.0522,0,0},
                                {0,0,0,0,0,0,0,0,0,0,0,0,0.02,0,0.9419,0,0.0581,0},
                                {0,0,0,0,0,0,0,0,0,0,0,0,0,0.0158,0,0.9392,0,0.0542},
                                {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.9388,0},
                                {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.9458}};

  Matrix<double> test_Matrix;
	test_Matrix.setMatrix<18,18>(test_matrix);

  TMTEighteenPlexQuantitationMethod quant_meth;

  // we only check the default matrix here which is an identity matrix
  // for tmt18plex
  Matrix<double> m = quant_meth.getIsotopeCorrectionMatrix();

  TEST_EQUAL(m.rows(), 18)
  TEST_EQUAL(m.cols(), 18)

  ABORT_IF(m.rows() != 18)
  ABORT_IF(m.cols() != 18)

  for(Matrix<double>::SizeType i = 0; i < m.rows(); ++i)
  {
    for(Matrix<double>::SizeType j = 0; j < m.cols(); ++j)
    {
      if (i == j) TEST_REAL_SIMILAR(m(i,j), test_Matrix(i,j))
      else TEST_REAL_SIMILAR(m(i,j), test_Matrix(i,j))
    }
  }
}
END_SECTION

START_SECTION((Size getReferenceChannel() const ))
{
  TMTEighteenPlexQuantitationMethod quant_meth;
  TEST_EQUAL(quant_meth.getReferenceChannel(), 0)

  Param p;
  p.setValue("reference_channel","128N");
  quant_meth.setParameters(p);

  TEST_EQUAL(quant_meth.getReferenceChannel(), 3)
}
END_SECTION

START_SECTION((TMTEighteenPlexQuantitationMethod(const TMTEighteenPlexQuantitationMethod &other)))
{
  TMTEighteenPlexQuantitationMethod qm;
  Param p = qm.getParameters();
  p.setValue("channel_127N_description", "new_description");
  p.setValue("reference_channel", "129C");
  qm.setParameters(p);

  TMTEighteenPlexQuantitationMethod qm2(qm);
  IsobaricQuantitationMethod::IsobaricChannelList channel_list = qm2.getChannelInformation();
  TEST_STRING_EQUAL(channel_list[1].description, "new_description")
  TEST_EQUAL(qm2.getReferenceChannel(), 6)
}
END_SECTION

START_SECTION((TMTEighteenPlexQuantitationMethod& operator=(const TMTEighteenPlexQuantitationMethod &rhs)))
{
  TMTEighteenPlexQuantitationMethod qm;
  Param p = qm.getParameters();
  p.setValue("channel_127N_description", "new_description");
  p.setValue("reference_channel", "130C");
  qm.setParameters(p);

  TMTEighteenPlexQuantitationMethod qm2 = qm;
  IsobaricQuantitationMethod::IsobaricChannelList channel_list = qm2.getChannelInformation();
  TEST_STRING_EQUAL(channel_list[1].description, "new_description")
  TEST_EQUAL(qm2.getReferenceChannel(), 8)
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
