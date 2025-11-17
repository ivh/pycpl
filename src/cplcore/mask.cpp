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


// Aims to wrap cpl_mask structs as a C++ class.
// This wrapper will contain a pointer to the underlying CPL struct, using the
// appropriate CPL method for applying/retrieving information.

#include "cplcore/mask.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>

#include "cplcore/error.hpp"
#include "cplcore/image.hpp"

namespace cpl
{
namespace core
{
Mask::Mask(cpl_mask* to_steal) noexcept : m_interface(to_steal) {}

Mask::Mask(const Mask& other)
    : m_interface(
          Error::throw_errors_with(cpl_mask_duplicate, other.m_interface))
{
}

Mask::Mask(Mask&& other) noexcept
    : m_on_destruct(other.m_on_destruct), m_borrows(other.m_borrows),
      m_interface(other.m_interface)
{
  other.m_interface = nullptr;
  // At this point, the only reason we reset other.m_borrows is if has a value
  // moved into it again
  other.m_borrows = true;
  // This must be set or the moved-from will call it.
  other.m_on_destruct = {};
}

Mask::Mask(const ImageBase& in, double lo_cut, double hi_cut)
{
  m_interface = Error::throw_errors_with(cpl_mask_threshold_image_create,
                                         in.ptr(), lo_cut, hi_cut);
}

void
Mask::threshold_image(const ImageBase& image, double lo_cut, double hi_cut,
                      bool inval)
{
  Error::throw_errors_with(cpl_mask_threshold_image, m_interface, image.ptr(),
                           lo_cut, hi_cut, inval);
}

Mask&
Mask::operator=(const Mask& other)
{
  if (m_on_destruct) {
    m_on_destruct(*this);
  }
  if (!m_borrows) {
    cpl_mask_delete(m_interface);  // Doesn't throw
  }
  m_on_destruct = {};
  m_borrows = false;
  m_interface = nullptr;
  // If the below does throw, we don't want m_interface to point anywhere
  m_interface = Error::throw_errors_with(cpl_mask_duplicate, other.m_interface);
  return *this;
}

Mask&
Mask::operator=(Mask&& other) noexcept
{
  if (m_on_destruct) {
    m_on_destruct(*this);
  }
  if (!m_borrows) {
    cpl_mask_delete(m_interface);  // Doesn't throw
  }
  m_interface = other.m_interface;
  m_borrows = other.m_borrows;
  m_on_destruct = other.m_on_destruct;
  other.m_interface = nullptr;
  other.m_borrows = false;
  other.m_on_destruct = {};
  return *this;
}

Mask::Mask(cpl::core::size width, cpl::core::size height,
           unsigned char* bitmask)
    : m_interface(
          bitmask == nullptr
              ? Error::throw_errors_with(cpl_mask_new, width, height)
              : Error::throw_errors_with(cpl_mask_wrap, width, height, bitmask))
{
}

Mask::Mask(cpl::core::size width, cpl::core::size height, std::string bitmask)
    // THIS DOESN'T WORK because the underlying string might be deallocated
    // whilst the c_str() ptr is still in use by this mask. Not to mention,
    // the const cast requirement telling us somethings' wrong.
    // : m_interface(cpl_mask_wrap(width, height, const_cast<unsigned char *>(
    //     reinterpret_cast<const unsigned char *>(bitmask.c_str()))))
    : m_interface(cpl_mask_new(width, height))
{
  if (bitmask.length() != get_size()) {
    throw IllegalInputError(
        PYCPL_ERROR_LOCATION,
        "Mask input string size doesn't match width * height");
  }

  unsigned char* data_ptr =
      Error::throw_errors_with(cpl_mask_get_data, m_interface);
  std::memcpy(data_ptr, bitmask.c_str(), bitmask.length());
}

Mask::~Mask()
{
  if (m_on_destruct) {
    m_on_destruct(*this);
  }
  if (!m_borrows) {
    cpl_mask_delete(m_interface);  // Doesn't throw
  }
}

std::string
Mask::dump(std::optional<Window> window) const
{
  Window w = window.value_or(Window::All);
  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = Window::All;
  }
  if (w != Window::All) {
    int dw = this->get_width();
    int dh = this->get_height();
    std::ostringstream oss;
    if ((w.llx < 0) || (w.lly < 0) || (w.urx >= dw) || (w.ury >= dh)) {
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      oss << " exceeds mask bounds (0,0," << dw - 1 << "," << dh - 1 << ")";
      throw cpl::core::AccessOutOfRangeError(PYCPL_ERROR_LOCATION, oss.str());
    }
    if ((w.llx > w.urx) || (w.lly > w.ury)) {
      oss << "Invalid mask window definition: ";
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION, oss.str());
    }
  }
  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = Window::All;
  }
  if (w == Window::All) {
    w = {0, 0, this->get_width() - 1, this->get_height() - 1};
  }

  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(cpl_mask_dump_window, m_interface, EXPAND_WINDOW(w),
                           stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

unsigned char*
Mask::data()
{
  return Error::throw_errors_with(cpl_mask_get_data, m_interface);
}

const unsigned char*
Mask::data() const
{
  return Error::throw_errors_with(cpl_mask_get_data_const, m_interface);
}

bool
Mask::get_bit(size y, size x) const
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  return Error::throw_errors_with(cpl_mask_get, m_interface, coords.first,
                                  coords.second);
}

size
Mask::get_width() const
{
  return Error::throw_errors_with(cpl_mask_get_size_x, m_interface);
}

size
Mask::get_height() const
{
  return Error::throw_errors_with(cpl_mask_get_size_y, m_interface);
}

