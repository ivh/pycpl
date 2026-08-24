// This file is part of PyCPL the ESO CPL Python language bindings
// Copyright (C) 2020-2026 European Southern Observatory
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <string>

#include <cpl_error.h>
#include <cpl_init.h>
#include <pybind11/pybind11.h>

#include "cplcore/error_bindings.hpp"
#include "cplcore/filter_bindings.hpp"
#include "cplcore/image_bindings.hpp"
#include "cplcore/io_bindings.hpp"
#include "cplcore/mask_bindings.hpp"
#include "cplcore/matrix_bindings.hpp"
#include "cplcore/msg_bindings.hpp"
#include "cplcore/polynomial_bindings.hpp"
#include "cplcore/property_bindings.hpp"
#include "cplcore/table_bindings.hpp"
#include "cplcore/type_bindings.hpp"
#include "cplcore/vector_bindings.hpp"
#include "cpldfs/dfs_bindings.hpp"
#include "cpldrs/apertures_bindings.hpp"
#include "cpldrs/detector_bindings.hpp"
#include "cpldrs/fft_bindings.hpp"
#include "cpldrs/fit_bindings.hpp"
#include "cpldrs/geom_img_bindings.hpp"
#include "cpldrs/photom_bindings.hpp"
#include "cpldrs/ppm_bindings.hpp"
#include "cpldrs/wcs_bindings.hpp"
#include "cpldrs/wlcalib_bindings.hpp"
#include "cplui/frame_bindings.hpp"
#include "cplui/parameter_bindings.hpp"
#include "cplui/plugin_bindings.hpp"

// HDRL bindings
#include "hdrlcore/error_bindings.hpp"
#include "hdrlcore/image_bindings.hpp"
#include "hdrlcore/imagelist_bindings.hpp"
#include "hdrlcore/spectrum_bindings.hpp"
#include "hdrldebug/types_bindings.hpp"
#include "hdrlfunc/airmass_bindings.hpp"
#include "hdrlfunc/barycorr_bindings.hpp"
#include "hdrlfunc/bpm_bindings.hpp"
#include "hdrlfunc/catalogue_bindings.hpp"
#include "hdrlfunc/collapse_bindings.hpp"
#include "hdrlfunc/dar_bindings.hpp"
#include "hdrlfunc/efficiency_bindings.hpp"
#include "hdrlfunc/flat_bindings.hpp"
#include "hdrlfunc/fpn_bindings.hpp"
#include "hdrlfunc/fringe_bindings.hpp"
#include "hdrlfunc/lacosmic_bindings.hpp"
#include "hdrlfunc/maglim_bindings.hpp"
#include "hdrlfunc/overscan_bindings.hpp"
#include "hdrlfunc/resample_bindings.hpp"
#include "hdrlfunc/response_bindings.hpp"
#include "hdrlfunc/strehl_bindings.hpp"

namespace py = pybind11;

/**
 * @brief Python-called code to bind all C++ modules to Python.
 *
 * This is the top-level function that is run by Python/Pybind
 * to bind PyCPL's methods to python.
 * Its purpose is to setup the top-level Python module named 'cpl', and
 * call into submodule binding functions to do the rest of the binding.
 *
 * These called functions are named bind_<subject>, and are usually
 * defined in src/cpl/<module>/<subject>_bindings.*
 *
 * There may be multiple files/functions that bind to the same module,
 * This is because modules correspond to C CPL's modules: cplcore, cplui, etc,
 * whereas it would be prudent to separate some of the related bindings
 * to their own files & functions.
 *
 * The extent of Python specific code is within this file, and all
 * files ending in '_bindings'.
 * All other C++ sources files should be compilable without pybind.
 */
