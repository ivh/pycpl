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

#include "hdrlcore/image.hpp"

#include <cstdio>
#include <sstream>
#include <utility>

#include <cpl_mask.h>
#include <hdrl_image_math.h>
#include <hdrl_types.h>

#include "hdrlcore/error.hpp"

// Need to have an Error class similar to pycpl one ... so we can use
// Error::throw_errors_with

namespace hdrl
{
namespace core
{
// using Image = hdrl::core::Image;
// should use hdrl::core::size instead of size?
Image::Image(int width, int height)
{
  m_interface = Error::throw_errors_with(hdrl_image_new, width, height);
}

Image::Image(pycpl_image i1, pycpl_image i2)
{
  m_interface = Error::throw_errors_with(hdrl_image_create, i1.im, i2.im);
}

Image::Image(hdrl_image* to_steal) : m_interface(to_steal) {}

/*Image::Image(const std::shared_ptr<Image& other) : m_interface(
    //best to leave this commented out, otherwise we run into memory issues.
    Error::throw_errors_with(hdrl_image_duplicate,other.m_interface))
{
}
*/

std::shared_ptr<Image>
Image::duplicate() const
{
  return std::make_shared<Image>(
      Error::throw_errors_with(hdrl_image_duplicate, m_interface));
}

pycpl_mask
Image::get_mask()
{
  cpl_mask* m = Error::throw_errors_with(hdrl_image_get_mask, m_interface);
  return pycpl_mask(m);
}

pycpl_image
Image::get_image()
{
  pycpl_image im = Error::throw_errors_with(hdrl_image_get_image, m_interface);
  return im;
}

pycpl_image
Image::get_error()
{
  pycpl_image err = Error::throw_errors_with(hdrl_image_get_error, m_interface);
  return err;
}

int
Image::get_width() const
{
  return Error::throw_errors_with(hdrl_image_get_size_x, m_interface);
}

int
Image::get_height() const
{
  return Error::throw_errors_with(hdrl_image_get_size_y, m_interface);
}

int
Image::get_size() const
{
  int size = hdrl::core::Image::get_height() * hdrl::core::Image::get_width();
  return size;  // Error::throw_errors_with(size, m_interface);
}

hdrl_image*
Image::ptr() const
{
  return m_interface;
}

cpl_error_code
Image::reject(cpl_size ypos, cpl_size xpos)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  cpl_error_code err = Error::throw_errors_with(hdrl_image_reject, m_interface,
                                                coords.first, coords.second);
  return err;
}

void
Image::reject_value(cpl_value mode)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_reject_value, m_interface, mode);
}

void
Image::reject_from_mask(pycpl_mask map)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_reject_from_mask, m_interface, map.m);
}

void
Image::turn(int rot)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_turn, m_interface, rot);
}

void
Image::copy_into(std::shared_ptr<Image> other, cpl_size ypos, cpl_size xpos)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  cpl_error_code err = Error::throw_errors_with(
      hdrl_image_copy, m_interface, other->ptr(), coords.first, coords.second);
}

void
Image::insert_into(pycpl_image image, std::optional<pycpl_image> error,
                   cpl_size ypos, cpl_size xpos)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  if (!error.has_value()) {
    Error::throw_errors_with(hdrl_image_insert, m_interface, image.im, nullptr,
                             coords.first, coords.second);
  } else {
    Error::throw_errors_with(hdrl_image_insert, m_interface, image.im,
                             error.value().im, coords.first, coords.second);
  }
}

cpl_size
Image::count_rejected()
{
  return Error::throw_errors_with(hdrl_image_count_rejected, m_interface);
}

bool
Image::is_rejected(cpl_size ypos, cpl_size xpos)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  int is_rej = Error::throw_errors_with(hdrl_image_is_rejected, m_interface,
                                        coords.first, coords.second);
  return static_cast<bool>(is_rej);
}

Value
Image::get_pixel(cpl_size ypos, cpl_size xpos)
{
  int d;
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  hdrl_value pix = Error::throw_errors_with(hdrl_image_get_pixel, m_interface,
                                            coords.first, coords.second, &d);
  return Value(pix);
}

void
Image::set_pixel(cpl_size ypos, cpl_size xpos, Value value)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  cpl_error_code err = Error::throw_errors_with(
      hdrl_image_set_pixel, m_interface, coords.first, coords.second, value.v);
}

std::shared_ptr<Image>
Image::extract(pycpl_window w)
{
  hdrl_image* himg = Error::throw_errors_with(hdrl_image_extract, m_interface,
                                              EXPAND_WINDOW(w));
  Image im = Image(himg);
  return std::make_shared<Image>(im);
}

void
Image::accept(cpl_size ypos, cpl_size xpos)
{
  std::pair<cpl_size, cpl_size> coords = hdrl::core::cpl_coord(xpos, ypos);
  cpl_error_code err = Error::throw_errors_with(hdrl_image_accept, m_interface,
                                                coords.first, coords.second);
}

void
Image::accept_all()
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_accept_all, m_interface);
}

// Math
Value
Image::get_sqsum()
{
  hdrl_value val = Error::throw_errors_with(hdrl_image_get_sqsum, m_interface);
  return Value(val);
}

Value
Image::get_sum()
{
  hdrl_value val = Error::throw_errors_with(hdrl_image_get_sum, m_interface);
  return Value(val);
}

double
Image::get_stdev()
{
  return Error::throw_errors_with(hdrl_image_get_stdev, m_interface);
}

Value
Image::get_median()
{
  hdrl_value val = Error::throw_errors_with(hdrl_image_get_median, m_interface);
  return Value(val);
}