size
Mask::get_size() const
{
  return Error::throw_errors_with(cpl_mask_get_size_x, m_interface) *
         Error::throw_errors_with(cpl_mask_get_size_y, m_interface);
}

bool
Mask::is_empty() const
{
  return Error::throw_errors_with(cpl_mask_is_empty, m_interface);
}

size
Mask::count(Window area) const
{
  if (area == Window::All) {
    return Error::throw_errors_with(cpl_mask_count, m_interface);
  }
  return Error::throw_errors_with(cpl_mask_count_window, m_interface,
                                  EXPAND_WINDOW(area));
}

Mask&
Mask::and_with(const Mask& other)
{
  Error::throw_errors_with(cpl_mask_and, m_interface, other.m_interface);
  return *this;
}

Mask
Mask::operator&(const Mask& other) const
{
  return Mask(*this).and_with(other);
}

Mask&
Mask::or_with(const Mask& other)
{
  Error::throw_errors_with(cpl_mask_or, m_interface, other.m_interface);
  return *this;
}

Mask
Mask::operator|(const Mask& other) const
{
  return Mask(*this).or_with(other);
}

Mask&
Mask::xor_with(const Mask& other)
{
  Error::throw_errors_with(cpl_mask_xor, m_interface, other.m_interface);
  return *this;
}

Mask
Mask::operator^(const Mask& other) const
{
  return Mask(*this).xor_with(other);
}

Mask&
Mask::negate()
{
  Error::throw_errors_with(cpl_mask_not, m_interface);
  return *this;
}

Mask
Mask::operator~() const
{
  return Mask(*this).negate();
}

Mask
Mask::collapse_rows() const
{
  return Mask(
      Error::throw_errors_with(cpl_mask_collapse_create, m_interface, 0));
}

Mask
Mask::collapse_cols() const
{
  return Mask(
      Error::throw_errors_with(cpl_mask_collapse_create, m_interface, 1));
}

Mask
Mask::extract(Window window) const
{
  if (window == Window::All) {
    return Mask(*this);
  }

  return Mask(Error::throw_errors_with(cpl_mask_extract, m_interface,
                                       EXPAND_WINDOW(window)));
}

Mask&
Mask::rotate(int right_angle_turns)
{
  Error::throw_errors_with(cpl_mask_turn, m_interface, right_angle_turns);
  return *this;
}

Mask&
Mask::shift(size y_shift, size x_shift)
{
  Error::throw_errors_with(cpl_mask_shift, m_interface, x_shift, y_shift);
  return *this;
}

Mask&
Mask::insert(const Mask& to_insert, size y, size x)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  Error::throw_errors_with(cpl_mask_copy, m_interface, to_insert.m_interface,
                           coords.first, coords.second);
  return *this;
}

// TODO: Possibly create new enum for axis? For readability.
Mask&
Mask::flip(int axis)
{
  Error::throw_errors_with(cpl_mask_flip, m_interface, axis);
  return *this;
}

Mask&
Mask::move(size nb_cut, const std::vector<size>& positions)
{
  /**
   *
   * nb_cut divides this image into nb_cut×nb_cut equisized tiles,
   * without any left over pixels.
   *
   *
   */
  if (nb_cut * nb_cut != positions.size()) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "positions not equal to nb_cut^2");
  }
  if (this->get_width() % nb_cut != 0 || this->get_height() % nb_cut != 0 ||
      nb_cut < 1) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "nb_cut of " + std::to_string(nb_cut) +
                                " cant slice mask of shape" +
                                std::to_string(this->get_width()) + "x" +
                                std::to_string(this->get_height()));
  }
  Error::throw_errors_with(cpl_mask_move, m_interface, nb_cut, &positions[0]);
  return *this;
}

Mask
Mask::subsample(size ystep, size xstep) const
{
  return Mask(Error::throw_errors_with(cpl_mask_extract_subsample, m_interface,
                                       xstep, ystep));
}

Mask
Mask::filter(const Mask& kernel, cpl_filter_mode filter,
             cpl_border_mode border) const
{
  // Keep the output the same as input size.
  Mask filter_output = Mask(get_width(), get_height());

  Error::throw_errors_with(cpl_mask_filter, filter_output.m_interface,
                           m_interface, kernel.m_interface, filter, border);
  return filter_output;
}

const cpl_mask*
Mask::ptr() const
{
  return m_interface;
}

cpl_mask*
Mask::ptr()
{
  return m_interface;
}

std::optional<cpl_mask*>
Mask::unwrap(Mask&& self)
{
  if (self.m_borrows) {
    return nullptr;
  }
  cpl_mask* interface = self.m_interface;
  self.m_interface = nullptr;
  return {interface};
}

Mask::operator std::string() const
{
  return std::string(reinterpret_cast<const char*>(data()));
}

bool
Mask::operator==(const Mask& other) const
{
  return std::memcmp(data(), other.data(), get_size()) == 0;
}

bool
Mask::operator!=(const Mask& other) const
{
  return !operator==(other);
}

Mask
load_mask(const std::filesystem::path& fitsfile, size plane, size extension,
          Window area)
{
  if (area == Window::All) {
    return Mask(Error::throw_errors_with(cpl_mask_load, fitsfile.c_str(), plane,
                                         extension));
  } else {
    return Mask(Error::throw_errors_with(cpl_mask_load_window, fitsfile.c_str(),
                                         plane, extension,
                                         EXPAND_WINDOW(area)));
  }
}

const Mask&
Mask::save(const std::filesystem::path& filename, const PropertyList& pl,
           unsigned mode) const
{
  Error::throw_errors_with(cpl_mask_save, m_interface, filename.c_str(),
                           pl.ptr().get(), mode);
  return *this;
}

}  // namespace core
}  // namespace cpl
