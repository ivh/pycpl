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

#ifndef PYHDRL_CORE_PYCPL_TYPES_HPP_
#define PYHDRL_CORE_PYCPL_TYPES_HPP_

// Utility structs (not bindings) and functions for more C++-idiomatic usage
// of CPL's cpl_image, cpl_mask, and related functions.

#include <cpl_image_io.h>
#include <cpl_imagelist_io.h>
#include <cpl_mask.h>
#include <cpl_table.h>
#include <cpl_vector.h>
#include <cpl_wcs.h>
#include <hdrl_types.h>
#include <pybind11/pybind11.h>

#include "hdrlcore/error.hpp"

namespace py = pybind11;

namespace hdrl
{
namespace core
{

struct Value
{
  hdrl_value v;

  Value() {}

  Value(hdrl_value other) noexcept : v{other.data, other.error} {}

  Value(const Value& other) noexcept : v{other.v.data, other.v.error} {}

  void operator=(const Value& other) noexcept;
  void operator=(hdrl_value other) noexcept;
};

std::pair<cpl_size, cpl_size> cpl_coord(cpl_size x, cpl_size y);

std::pair<cpl_size, cpl_size> cpl_to_coord(cpl_size x, cpl_size y);

#define EXPAND_WINDOW(WINDOW) \
  WINDOW.llx + 1, WINDOW.lly + 1, WINDOW.urx + 1, WINDOW.ury + 1

struct pycpl_window
{
  cpl_size llx;
  cpl_size lly;
  cpl_size urx;
  cpl_size ury;

  static const pycpl_window All;

  pycpl_window() {}

  pycpl_window(cpl_size llx1, cpl_size lly1, cpl_size urx1,
               cpl_size ury1) noexcept
      : llx(llx1), lly(lly1), urx(urx1), ury(ury1)
  {
  }

  pycpl_window(const pycpl_window& other) noexcept
      : llx(other.llx), lly(other.lly), urx(other.urx), ury(other.ury)
  {
  }

  bool operator==(const pycpl_window& other) const noexcept;
  bool operator!=(const pycpl_window& other) const noexcept;
  // void operator=(const pycpl_window& other) noexcept;
  pycpl_window& operator=(const pycpl_window& other) noexcept;
};

std::ostream& operator<<(std::ostream& os, const pycpl_window& other) noexcept;

struct pycpl_image
{
  cpl_image* im;

  pycpl_image() : im(nullptr) {}

  pycpl_image(const pycpl_image& other) : im(nullptr)
  {
    if (other.im != nullptr) {
      im = Error::throw_errors_with(cpl_image_duplicate, other.im);
    }
  }

  pycpl_image(const cpl_image* other) : im(nullptr)
  {
    if (other != nullptr) {
      im = Error::throw_errors_with(cpl_image_duplicate, other);
    }
  }

  ~pycpl_image() { cpl_image_delete(im); }

  pycpl_image& operator=(const pycpl_image& other);
  pycpl_image& operator=(const cpl_image* other);
  // bool operator==(const pycpl_image& other) const noexcept;
  // bool operator!=(const pycpl_image& other) const noexcept;
};

struct pycpl_imagelist
{
  cpl_imagelist* il;

  pycpl_imagelist() { il = nullptr; }

  pycpl_imagelist(const pycpl_imagelist& other) : il(nullptr)
  {
    if (other.il != nullptr) {
      il = Error::throw_errors_with(cpl_imagelist_duplicate, other.il);
    }
  }

  pycpl_imagelist(cpl_imagelist* other) : il(nullptr)
  {
    if (other != nullptr) {
      il = Error::throw_errors_with(cpl_imagelist_duplicate, other);
    }
  }

  ~pycpl_imagelist() { cpl_imagelist_delete(il); }

  pycpl_imagelist& operator=(const pycpl_imagelist& other);
  pycpl_imagelist& operator=(const cpl_imagelist* other);
  // bool operator==(const pycpl_image& other) const noexcept;
  // bool operator!=(const pycpl_image& other) const noexcept;
};

struct pycpl_mask
{
  cpl_mask* m;

  pycpl_mask() { m = nullptr; }

