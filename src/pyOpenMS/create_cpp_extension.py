# input-encoding: latin-1
from __future__ import print_function


# use autowrap to generate Cython and .cpp file for wrapping OpenMS:
import autowrap.Main
import autowrap.CodeGenerator
import autowrap.DeclResolver
import glob
import pickle
import os.path
import os
import shutil
import sys

# windows ?
iswin = sys.platform == "win32"

# make sure we only log errors and not info/debug ...
import logging
import autowrap
# from logging import CRITICAL, ERROR, WARNING, INFO, DEBUG
# INFO = 20, WARNING=30, autowrap progress reports = 25
logging.getLogger("autowrap").setLevel(logging.INFO+1)

def doCythonCodeGeneration(modname, allDecl_mapping, instance_map, converters):
    m_filename = "pyopenms/%s.pyx" % modname
    cimports, manual_code = autowrap.Main.collect_manual_code(allDecl_mapping[modname]["addons"])
    autowrap.Main.register_converters(converters)
    autowrap_include_dirs = autowrap.generate_code(allDecl_mapping[modname]["decls"], instance_map,
                                                        target=m_filename, debug=False, manual_code=manual_code,
                                                        extra_cimports=cimports,
                                                        include_boost=False, include_numpy=True, all_decl=allDecl_mapping, add_relative=True)
    allDecl_mapping[modname]["inc_dirs"] = autowrap_include_dirs
    return autowrap_include_dirs

def doCythonCompile(arg):
    """
    Perform the Cython compilation step for each module
    """

    modname, autowrap_include_dirs = arg
    m_filename = "pyopenms/%s.pyx" % modname
    print ("Cython compile", m_filename)
    # By omitting "compiler_directives": {"language_level": 3} as extra_opt, autowrap will choose the language_level of the used python executable
    autowrap.Main.run_cython(inc_dirs=autowrap_include_dirs, extra_opts={}, out=m_filename, warn_level=2)

    if False:
        #
        # Fix two bugs in the cpp code generated by Cython to allow error-free
        # compilation (see OpenMS issues on github #527 and #745).
        #
        import re
        f = open(m_filename)
        fout = open(m_filename + "tmp", "w")
        expr_fix = re.compile(r"(.*).std::vector<(.*)>::iterator::~iterator\(\)")
        for line in f:
            # Fix for Issue #527
            res = expr_fix.sub('typedef std::vector<\\2>::iterator _it;\n\\1.~_it()', line)
            # Fix for Issue #745
            res = res.replace("__Pyx_PyUnicode_FromString(char", "__Pyx_PyUnicode_FromString(const char")
            fout.write(res)

        fout.close()
        f.close()
        shutil.copy(m_filename + "tmp", m_filename)
        os.remove(m_filename + "tmp")

