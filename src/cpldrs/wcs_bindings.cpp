// This file is part of PyCPL the ESO CPL Python language bindings
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

#include "cpldrs/wcs_bindings.hpp"

#include <exception>

#include <pybind11/eval.h>
#include <pybind11/stl.h>

#include "cplcore/propertylist.hpp"
#include "cpldrs/wcs.hpp"

namespace py = pybind11;

py::object WCSLibError_pyclass;

void
bind_wcs(py::module& m)
{
  py::class_<cpl::drs::WCS> wcs_class(m, "WCS");

  wcs_class.doc() = R"docstring(
        WCS(cpl.core.PropertyList plist)
        
        Create a WCS object by parsing a propertylist.

        Notes
        -----
        The WCS object is created reading the WCS keyword information from the
        property list `plist` which is used to setup a WCSLIB data structure. In
        addition a few ancillary items are also filled in.

        It is allowed to pass a :py:class:`cpl.core.PropertyList` with a valid WCS
        structure and ``NAXIS`` = 0. Such a propertylist can be created by the method
        :py:meth:`platesol`.

        Trying to use any function without first installing WCSLIB will result in a
        :py:exc:`cpl.core.NoWCSError`.
        )docstring";


  py::exec(
      R"(
            class WCSLibError(Exception):
                """
                Used to return errors from WCSLIB conversion functions.

                Contains error_list attribute containing a list of all errors found
                in the opertation for each row in the format:
                (matrix row, error enum string)

                This is not meant to be thrown in the Python environment.
                """
                def __init__(self, error_list,message):
                        self.error_list = error_list
                        super().__init__(message)

        )",
      m.attr("__dict__"));
  WCSLibError_pyclass = m.attr("WCSLibError");

  py::enum_<cpl_wcs_trans_mode> trans_mode(wcs_class, "trans_mode");
  trans_mode.value("PHYS2WORLD", CPL_WCS_PHYS2WORLD)
      .value("WORLD2PHYS", CPL_WCS_WORLD2PHYS)
      .value("WORLD2STD", CPL_WCS_WORLD2STD)
      .value("PHYS2STD", CPL_WCS_PHYS2STD)
      .export_values();

  py::enum_<cpl_wcs_platesol_fitmode> platesol_fitmode(wcs_class,
                                                       "platesol_fitmode");
  platesol_fitmode.value("PLATESOL_4", CPL_WCS_PLATESOL_4)
      .value("PLATESOL_6", CPL_WCS_PLATESOL_6)
      .export_values();

  py::enum_<cpl_wcs_platesol_outmode> platsol_outmode(wcs_class,
                                                      "platesol_outmode");
  platsol_outmode.value("MV_CRVAL", CPL_WCS_MV_CRVAL)
      .value("MV_CRPIX", CPL_WCS_MV_CRPIX)
      .export_values();

  wcs_class.def(py::init<cpl::core::PropertyList>())
      .def("convert", &cpl::drs::WCS::convert, py::arg("from"),
           py::arg("transform"),
           R"mydelim(
    Convert between coordinate systems.

    Parameters
    ----------
    from : cpl.core.Matrix
        The input coordinate matrix
    transform : cpl.drs.WCS.trans_mode
        The transformation mode

    Returns
    -------
    cpl.core.Matrix
        The output coordinate matrix

    Raises
    ------
    cpl.drs.WCSLibError
        If any error occurs during conversion, retrieved from WCSLIB.
    cpl.core.UnspecifiedError
        If no rows or columns in the input matrix, or an unspecified
        error has occurred in the WCSLIB routine
    cpl.core.UnsupportedModeError
        If the input conversion mode is not supported

    Notes
    -----
    This function converts between several types of coordinates. These include:
    
    physical coordinates:
        The physical location on a detector (i.e. pixel coordinates)
    world coordinates:
         The real astronomical coordinate system for the observations. This may
         be spectral, celestial, time, etc.
    standard coordinates:
        These are an intermediate relative coordinate representation, defined as a
        distance from a reference point in the natural units of the world coordinate
        system. Any defined projection geometry will have already been included in the
        definition of standard coordinates.

    The supported conversion modes are:

    - cpl.drs.WCS.trans_mode.PHYS2WORLD: Converts from physical to world coordinates
    - cpl.drs.WCS.trans_mode.WORLD2PHYS: Converts from world to physical coordinates
    - cpl.drs.WCS.trans_mode.WORLD2STD: Converts from world to standard coordinates
    - cpl.drs.WCS.trans_mode.PHYS2STD: Converts from physical to standard coordinates
    )mydelim")
      .def_property_readonly(
          "image_naxis", &cpl::drs::WCS::get_image_naxis,
          "Dimensionality of the image associated with a WCS.")
      .def_property_readonly("image_dims", &cpl::drs::WCS::get_image_dims,
                             "Axis lengths of the image associated with a WCS.")
      .def_property_readonly("crval", &cpl::drs::WCS::get_crval)
      .def_property_readonly("crpix", &cpl::drs::WCS::get_crpix)
      .def_property_readonly("cd", &cpl::drs::WCS::get_cd)
      .def_property_readonly("ctype", &cpl::drs::WCS::get_ctype)
      .def_property_readonly("cunit", &cpl::drs::WCS::get_cunit)
      .def_static("platesol", &cpl::drs::WCS::platesol, py::arg("ilist"),
                  py::arg("cel"), py::arg("xy"), py::arg("niter"),
                  py::arg("thresh"), py::arg("fitmode"), py::arg("outmode"),
                  R"mydelim(
    Do a 2d plate solution given physical and celestial coordinates

    Parameters
    ----------
    ilist : cpl.core.PropertyList
        The input property list containing the first pass WCS
    cel : cpl.core.Matrix
        The celestial coordinate matrix
    xy : cpl.core.Matrix
        The physical coordinate matrix
    niter : int
        The number of fitting iterations
    thresh : float
        The threshold for the fitting rejection cycle
    fitmode : cpl.drs.WCS.platesol_fitmode
        The fitting mode (see below)
    outmode : cpl.drs.WCS.platesol_outmode
        The output mode (see below)

    Returns
    -------
    cpl.core.PropertyList
        The output property list containing the new WCS

    Notes
    -----
    This function allows for the following type of fits:

    - cpl.drs.WCS.PLATESOL_4: Fit for zero point, 1 scale and 1 rotation.
    - cpl.drs.WCS.PLATESOL_6: Fit for zero point, 2 scales, 1 rotation, 1 shear.

    This function allows the zeropoint to be defined by shifting either the
    physical or the celestial coordinates of the reference point:

    - cpl.drs.WCS.MV_CRVAL: Keeps the physical point fixed and shifts the celestial
    - cpl.drs.WCS.MV_CRPIX: Keeps the celestial point fixed and shifts the physical

    The output property list contains WCS relevant information only.

    Raises
    ------
    cpl.core.UnspecifiedError
        If unable to parse the input propertylist into a proper FITS WCS or there
        are too few points in the input matrices for a fit.
    cpl.core.IncompatibleInputError
        If the matrices `cel` and `xy` have different sizes.
    cpl.core.UnsupportedModeError
        If either fitmode or outmode are specified incorrectly.
    cpl.core.DataNotFoundError
        If the threshold is so low that no valid points are found. If the threshold 
        is not positive, this error is certain to occur.
    cpl.core.IllegalInputError
        If the parameter niter is non-positive.
    )mydelim");


  py::register_exception_translator(
      // Catch and convert WCSLibErrors to a catchable exception with a
      // error_list attribute
      [](std::exception_ptr p) -> void {
        py::object error_data_obj;
        py::object specific_error_class = WCSLibError_pyclass;

        try {
          if (p)
            std::rethrow_exception(p);
        }
        catch (const cpl::drs::WCSLibError& wcs_exception) {
          // This catch case assumes regular ownership/referencing rules
          // with regard to the error: The error is owned by the C++ runtime
          // or whatever threw it. A copy must be made to transfer to python.

          error_data_obj = specific_error_class(wcs_exception.error_list,
                                                wcs_exception.message);
        }

        // SetObject lets us raise any python object (the object = second
        // argument, class of said object = first argument)
        PyErr_SetObject(specific_error_class.ptr(),
                        error_data_obj.release().ptr());
      });
}
