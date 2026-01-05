// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2020-2024 European Southern Observatory
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

#ifndef PYHDRL_CORE_PYCPL_WCS_HPP_
#define PYHDRL_CORE_PYCPL_WCS_HPP_

#include <string>
#include <vector>

#include <cpl_memory.h>
#include <cpl_propertylist.h>
#include <cpl_type.h>
#include <cpl_wcs.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"


namespace py = pybind11;

namespace pybind11
{
namespace detail
{

template <>
struct type_caster<hdrl::core::pycpl_wcs>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_wcs' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_wcs
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_wcs, _("cpl.drs.WCS"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * hdrl::core::pycpl_wcs instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_drs = py::module_::import("cpl.drs");

    // Allow for None to result in a null ptr (cpl_wcs* pl = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.w = nullptr;
      return true;
    }
    // If the object type is not cpl.drs.WCS, throw an error
    if (!py::isinstance(source, pycpl_drs.attr("WCS"))) {
      value.w = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.drs.WCS type");
    }

    // Create a new cpl_wcs with the contents of the cpl.drs.WCS object
    try {
      // Implement Python equivalent of hdrl_wcs_to_propertylist()

      // The image dimensionality
      std::vector<int> idims =
          source.attr("image_dims").cast<std::vector<int>>();
      // Equivalent of source.attr("get_image_naxis")().cast<int>();
      int naxis = idims.size();
      std::vector<double> crval =
          source.attr("crval").cast<std::vector<double>>();
      std::vector<double> crpix =
          source.attr("crpix").cast<std::vector<double>>();
      std::vector<std::string> ctype =
          source.attr("ctype").cast<std::vector<std::string>>();
      std::vector<std::string> cunit =
          source.attr("cunit").cast<std::vector<std::string>>();
      // py::object since we have not yet implemented a custom type caster for
      // cpl.core.Matrix we can access its contents using __getitem__ (see
      // below)
      py::object cd = source.attr("cd");

      // An empty propertylist to add keywords to
      cpl_propertylist* header = cpl_propertylist_new();

      // Add keywords; this follows the approach in hdrl_wcs_to_propertylist
      // Check NAXIS
      for (cpl_size i = 0; i < naxis; i++) {
        if (i == 0) {
          cpl_propertylist_update_int(header, "NAXIS", naxis);
        }
        char* buf = cpl_sprintf("NAXIS%lld", i + 1);
        cpl_propertylist_update_int(header, buf, idims[i]);
        cpl_free(buf);
      }
#if 0
      // FIXME: Not relevant for our purposes?
      // Make sure to have the right NAXIS keywords if 2D is forced
      if (only2d == TRUE) {
          cpl_propertylist_update_int(header, "NAXIS", 2);

          if(cpl_propertylist_has(header, "NAXIS3")){
          cpl_propertylist_erase(header, "NAXIS3");
          }
        }
#endif

      // for 2D images
      if (crval.size() > 0) {
        cpl_propertylist_update_double(header, "CRVAL1", crval[0]);
        cpl_propertylist_update_double(header, "CRVAL2", crval[1]);
      }

      if (crpix.size() > 0) {
        cpl_propertylist_update_double(header, "CRPIX1", crpix[0]);
        cpl_propertylist_update_double(header, "CRPIX2", crpix[1]);
      }

      if (ctype.size() > 0) {
        cpl_propertylist_update_string(header, "CTYPE1", ctype[0].c_str());
        cpl_propertylist_update_string(header, "CTYPE2", ctype[1].c_str());
      }

      if (cunit.size() > 0) {
        cpl_propertylist_update_string(header, "CUNIT1", cunit[0].c_str());
        cpl_propertylist_update_string(header, "CUNIT2", cunit[1].c_str());
      }

      if (!cd.is(py::none())) {
        // Use __getitem__ to get the cpl.core.Matrix elements e.g. cd11 =
        // cd[0][0]
        double cd11 =
            cd.attr("__getitem__")(0).attr("__getitem__")(0).cast<double>();
        double cd12 =
            cd.attr("__getitem__")(0).attr("__getitem__")(1).cast<double>();
        double cd21 =
            cd.attr("__getitem__")(1).attr("__getitem__")(0).cast<double>();
        double cd22 =
            cd.attr("__getitem__")(1).attr("__getitem__")(1).cast<double>();
        cpl_propertylist_update_double(header, "CD1_1", cd11);
        cpl_propertylist_update_double(header, "CD1_2", cd12);
        cpl_propertylist_update_double(header, "CD2_1", cd21);
        cpl_propertylist_update_double(header, "CD2_2", cd22);
      }

      // For 3D cubes
      // if (only2d == FALSE && cpl_array_get_size(crval) > 2) {
      if (crval.size() > 2) {
        cpl_propertylist_update_double(header, "CRVAL3", crval[2]);

        if (crpix.size() > 2) {
          cpl_propertylist_update_double(header, "CRPIX3", crpix[2]);
        }

        if (ctype.size() > 2) {
          cpl_propertylist_update_string(header, "CTYPE3", ctype[2].c_str());
        }

        if (cunit.size() > 2) {
          cpl_propertylist_update_string(header, "CUNIT3", cunit[2].c_str());
        }

        if (!cd.is(py::none())) {
          double cd13 =
              cd.attr("__getitem__")(0).attr("__getitem__")(2).cast<double>();
          double cd23 =
              cd.attr("__getitem__")(1).attr("__getitem__")(2).cast<double>();
          double cd31 =
              cd.attr("__getitem__")(2).attr("__getitem__")(0).cast<double>();
          double cd32 =
              cd.attr("__getitem__")(2).attr("__getitem__")(1).cast<double>();
          double cd33 =
              cd.attr("__getitem__")(2).attr("__getitem__")(2).cast<double>();
          cpl_propertylist_update_double(header, "CD1_3", cd13);
          cpl_propertylist_update_double(header, "CD2_3", cd23);
          cpl_propertylist_update_double(header, "CD3_1", cd31);
          cpl_propertylist_update_double(header, "CD3_2", cd32);
          cpl_propertylist_update_double(header, "CD3_3", cd33);
        }
      }

      cpl_wcs* new_w = cpl_wcs_new_from_propertylist(header);
      value.w = new_w;
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_wcs
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_wcs src, return_value_policy, handle)
  {
    // if the pointer is null, return a None object
    if (src.w == nullptr) {
      return py::none();
    }

    // The input cpl_wcs object to convert to Python. First we must convert it
    // to a propertylist.
    cpl_wcs* input = src.w;

    // An empty propertylist for pyhdrl_wcs_to_propertylist
    cpl_propertylist* pl = cpl_propertylist_new();

    // pyhdrl_wcs_to_propertylist is a copy of hdrl_wcs_to_propertylist taken
    // from HDRL
    cpl_error_code err =
        hdrl::core::pyhdrl_wcs_to_propertylist(input, pl, CPL_FALSE);

    // convert the propertylist to a pycpl_propertylist object so we can use it
    // with a py::object
    hdrl::core::pycpl_propertylist new_pl = hdrl::core::pycpl_propertylist(pl);

    // Create a new cpl.drs.WCS object and fill with contents of the cpl_wcs*
    // input
    py::module_ pycpl_drs = py::module_::import("cpl.drs");
    py::object new_wcs = pycpl_drs.attr("WCS")(new_pl);
    // Return the result
    return new_wcs.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_WCS_HPP_