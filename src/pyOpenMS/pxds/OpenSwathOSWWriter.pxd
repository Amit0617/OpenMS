from Types cimport *
from String cimport *
from LightTargetedExperiment cimport *
from FeatureMap cimport *

cdef extern from "<OpenMS/ANALYSIS/OPENSWATH/OpenSwathOSWWriter.h>" namespace "OpenMS":
    
    cdef cppclass OpenSwathOSWWriter "OpenMS::OpenSwathOSWWriter":

        OpenSwathOSWWriter(String output_filename, UInt64 run_id, String input_filename, bool ms1_scores, bool sonar, bool uis_scores) nogil except +
        OpenSwathOSWWriter(OpenSwathOSWWriter &) nogil except + # compiler

        bool isActive() nogil except +
        void writeHeader() nogil except + # wrap-doc:Initializes file by generating SQLite tables
        String prepareLine(LightCompound & compound, LightTransition * tr, FeatureMap & output, String id_) nogil except +
            # wrap-doc:
                #   Prepare a single line (feature) for output\n
                #   
                #   The result can be flushed to disk using writeLines (either line by line or after collecting several lines)
                #   
                #   -----
                #   :param pep: The compound (peptide/metabolite) used for extraction
                #   :param transition: The transition used for extraction 
                #   :param output: The feature map containing all features (each feature will generate one entry in the output)
                #   :param id: The transition group identifier (peptide/metabolite id)
                #   :return: A String to be written using writeLines

        void writeLines(libcpp_vector[ String ] to_osw_output) nogil except +
            # wrap-doc:
                #   Write data to disk\n
                #   
                #   Takes a set of pre-prepared data statements from prepareLine and flushes them to disk
                #   
                #   -----
                #   :param to_osw_output: Statements generated by prepareLine