Value
Image::get_mean()
{
  hdrl_value val = Error::throw_errors_with(hdrl_image_get_mean, m_interface);
  return Value(val);
}

Value
Image::get_weighted_mean()
{
  hdrl_value val =
      Error::throw_errors_with(hdrl_image_get_weighted_mean, m_interface);
  return Value(val);
}

Value
Image::get_sigclip_mean(double kappa_low, double kappa_high, int niter)
{
  hdrl_value val = Error::throw_errors_with(
      hdrl_image_get_sigclip_mean, m_interface, kappa_low, kappa_high, niter);
  return Value(val);
}

Value
Image::get_minmax_mean(double nlow, double nhigh)
{
  hdrl_value val = Error::throw_errors_with(hdrl_image_get_minmax_mean,
                                            m_interface, nlow, nhigh);
  return Value(val);
}

Value
Image::get_mode(double histo_min, double histo_max, double bin_size,
                hdrl_mode_type method, cpl_size error_niter)
{
  hdrl_value val =
      Error::throw_errors_with(hdrl_image_get_mode, m_interface, histo_min,
                               histo_max, bin_size, method, error_niter);
  return Value(val);
}

std::shared_ptr<hdrl::core::Image>
Image::div_image_create(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  hdrl_image* himg = Error::throw_errors_with(hdrl_image_div_image_create,
                                              m_interface, other->ptr());
  // return Image(himg);
  return std::make_shared<Image>(Image(himg));
}

void
Image::div_image(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_div_image, m_interface, other->ptr());
}

void
Image::div_scalar(Value value)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_div_scalar, m_interface, value.v);
}

std::shared_ptr<Image>
Image::mul_image_create(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  hdrl_image* himg = Error::throw_errors_with(hdrl_image_mul_image_create,
                                              m_interface, other->ptr());
  return std::make_shared<Image>(Image(himg));
}

void
Image::mul_image(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_mul_image, m_interface, other->ptr());
}

void
Image::mul_scalar(Value value)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_mul_scalar, m_interface, value.v);
}

std::shared_ptr<Image>
Image::sub_image_create(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  hdrl_image* himg = Error::throw_errors_with(hdrl_image_sub_image_create,
                                              m_interface, other->ptr());
  return std::make_shared<Image>(Image(himg));
}

void
Image::sub_image(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_sub_image, m_interface, other->ptr());
}

void
Image::sub_scalar(Value value)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_sub_scalar, m_interface, value.v);
}

std::shared_ptr<Image>
Image::add_image_create(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  hdrl_image* himg = Error::throw_errors_with(hdrl_image_add_image_create,
                                              m_interface, other->ptr());
  return std::make_shared<Image>(Image(himg));
}

void
Image::add_image(const std::shared_ptr<Image> other)
{
  // Add null check before calling C function to prevent segfaults
  if (!other) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "other image cannot be None");
  }

  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_add_image, m_interface, other->ptr());
}

void
Image::add_scalar(Value value)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_add_scalar, m_interface, value.v);
}

std::shared_ptr<Image>
Image::pow_scalar_create(const Value exponent)
{
  hdrl_image* himg = Error::throw_errors_with(hdrl_image_pow_scalar_create,
                                              m_interface, exponent.v);
  return std::make_shared<Image>(Image(himg));
}

void
Image::pow_scalar(const Value exponent)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_pow_scalar, m_interface, exponent.v);
}

std::shared_ptr<Image>
Image::exp_scalar_create(const Value base)
{
  hdrl_image* himg = Error::throw_errors_with(hdrl_image_exp_scalar_create,
                                              m_interface, base.v);
  return std::make_shared<Image>(Image(himg));
}

void
Image::exp_scalar(const Value base)
{
  cpl_error_code err =
      Error::throw_errors_with(hdrl_image_exp_scalar, m_interface, base.v);
}

std::string
Image::dump_structure() const
{
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(hdrl_image_dump_structure, m_interface, stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
Image::dump(std::optional<pycpl_window> window) const
{
  pycpl_window w = window.value_or(pycpl_window::All);

  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = pycpl_window::All;
  }

  if (w != pycpl_window::All) {
    int dw = this->get_width();
    int dh = this->get_height();
    std::ostringstream oss;
    if ((w.llx < 0) || (w.lly < 0) || (w.urx >= dw) || (w.ury >= dh)) {
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      oss << " exceeds image bounds (0,0," << dw - 1 << "," << dh - 1 << ")";
      throw std::runtime_error(oss.str());
      // Currently causes a segfault for unknown reason.
      // throw hdrl::core::AccessOutOfRangeError(HDRL_ERROR_LOCATION,
      // oss.str());
    }
    if ((w.llx > w.urx) || (w.lly > w.ury)) {
      oss << "Invalid image window definition: ";
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      throw std::runtime_error(oss.str());
      // Currently causes a segfault for unknown reason.
      // throw hdrl::core::IllegalInputError(HDRL_ERROR_LOCATION, oss.str());
    }
  }

  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = pycpl_window::All;
  }
  if (w == pycpl_window::All) {
    w = {0, 0, this->get_width() - 1, this->get_height() - 1};
  }

  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(hdrl_image_dump_window, m_interface,
                           EXPAND_WINDOW(w), stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

/*bool
Image::operator==(const Image& other) const
{
  if (get_width() != other.get_width() || get_height() != other.get_height() ||
      get_type() != other.get_type()) {
    return false;
  }
  return std::memcmp(get_image(), other.get_image(), get_size() * pixel_size())
== 0;
}

bool
Image::operator!=(const Image& other) const
{
  return !operator==(other);
}*/

}  // namespace core
}  // namespace hdrl
