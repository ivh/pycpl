// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

// Utility structs (not bindings) and functions for more C++-idiomatic usage
// of CPL's cpl_image, cpl_mask, and related functions.

#include "hdrlcore/pycpl_types.hpp"

#include <optional>
#include <ostream>

#include <cpl_imagelist.h>
#include <cpl_imagelist_io.h>
#include <cpl_memory.h>

namespace hdrl
{
namespace core
{
// hdrl::core::pycpl_image overloaded operators
pycpl_image&
pycpl_image::operator=(const pycpl_image& other)
{
  cpl_image_delete(im);
  im = nullptr;
  im = Error::throw_errors_with(cpl_image_duplicate, other.im);
  return *this;
}

pycpl_image&
pycpl_image::operator=(const cpl_image* other)
{
  cpl_image_delete(im);
  im = nullptr;
  im = Error::throw_errors_with(cpl_image_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_imagelist overloaded operators
pycpl_imagelist&
pycpl_imagelist::operator=(const pycpl_imagelist& other)
{
  cpl_imagelist_delete(il);
  il = nullptr;
  il = Error::throw_errors_with(cpl_imagelist_duplicate, other.il);
  return *this;
}

pycpl_imagelist&
pycpl_imagelist::operator=(const cpl_imagelist* other)
{
  cpl_imagelist_delete(il);
  il = nullptr;
  il = Error::throw_errors_with(cpl_imagelist_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_mask overloaded operators
pycpl_mask&
pycpl_mask::operator=(const pycpl_mask& other)
{
  cpl_mask_delete(m);
  m = nullptr;
  m = Error::throw_errors_with(cpl_mask_duplicate, other.m);
  return *this;
}

pycpl_mask&
pycpl_mask::operator=(const cpl_mask* other)
{
  cpl_mask_delete(m);
  m = nullptr;
  m = Error::throw_errors_with(cpl_mask_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_vector overloaded operators
pycpl_vector&
pycpl_vector::operator=(const pycpl_vector& other)
{
  cpl_vector_delete(v);
  v = nullptr;
  v = Error::throw_errors_with(cpl_vector_duplicate, other.v);
  return *this;
}

pycpl_vector&
pycpl_vector::operator=(const cpl_vector* other)
{
  cpl_vector_delete(v);
  v = nullptr;
  v = Error::throw_errors_with(cpl_vector_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_table overloaded operators
pycpl_table&
pycpl_table::operator=(const pycpl_table& other)
{
  cpl_table_delete(t);
  t = nullptr;
  t = Error::throw_errors_with(cpl_table_duplicate, other.t);
  return *this;
}

pycpl_table&
pycpl_table::operator=(const cpl_table* other)
{
  cpl_table_delete(t);
  t = nullptr;
  t = Error::throw_errors_with(cpl_table_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_propertylist overloaded operators
pycpl_propertylist&
pycpl_propertylist::operator=(const pycpl_propertylist& other)
{
  cpl_propertylist_delete(pl);
  pl = nullptr;
  pl = Error::throw_errors_with(cpl_propertylist_duplicate, other.pl);
  return *this;
}

pycpl_propertylist&
pycpl_propertylist::operator=(const cpl_propertylist* other)
{
  cpl_propertylist_delete(pl);
  pl = nullptr;
  pl = Error::throw_errors_with(cpl_propertylist_duplicate, other);
  return *this;
}

// hdrl::core::pycpl_wcs overloaded operators
pycpl_wcs&
pycpl_wcs::operator=(const pycpl_wcs& other)
{
  cpl_wcs_delete(w);
  w = nullptr;
  w = Error::throw_errors_with(cpl_wcs_duplicate, other.w);
  return *this;
}

pycpl_wcs&
pycpl_wcs::operator=(const cpl_wcs* other)
{
  cpl_wcs_delete(w);
  w = nullptr;
  w = Error::throw_errors_with(cpl_wcs_duplicate, other);
  return *this;
}

// hdrl::core::Value
void
Value::operator=(const Value& other) noexcept
{
  v.data = other.v.data;
  v.error = other.v.error;
}

void
Value::operator=(hdrl_value other) noexcept
{
  v.data = other.data;
  v.error = other.error;
}

// hdrl::core::pycpl_window
bool
pycpl_window::operator==(const pycpl_window& other) const noexcept
{
  return llx == other.llx && lly == other.lly && urx == other.urx &&
         ury == other.ury;
}

bool
pycpl_window::operator!=(const pycpl_window& other) const noexcept
{
  return llx != other.llx || lly != other.lly || urx != other.urx ||
         ury != other.ury;
}

/*void
pycpl_window::operator=(const pycpl_window& other) noexcept
{
  llx = other.llx;
  lly = other.lly;
  urx = other.urx;
  ury = other.ury;
}*/

pycpl_window&
pycpl_window::operator=(const pycpl_window& other) noexcept
{
  llx = other.llx;
  lly = other.lly;
  urx = other.urx;
  ury = other.ury;
  return *this;
}

std::ostream&
operator<<(std::ostream& os, const pycpl_window& other) noexcept
{
  os << "pycpl_window(" << other.llx << "," << other.lly << "," << other.urx
     << "," << other.ury << ")";
  return os;
}

const pycpl_window pycpl_window::All(-1891, -1891, -9012, -9012);

std::pair<cpl_size, cpl_size>
cpl_coord(cpl_size x, cpl_size y)
{
  return std::make_pair(x + 1, y + 1);
}

std::pair<cpl_size, cpl_size>
cpl_to_coord(cpl_size x, cpl_size y)
{
  return std::make_pair(x - 1, y - 1);
}

/*---------------------------------------------------------------------------*/
/**
  @private
  @brief   Write WCS properties in a cpl propertylist.
  @param   wcs      cpl_wcs structure containing the wcs information
  @param   header   output header informations
  @param   only2d   if TRUE save only the 2D part of the wcs structure
  @return  CPL_ERROR_NONE on non-critical failure or success, another CPL error
           code otherwise.
 */
/*---------------------------------------------------------------------------*/
cpl_error_code
pyhdrl_wcs_to_propertylist(const cpl_wcs* wcs, cpl_propertylist* header,
                           cpl_boolean only2d)
{
  cpl_ensure_code(wcs && header, CPL_ERROR_NULL_INPUT);
  int err = 0;
  const cpl_array* crval = cpl_wcs_get_crval(wcs);
  const cpl_array* crpix = cpl_wcs_get_crpix(wcs);
  const cpl_array* ctype = cpl_wcs_get_ctype(wcs);
  const cpl_array* cunit = cpl_wcs_get_cunit(wcs);

  const cpl_matrix* cd = cpl_wcs_get_cd(wcs);

  const cpl_array* dims = cpl_wcs_get_image_dims(wcs);
  int naxis = cpl_wcs_get_image_naxis(wcs);


  /* Check NAXIS */
  for (cpl_size i = 0; i < naxis; i++) {
    if (i == 0) {
      cpl_propertylist_update_int(header, "NAXIS", naxis);
    }
    char* buf = cpl_sprintf("NAXIS%lld", i + 1);
    cpl_propertylist_update_int(header, buf, cpl_array_get_int(dims, i, &err));
    cpl_free(buf);
  }

  /* Make sure to have the right NAXIS keywords if 2D is forced */
  if (only2d == TRUE) {
    cpl_propertylist_update_int(header, "NAXIS", 2);

    if (cpl_propertylist_has(header, "NAXIS3")) {
      cpl_propertylist_erase(header, "NAXIS3");
    }
  }

  /* for 2D images */
  if (crval) {
    cpl_propertylist_update_double(header, "CRVAL1",
                                   cpl_array_get_double(crval, 0, &err));
    cpl_propertylist_update_double(header, "CRVAL2",
                                   cpl_array_get_double(crval, 1, &err));
  }

  if (crpix) {
    cpl_propertylist_update_double(header, "CRPIX1",
                                   cpl_array_get_double(crpix, 0, &err));
    cpl_propertylist_update_double(header, "CRPIX2",
                                   cpl_array_get_double(crpix, 1, &err));
  }

  if (ctype) {
    cpl_propertylist_update_string(header, "CTYPE1",
                                   cpl_array_get_string(ctype, 0));
    cpl_propertylist_update_string(header, "CTYPE2",
                                   cpl_array_get_string(ctype, 1));
  }

  if (cunit) {
    cpl_propertylist_update_string(header, "CUNIT1",
                                   cpl_array_get_string(cunit, 0));
    cpl_propertylist_update_string(header, "CUNIT2",
                                   cpl_array_get_string(cunit, 1));
  }

  if (cd) {
    double cd11 = cpl_matrix_get(cd, 0, 0);
    double cd12 = cpl_matrix_get(cd, 0, 1);
    double cd21 = cpl_matrix_get(cd, 1, 0);
    double cd22 = cpl_matrix_get(cd, 1, 1);
    cpl_propertylist_update_double(header, "CD1_1", cd11);
    cpl_propertylist_update_double(header, "CD1_2", cd12);
    cpl_propertylist_update_double(header, "CD2_1", cd21);
    cpl_propertylist_update_double(header, "CD2_2", cd22);
  }

  /* for 3D cubes */
  if (only2d == FALSE && cpl_array_get_size(crval) > 2) {
    if (crval) {
      cpl_propertylist_update_double(header, "CRVAL3",
                                     cpl_array_get_double(crval, 2, &err));
    }

    if (crpix) {
      cpl_propertylist_update_double(header, "CRPIX3",
                                     cpl_array_get_double(crpix, 2, &err));
    }

    if (ctype) {
      cpl_propertylist_update_string(header, "CTYPE3",
                                     cpl_array_get_string(ctype, 2));
    }

    if (cunit) {
      cpl_propertylist_update_string(header, "CUNIT3",
                                     cpl_array_get_string(cunit, 2));
    }

    if (cd) {
      double cd13 = cpl_matrix_get(cd, 0, 2);
      double cd23 = cpl_matrix_get(cd, 1, 2);
      double cd31 = cpl_matrix_get(cd, 2, 0);
      double cd32 = cpl_matrix_get(cd, 2, 1);
      double cd33 = cpl_matrix_get(cd, 2, 2);
      cpl_propertylist_update_double(header, "CD1_3", cd13);
      cpl_propertylist_update_double(header, "CD2_3", cd23);
      cpl_propertylist_update_double(header, "CD3_1", cd31);
      cpl_propertylist_update_double(header, "CD3_2", cd32);
      cpl_propertylist_update_double(header, "CD3_3", cd33);
    }
  } /* if 3D */
  return CPL_ERROR_NONE;
}

namespace
{
// Keep list of type mappings in sync with the corresponding
// definitions in PyCPL (type_bindings.cpp)!
std::array<std::pair<py::object, cpl_type>, 18>
init_numpy_type_conversions()
{
  py::object numpy_import = py::module::import("numpy");
  auto elem = [&numpy_import](const char* name, cpl_type ty) {
    return std::make_pair(numpy_import.attr(name), ty);
  };
  return {
      // https://numpy.org/doc/stable/user/basics.types.html
      elem("byte", CPL_TYPE_CHAR),
      elem("ubyte", CPL_TYPE_UCHAR),
      elem("bool_", CPL_TYPE_BOOL),
      elem("short", CPL_TYPE_SHORT),
      elem("ushort", CPL_TYPE_USHORT),
      elem("intc", CPL_TYPE_INT),
      elem("uintc", CPL_TYPE_UINT),
      elem("int_", CPL_TYPE_LONG),
      elem("uint", CPL_TYPE_ULONG),
      elem("longlong", CPL_TYPE_LONG_LONG),
      elem("single", CPL_TYPE_FLOAT),
      elem("double", CPL_TYPE_DOUBLE),
      elem("csingle", CPL_TYPE_FLOAT_COMPLEX),
      elem("cdouble", CPL_TYPE_DOUBLE_COMPLEX),
      // Fixed-sized types that correspond to CPL types directly
      // (Since CPL doesn't do fixed-sized uint_t/int_t
      // this is just for floating-points)
      elem("float32", CPL_TYPE_FLOAT),
      elem("float64", CPL_TYPE_DOUBLE),
      elem("complex64", CPL_TYPE_FLOAT_COMPLEX),
      elem("complex128", CPL_TYPE_DOUBLE_COMPLEX),
  };
}
}  // namespace

std::optional<cpl_type>
pycpl_numpy_type_to_cpl(py::object numpy_dtype)
{
  const static thread_local std::array<std::pair<py::object, cpl_type>, 18>
      conversions = init_numpy_type_conversions();

  for (auto objtotype : conversions) {
    if (objtotype.first.equal(numpy_dtype)) {
      return {objtotype.second};
    }
  }
  return {};
}

}  // namespace core
}  // namespace hdrl