PYBIND11_MODULE(cpl, m)
{
  // m.doc(R"pydoc(
  // )pydoc")

  // Get library descriptions on import
  m.add_object(
      "DESCRIPTION",
      py::str(std::string(cpl_get_description(CPL_DESCRIPTION_DEFAULT))),
      "str : a string of version numbers of CPL and its "
      "libraries.");

  m.add_object("__version__", py::str(PYCPL_VERSION),
               "str: package version string");

  // Initialise cpl needed for memory management on the C side. Will execute on
  // import
  cpl_init(CPL_INIT_DEFAULT);
  // in case cpl_init has already been called elsewhere (e.g. PyHDRL)
  cpl_error_reset();

  py::module_ cplui = m.def_submodule("ui", R"pydoc(CPL UI submodule
  
  This module provides the features to implement data processing modules 
  (recipes) which can be executed using the ESO data processing environments.
  It provides the plugin API needed to implement these modules as well as the
  data types to pass data and configuration parameters to these modules. 
  )pydoc");
  bind_parameters(cplui);
  bind_frames(cplui);
  bind_plugin(cplui);

  py::module_ cplcore = m.def_submodule("core", R"pydoc(CPL Core submodule
  
  This module provides the core functionality of CPL. It provides the basic
  data types, like images and tables, the basic operations defined for
  these types, and also basic input and output operations. In addition
  the module provides utilities for basic image and signal processing,
  error handling, log message output.
  )pydoc");
  bind_types(cplcore);
  bind_errors(cplcore);
  bind_mask(cplcore);
  bind_filters(cplcore);
  bind_image(cplcore);
  bind_propertylist(cplcore);
  bind_table(cplcore);
  bind_vector(cplcore);
  bind_message(cplcore);
  bind_io(cplcore);
  bind_matrix(cplcore);
  bind_polynomial(cplcore);

  py::module_ dfs = m.def_submodule("dfs", R"pydoc(CPL DFS submodule

  This module provides the necessary functionality to create data products
  which comply to the ESO Data Interface Control Document (DICD) standard.
  The utility functions provided here make sure that data products are
  written using the correct format and structure, and that all required
  date product keywords are present.

  The use of these utilities is mandatory for the data processing pipelines
  of the ESO VLT and ELT instruments, or any other software component which
  is intended to be operated by ESO for the purpose of generating data
  products to be stored in the ESO Archive or to be used for data quality
  control.
  )pydoc");
  bind_dfs(dfs);

  py::module_ cpldrs = m.def_submodule("drs", R"pydoc(CPL DRS submodule
  
  This module provides standard implementations of instrument independent,
  higher level data processing functions for general non-linear fitting, image
  fourier transformation, point pattern matching, world coordinate system
  transformation, etc.
  )pydoc");
  bind_wlcalib(cpldrs);
  bind_detector(cpldrs);
  bind_fit(cpldrs);
  bind_fft(cpldrs);
  bind_ppm(cpldrs);
  bind_wcs(cpldrs);
  bind_photom(cpldrs);
  bind_apertures(cpldrs);
  bind_geom_img(cpldrs);

  // HDRL module (accessible as cpl.hdrl or: from cpl import hdrl)
  py::module_ hdrl = m.def_submodule("hdrl", R"pydoc(HDRL - High Level Data Reduction Library

  Python bindings for the ESO High Level Data Reduction Library (HDRL).
  Provides algorithms for astronomical data reduction including airmass
  calculation, bad pixel detection, flat fielding, fringing correction,
  overscan correction, image resampling, and more.
  )pydoc");

  py::module_ hdrlcore = hdrl.def_submodule("core", R"pydoc(HDRL Core submodule

  This module provides the core data types required by the HDRL algorithms,
  including Image (with error propagation) and ImageList.
  )pydoc");
  bind_hdrl_errors(hdrlcore);
  bind_hdrl_image(hdrlcore);
  bind_hdrl_imagelist(hdrlcore);
  bind_spectrum1d(hdrlcore);

  py::module_ hdrlfunc = hdrl.def_submodule("func", R"pydoc(HDRL Functionalities submodule

  This module provides the Python API for HDRL algorithms including:
  effective airmass, bad-pixel detection, barycentric correction,
  flat fielding, fringing, overscan correction, image resampling, and more.
  )pydoc");
  bind_airmass(hdrlfunc);
  bind_barycorr(hdrlfunc);
  bind_bpm(hdrlfunc);
  bind_catalogue(hdrlfunc);
  bind_collapse(hdrlfunc);
  bind_dar(hdrlfunc);
  bind_efficiency(hdrlfunc);
  bind_flat(hdrlfunc);
  bind_fpn(hdrlfunc);
  bind_fringe(hdrlfunc);
  bind_lacosmic(hdrlfunc);
  bind_maglim(hdrlfunc);
  bind_overscan(hdrlfunc);
  bind_resample(hdrlfunc);
  bind_response(hdrlfunc);
  bind_strehl(hdrlfunc);

  py::module_ hdrldebug = hdrl.def_submodule("debug", R"pydoc(HDRL Debug submodule

  This module provides utilities for testing PyCPL to PyHDRL type converters.
  Not intended for use by pipeline developers.
  )pydoc");
  bind_hdrl_types(hdrldebug);
}
