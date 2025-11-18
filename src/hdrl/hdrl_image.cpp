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

#include "hdrl/hdrl_image.hpp"

#include <cpl_image.h>

namespace cpl
{
namespace hdrl
{

using core::Error;

Image::Image(cpl::core::size nx, cpl::core::size ny)
{
  m_image = Error::throw_errors_with(hdrl_image_new, nx, ny);
}

Image::Image(std::shared_ptr<cpl::core::ImageBase> data,
             std::shared_ptr<cpl::core::ImageBase> error)
{
  cpl_image* data_ptr = nullptr;
  cpl_image* error_ptr = nullptr;

  if (data) {
    // We need to duplicate the images because hdrl_image_create takes ownership
    data_ptr = Error::throw_errors_with(cpl_image_duplicate, data->ptr());
  }

  if (error) {
    error_ptr = Error::throw_errors_with(cpl_image_duplicate, error->ptr());
  }

  m_image = Error::throw_errors_with(hdrl_image_create, data_ptr, error_ptr);
}

Image::Image(hdrl_image* to_steal) : m_image(to_steal) {}

Image::Image(const Image& other)
{
  if (other.m_image) {
    m_image = Error::throw_errors_with(hdrl_image_duplicate, other.m_image);
  } else {
    m_image = nullptr;
  }
}

Image::Image(Image&& other) noexcept : m_image(other.m_image)
{
  other.m_image = nullptr;
}

Image& Image::operator=(const Image& other)
{
  if (this != &other) {
    if (m_image) {
      hdrl_image_delete(m_image);
    }
    if (other.m_image) {
      m_image = Error::throw_errors_with(hdrl_image_duplicate, other.m_image);
    } else {
      m_image = nullptr;
    }
  }
  return *this;
}

Image& Image::operator=(Image&& other) noexcept
{
  if (this != &other) {
    if (m_image) {
      hdrl_image_delete(m_image);
    }
    m_image = other.m_image;
    other.m_image = nullptr;
  }
  return *this;
}

Image::~Image()
{
  if (m_image) {
    hdrl_image_delete(m_image);
  }
}

std::shared_ptr<cpl::core::ImageBase> Image::get_image()
{
  cpl_image* img = Error::throw_errors_with(hdrl_image_get_image, m_image);
  // hdrl_image_get_image returns a non-owning pointer, so we duplicate it
  cpl_image* dup = Error::throw_errors_with(cpl_image_duplicate, img);
  return cpl::core::ImageBase::make_image(dup);
}

std::shared_ptr<const cpl::core::ImageBase> Image::get_image() const
{
  cpl_image* img =
      Error::throw_errors_with(hdrl_image_get_image_const, m_image);
  cpl_image* dup = Error::throw_errors_with(cpl_image_duplicate, img);
  return cpl::core::ImageBase::make_image(dup);
}

std::shared_ptr<cpl::core::ImageBase> Image::get_error()
{
  cpl_image* img = Error::throw_errors_with(hdrl_image_get_error, m_image);
  cpl_image* dup = Error::throw_errors_with(cpl_image_duplicate, img);
  return cpl::core::ImageBase::make_image(dup);
}

std::shared_ptr<const cpl::core::ImageBase> Image::get_error() const
{
  cpl_image* img =
      Error::throw_errors_with(hdrl_image_get_error_const, m_image);
  cpl_image* dup = Error::throw_errors_with(cpl_image_duplicate, img);
  return cpl::core::ImageBase::make_image(dup);
}

std::shared_ptr<cpl::core::Mask> Image::get_mask()
{
  cpl_mask* mask = Error::throw_errors_with(hdrl_image_get_mask, m_image);
  // hdrl_image_get_mask returns a non-owning pointer, so we duplicate it
  cpl_mask* dup = Error::throw_errors_with(cpl_mask_duplicate, mask);
  return std::make_shared<cpl::core::Mask>(dup);
}

std::shared_ptr<const cpl::core::Mask> Image::get_mask() const
{
  cpl_mask* mask = Error::throw_errors_with(hdrl_image_get_mask_const, m_image);
  cpl_mask* dup = Error::throw_errors_with(cpl_mask_duplicate, mask);
  return std::make_shared<cpl::core::Mask>(dup);
}

std::pair<Value, bool> Image::get_pixel(cpl::core::size xpos,
                                         cpl::core::size ypos) const
{
  int is_rejected = 0;
  hdrl_value val =
      Error::throw_errors_with(hdrl_image_get_pixel, m_image, xpos, ypos, &is_rejected);
  return std::make_pair(Value(val), is_rejected != 0);
}

void Image::set_pixel(cpl::core::size xpos, cpl::core::size ypos,
                      const Value& value)
{
  hdrl_value val = value;
  Error::throw_errors_with(hdrl_image_set_pixel, m_image, xpos, ypos, val);
}

cpl::core::size Image::get_size_x() const
{
  return Error::throw_errors_with(hdrl_image_get_size_x, m_image);
}

cpl::core::size Image::get_size_y() const
{
  return Error::throw_errors_with(hdrl_image_get_size_y, m_image);
}

std::shared_ptr<Image> Image::extract(cpl::core::size llx, cpl::core::size lly,
                                       cpl::core::size urx,
                                       cpl::core::size ury) const
{
  hdrl_image* extracted =
      Error::throw_errors_with(hdrl_image_extract, m_image, llx, lly, urx, ury);
  return std::make_shared<Image>(extracted);
}

void Image::reject(cpl::core::size xpos, cpl::core::size ypos)
{
  Error::throw_errors_with(hdrl_image_reject, m_image, xpos, ypos);
}

void Image::reject_from_mask(std::shared_ptr<const cpl::core::Mask> mask)
{
  Error::throw_errors_with(hdrl_image_reject_from_mask, m_image, mask->ptr());
}

bool Image::is_rejected(cpl::core::size xpos, cpl::core::size ypos) const
{
  // Cast away const because the C API doesn't have a const version
  return Error::throw_errors_with(hdrl_image_is_rejected,
                                  const_cast<hdrl_image*>(m_image), xpos, ypos) != 0;
}

cpl::core::size Image::count_rejected() const
{
  return Error::throw_errors_with(hdrl_image_count_rejected, m_image);
}

void Image::accept(cpl::core::size xpos, cpl::core::size ypos)
{
  Error::throw_errors_with(hdrl_image_accept, m_image, xpos, ypos);
}

void Image::accept_all()
{
  Error::throw_errors_with(hdrl_image_accept_all, m_image);
}

}  // namespace hdrl
}  // namespace cpl
