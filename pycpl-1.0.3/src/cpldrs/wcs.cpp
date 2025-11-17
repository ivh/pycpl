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

#include "cpldrs/wcs.hpp"

#include <cstring>
#include <sstream>

#include <cpl_matrix.h>
#include <cpl_propertylist.h>

#include "cplcore/array.hpp"
#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{
std::vector<std::string> wcs_error_strings = {
    "WCSERR_SUCCESS",         "WCSERR_NULL_POINTER",    "WCSERR_MEMORY",
    "WCSERR_SINGULAR_MTX",    "WCSERR_BAD_CTYPE",       "WCSERR_BAD_PARAM",
    "WCSERR_BAD_COORD_TRANS", "WCSERR_ILL_COORD_TRANS", "WCSERR_BAD_PIX",
    "WCSERR_BAD_WORLD",       "WCSERR_BAD_WORLD_COORD", "WCSERR_NO_SOLUTION",
    "WCSERR_BAD_SUBIMAGE",    "WCSERR_NON_SEPARABLE"};

WCSLibError::WCSLibError(cpl_array* status_arr)
{
  for (int i = 0; i < cpl_array_get_size(status_arr); i++) {
    int code = cpl_array_get_int(status_arr, i, NULL);
    if (code != 0) {
      error_list.push_back(std::make_tuple(i, wcs_error_strings[code]));
    }
  }
  std::string error;
  int row;
  std::tie(row, error) = error_list[0];
  std::ostringstream msg_stream;
  msg_stream << error << " at row " << row << ", and " << error_list.size() - 1
             << " other errors (see error_list attribute for full list).";
  message = msg_stream.str();
}

WCSLibError::~WCSLibError() {}

const char*
WCSLibError::what() const throw()
{
  return message.c_str();
}

std::vector<std::tuple<int, std::string>>
WCSLibError::getList()
{
  return error_list;
}

WCS::WCS(cpl_wcs* to_steal) : m_interface(to_steal) {}

WCS::WCS(const cpl::core::PropertyList& plist)
{
  try {
    m_interface = cpl::core::Error::throw_errors_with(
        cpl_wcs_new_from_propertylist, plist.ptr().get());
  }
  catch (const cpl::core::NoWCSError& /* unused */) {
    throw(cpl::core::NoWCSError(
        PYCPL_ERROR_LOCATION,
        "The WCS sub library is not available. Install from "
        "https://www.atnf.csiro.au/people/mcalabre/WCS/"));
  }
}

WCS::~WCS()
{
  cpl::core::Error::throw_errors_with(cpl_wcs_delete, m_interface);
}

cpl::core::Matrix
WCS::convert(const cpl::core::Matrix& from, cpl_wcs_trans_mode transform) const
{
  cpl_matrix* to;     // Return variable as output parameter
  cpl_array* status;  // Status array tracks errors from wcslib
  cpl::core::Error::throw_errors_with(cpl_wcs_convert, m_interface, from.ptr(),
                                      &to, &status, transform);
  int* statusData = cpl_array_get_data_int(status);
  int statusLen = cpl_array_get_size(status);
  int* allzeroes = (int*)calloc(statusLen, sizeof(int));
  if (std::memcmp(allzeroes, statusData, statusLen) != 0) {
    // Error detected as not all data in status array is 0
    cpl_matrix_delete(to);
    WCSLibError toThrow(status);
    cpl_array_delete(status);
    throw toThrow;
  }

  cpl_array_delete(status);
  return cpl::core::Matrix(to);
}

cpl::core::PropertyList
WCS::platesol(const cpl::core::PropertyList& ilist,
              const cpl::core::Matrix& cel, const cpl::core::Matrix& xy,
              int niter, float thresh, cpl_wcs_platesol_fitmode fitmode,
              cpl_wcs_platesol_outmode outmode)
{
  cpl_propertylist* olist;
  cpl::core::Error::throw_errors_with(cpl_wcs_platesol, ilist.ptr().get(),
                                      cel.ptr(), xy.ptr(), niter, thresh,
                                      fitmode, outmode, &olist);
  return cpl::core::PropertyList(olist);
}

int
WCS::get_image_naxis() const
{
  return cpl::core::Error::throw_errors_with(cpl_wcs_get_image_naxis,
                                             m_interface);
}

std::vector<int>
WCS::get_image_dims() const
{
  return cpl::core::cpl_array_as_vector<int>(
      const_cast<cpl_array*>(cpl::core::Error::throw_errors_with(
          cpl_wcs_get_image_dims, m_interface)));
}

std::vector<double>
WCS::get_crval() const
{
  return cpl::core::cpl_array_as_vector<double>(const_cast<cpl_array*>(
      cpl::core::Error::throw_errors_with(cpl_wcs_get_crval, m_interface)));
}

std::vector<double>
WCS::get_crpix() const
{
  return cpl::core::cpl_array_as_vector<double>(const_cast<cpl_array*>(
      cpl::core::Error::throw_errors_with(cpl_wcs_get_crpix, m_interface)));
}

std::vector<std::string>
WCS::get_ctype() const
{
  return cpl::core::cpl_array_as_vector<std::string>(const_cast<cpl_array*>(
      cpl::core::Error::throw_errors_with(cpl_wcs_get_ctype, m_interface)));
}

std::vector<std::string>
WCS::get_cunit() const
{
  return cpl::core::cpl_array_as_vector<std::string>(const_cast<cpl_array*>(
      cpl::core::Error::throw_errors_with(cpl_wcs_get_cunit, m_interface)));
}

cpl::core::Matrix
WCS::get_cd() const
{
  const cpl_matrix* cpl_output =
      cpl::core::Error::throw_errors_with(cpl_wcs_get_cd, m_interface);
  cpl_matrix* duplicate =
      cpl::core::Error::throw_errors_with(cpl_matrix_duplicate, cpl_output);
  return cpl::core::Matrix(duplicate);
}

}  // namespace drs
}  // namespace cpl