if __name__ == '__main__':

  # import config
  from env import (QT_QMAKE_VERSION_INFO, OPEN_MS_BUILD_TYPE, PYOPENMS_SRC_DIR,
                   OPEN_MS_CONTRIB_BUILD_DIRS, OPEN_MS_LIB, OPEN_SWATH_ALGO_LIB,
                   OPEN_MS_BUILD_DIR, MSVS_RTLIBS, OPEN_MS_VERSION,
                   Boost_MAJOR_VERSION, Boost_MINOR_VERSION, PY_NUM_THREADS, PY_NUM_MODULES)

  IS_DEBUG = OPEN_MS_BUILD_TYPE.upper() == "DEBUG"

  print("Build type is: ", OPEN_MS_BUILD_TYPE)
  print("Number of submodules: ", PY_NUM_MODULES)
  print("Number of concurrent threads: ", PY_NUM_THREADS)

  if iswin and IS_DEBUG:
      raise Exception("building pyopenms on windows in debug mode not tested yet.")


  classdocu_base = "http://www.openms.de/current_doxygen/html/"
  autowrap.CodeGenerator.special_class_doc = "\n    Original C++ documentation is available `here <" + classdocu_base + "class%(namespace)s_1_1%(cpp_name)s.html>`_\n"
  autowrap.DeclResolver.default_namespace = "OpenMS"

  def chunkIt(seq, num):
      avg = len(seq) / float(num)
      out = []
      last = 0.0
      while len(out) < num:
        out.append(seq[int(last):int(last + avg)])
        last += avg

      # append the rest to the last element (if there is any)
      out[-1].extend( seq[int(last):] )
      return out

  j = os.path.join

  pxd_files = glob.glob(PYOPENMS_SRC_DIR + "/pxds/*.pxd")
  addons = glob.glob(PYOPENMS_SRC_DIR + "/addons/*.pyx")
  converters = [j(PYOPENMS_SRC_DIR, "converters")]

  persisted_data_path = "include_dir.bin"

  extra_cimports = []

  # We need to parse them all together but keep the association about which class
  # we found in which file (as they often need to be analyzed together)
  # TODO think about having a separate NUM_THREADS argument for parsing/cythonizing, since it is less
  #  memory intensive than the actualy compilation into a module (done in setup.py).
  # Hide annoying redeclaration errors from unscoped enums by using warning level 2.
  # This might lead to a minimal amount of unseen errors, but with all the mess, we would not have spotted them anyway.
  # This can be removed as soon as autowrap supports Cython 3 (intodruced scoped enum support) and OpenMS scopes all enums (e.g. with enum class).
  decls, instance_map = autowrap.parse(pxd_files, ".", num_processes=int(PY_NUM_THREADS), cython_warn_level=2)

  # Perform mapping
  pxd_decl_mapping = {}
  for de in decls:
      tmp = pxd_decl_mapping.get(de.cpp_decl.pxd_path, [])
      tmp.append(de)
      pxd_decl_mapping[ de.cpp_decl.pxd_path] = tmp

  # add __str__ if toString() method is declared:
  for d in decls:
      # enums, free functions, .. do not have a methods attribute
      methods = getattr(d, "methods", dict())
      to_strings = []
      for name, mdecls in methods.items():
          for mdecl in mdecls:
              name = mdecl.cpp_decl.annotations.get("wrap-cast", name)
              name = mdecl.cpp_decl.annotations.get("wrap-as", name)
              if name == "toString":
                  to_strings.append(mdecl)

      for to_string in to_strings:
          if len(to_string.arguments) == 0:
              d.methods.setdefault("__str__", []).append(to_string)
              print("ADDED __str__ method to", d.name)
              break

  # Split into chunks based on pxd files and store the mapping to decls, addons
  # and actual pxd files in a hash. We need to produce the exact number of chunks
  # as setup.py relies on it as well.
  pxd_files_chunk = chunkIt(list(pxd_decl_mapping.keys()), int(PY_NUM_MODULES))

  # Sanity checks: we should find all of our chunks and not have lost files
  if len(pxd_files_chunk) != int(PY_NUM_MODULES):
      raise Exception("Internal Error: number of chunks not equal to number of modules")
  if sum([len(ch) for ch in pxd_files_chunk]) != len(pxd_decl_mapping):
      raise Exception("Internal Error: chunking lost files")

  if (int(PY_NUM_MODULES)==1):
    mnames = ["_pyopenms"]
  else:
    mnames = ["_pyopenms_%s" % (k+1) for k in range(int(PY_NUM_MODULES))]
  allDecl_mapping = {}
  for pxd_f, m in zip(pxd_files_chunk, mnames):
      tmp_decls = []
      for f in pxd_f:
          tmp_decls.extend( pxd_decl_mapping[f] )

      allDecl_mapping[m] =  {"decls" : tmp_decls, "addons" : [] , "files" : pxd_f}

  # Deal with addons, make sure the addons are added to the correct compilation
  # unit (e.g. where the name corresponds to the pxd file).
  # Note that there are some special cases, e.g. addons that go into the first
  # unit or all *but* the first unit.
  is_added = [False for k in addons]
  for modname in mnames:

      for k,a in enumerate(addons):
          # Deal with special code that needs to go into all modules, only the
          # first or only all other modules...
          if modname == mnames[0]:
              if os.path.basename(a) == "ADD_TO_FIRST" + ".pyx":
                  allDecl_mapping[modname]["addons"].append(a)
                  is_added[k] = True
          else:
              if os.path.basename(a) == "ADD_TO_ALL_OTHER" + ".pyx":
                  allDecl_mapping[modname]["addons"].append(a)
                  is_added[k] = True
          if os.path.basename(a) == "ADD_TO_ALL" + ".pyx":
              allDecl_mapping[modname]["addons"].append(a)
              is_added[k] = True

          # Match addon basename to pxd basename
          for pfile in allDecl_mapping[modname]["files"]:
              if os.path.basename(a).split(".")[0] == os.path.basename(pfile).split(".")[0]:
                  allDecl_mapping[modname]["addons"].append(a)
                  is_added[k] = True

          # In the special case PY_NUM_MODULES==1 we need to mark ADD_TO_ALL_OTHER as is_added,
          # so it doesn't get added to pyopenms_1.pxd
          if PY_NUM_MODULES=='1':
              if os.path.basename(a) == "ADD_TO_ALL_OTHER" + ".pyx":
                  is_added[k] = True
                  
          if is_added[k]:
              continue


          # Also match by class name (sometimes one pxd contains multiple classes
          # and the addon is named after one of them)
          for dclass in allDecl_mapping[modname]["decls"]:
              if os.path.basename(a) == dclass.name + ".pyx":
                  allDecl_mapping[modname]["addons"].append(a)
                  is_added[k] = True

  # add any addons that did not get added anywhere else
  for k, got_added in enumerate(is_added):
      if not got_added:
          # add to all modules
          for m in mnames:
              allDecl_mapping[m]["addons"].append( addons[k] )


  for modname in mnames:
      autowrap_include_dirs = doCythonCodeGeneration(modname, allDecl_mapping, instance_map, converters)
      pickle.dump(autowrap_include_dirs, open(persisted_data_path, "wb"))

  argzip = [ (modname, allDecl_mapping[modname]["inc_dirs"]) for modname in mnames]

  import multiprocessing

  pool = multiprocessing.Pool(int(PY_NUM_THREADS))
  pool.map(doCythonCompile, argzip)
  pool.close()
  pool.join()

  print("Created all %s pyopenms.cpps" % PY_NUM_MODULES)


  with open("pyopenms/_all_modules.py", "w") as fp:
      for modname in mnames:
          fp.write("from .%s import *  # pylint: disable=wildcard-import; lgtm(py/polluting-import)\n" % modname)


  # create version information
  version = OPEN_MS_VERSION

  print("version=%r\n" % version, file=open("pyopenms/_version.py", "w"))
  print("info=%r\n" % QT_QMAKE_VERSION_INFO, file=open("pyopenms/_qt_version_info.py", "w"))