  pycpl_mask(const pycpl_mask& other) : m(nullptr)
  {
    if (other.m != nullptr) {
      m = Error::throw_errors_with(cpl_mask_duplicate, other.m);
    }
  }

  pycpl_mask(cpl_mask* other) : m(nullptr)
  {
    if (other != nullptr) {
      m = Error::throw_errors_with(cpl_mask_duplicate, other);
    }
  }

  ~pycpl_mask() { cpl_mask_delete(m); }

  pycpl_mask& operator=(const pycpl_mask& other);
  pycpl_mask& operator=(const cpl_mask* other);
};

struct pycpl_vector
{
  cpl_vector* v;

  pycpl_vector() { v = nullptr; }

  pycpl_vector(const pycpl_vector& other) : v(nullptr)
  {
    if (other.v != nullptr) {
      v = Error::throw_errors_with(cpl_vector_duplicate, other.v);
    }
  }

  pycpl_vector(cpl_vector* other) : v(nullptr)
  {
    if (other != nullptr) {
      v = Error::throw_errors_with(cpl_vector_duplicate, other);
    }
  }

  ~pycpl_vector() { cpl_vector_delete(v); }

  pycpl_vector& operator=(const pycpl_vector& other);
  pycpl_vector& operator=(const cpl_vector* other);
};

struct pycpl_table
{
  cpl_table* t;

  pycpl_table() { t = nullptr; }

  pycpl_table(const pycpl_table& other) : t(nullptr)
  {
    if (other.t != nullptr) {
      t = Error::throw_errors_with(cpl_table_duplicate, other.t);
    }
  }

  pycpl_table(cpl_table* other) : t(nullptr)
  {
    if (other != nullptr) {
      t = Error::throw_errors_with(cpl_table_duplicate, other);
    }
  }

  ~pycpl_table() { cpl_table_delete(t); }

  pycpl_table& operator=(const pycpl_table& other);
  pycpl_table& operator=(const cpl_table* other);
};

struct pycpl_propertylist
{
  cpl_propertylist* pl;

  pycpl_propertylist() { pl = nullptr; }

  pycpl_propertylist(const pycpl_propertylist& other) : pl(nullptr)
  {
    if (other.pl != nullptr) {
      pl = Error::throw_errors_with(cpl_propertylist_duplicate, other.pl);
    }
  }

  pycpl_propertylist(cpl_propertylist* other) : pl(nullptr)
  {
    if (other != nullptr) {
      pl = Error::throw_errors_with(cpl_propertylist_duplicate, other);
    }
  }

  ~pycpl_propertylist() { cpl_propertylist_delete(pl); }

  pycpl_propertylist& operator=(const pycpl_propertylist& other);
  pycpl_propertylist& operator=(const cpl_propertylist* other);
};

struct pycpl_wcs
{
  cpl_wcs* w;

  pycpl_wcs() { w = nullptr; }

  pycpl_wcs(cpl_propertylist* plist) : w(nullptr)
  {
    if (plist != nullptr) {
      w = Error::throw_errors_with(cpl_wcs_new_from_propertylist, plist);
    }
  }

  pycpl_wcs(pycpl_propertylist p) : w(nullptr)
  {
    if (p.pl != nullptr) {
      w = Error::throw_errors_with(cpl_wcs_new_from_propertylist, p.pl);
    }
  }

  pycpl_wcs(const pycpl_wcs& other) : w(nullptr)
  {
    if (other.w != nullptr) {
      w = cpl_wcs_duplicate(other.w);
    }
  }

  pycpl_wcs(cpl_wcs* other) : w(nullptr)
  {
    if (other != nullptr) {
      w = cpl_wcs_duplicate(other);
    }
  }

  ~pycpl_wcs() { cpl_wcs_delete(w); }

  pycpl_wcs& operator=(const pycpl_wcs& other);
  pycpl_wcs& operator=(const cpl_wcs* other);
};

cpl_error_code
pyhdrl_wcs_to_propertylist(const cpl_wcs* wcs, cpl_propertylist* header,
                           cpl_boolean only2d);


// Numpy type conversions
std::optional<cpl_type> pycpl_numpy_type_to_cpl(py::object numpy_dtype);

}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_PYCPL_TYPES_HPP_
