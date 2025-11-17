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

#include "cpldrs/apertures.hpp"

#include <cstdio>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{

Apertures::~Apertures()
{
  cpl::core::Error::throw_errors_with(cpl_apertures_delete, m_interface);
}

std::string
Apertures::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl::core::Error::throw_errors_with(cpl_apertures_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

Apertures::Apertures(cpl_apertures* to_steal) : m_interface(to_steal) {}

Apertures::Apertures(const cpl::core::ImageBase& inImage,
                     const cpl::core::ImageBase& lab)
{
  m_interface = cpl::core::Error::throw_errors_with(
      cpl_apertures_new_from_image, inImage.ptr(), lab.ptr());
}

size
Apertures::get_size() const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_size,
                                             m_interface);
}

double
Apertures::get_pos_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_pos_x,
                                             m_interface, ind);
}

double
Apertures::get_pos_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_pos_y,
                                             m_interface, ind);
}

double
Apertures::get_centroid_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_centroid_x,
                                             m_interface, ind);
}

double
Apertures::get_centroid_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_centroid_y,
                                             m_interface, ind);
}

size
Apertures::get_maxpos_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_maxpos_x,
                                             m_interface, ind);
}

size
Apertures::get_maxpos_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_maxpos_y,
                                             m_interface, ind);
}

size
Apertures::get_minpos_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_minpos_x,
                                             m_interface, ind);
}

size
Apertures::get_minpos_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_minpos_y,
                                             m_interface, ind);
}

size
Apertures::get_npix(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_npix,
                                             m_interface, ind);
}

size
Apertures::get_left(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_left,
                                             m_interface, ind);
}

size
Apertures::get_left_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_left_y,
                                             m_interface, ind);
}

size
Apertures::get_right(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_right,
                                             m_interface, ind);
}

size
Apertures::get_right_y(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_right_y,
                                             m_interface, ind);
}

size
Apertures::get_top_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_top_x,
                                             m_interface, ind);
}

size
Apertures::get_top(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_top, m_interface,
                                             ind);
}

size
Apertures::get_bottom_x(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_bottom_x,
                                             m_interface, ind);
}

size
Apertures::get_bottom(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_bottom,
                                             m_interface, ind);
}

double
Apertures::get_max(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_max, m_interface,
                                             ind);
}

double
Apertures::get_min(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_min, m_interface,
                                             ind);
}

double
Apertures::get_mean(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_mean,
                                             m_interface, ind);
}

double
Apertures::get_median(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_median,
                                             m_interface, ind);
}

double
Apertures::get_stdev(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_stdev,
                                             m_interface, ind);
}

double
Apertures::get_flux(size ind) const
{
  return cpl::core::Error::throw_errors_with(cpl_apertures_get_flux,
                                             m_interface, ind);
}

void
Apertures::sort_by_npix()
{
  cpl::core::Error::throw_errors_with(cpl_apertures_sort_by_npix, m_interface);
}

void
Apertures::sort_by_max()
{
  cpl::core::Error::throw_errors_with(cpl_apertures_sort_by_max, m_interface);
}

void
Apertures::sort_by_flux()
{
  cpl::core::Error::throw_errors_with(cpl_apertures_sort_by_flux, m_interface);
}

std::tuple<std::shared_ptr<Apertures>, size>
Apertures::extract(const cpl::core::ImageBase& inImage,
                   const cpl::core::Vector& sigmas)
{  //, size& pisigma) const {
  size pisigma_result;
  cpl_apertures* apertures_res = cpl::core::Error::throw_errors_with(
      cpl_apertures_extract, inImage.ptr(), sigmas.ptr(), &pisigma_result);
  return std::make_tuple(std::make_shared<Apertures>(apertures_res),
                         pisigma_result);
}

std::tuple<std::shared_ptr<Apertures>, size>
Apertures::extract_window(const cpl::core::ImageBase& inImage,
                          const cpl::core::Vector& sigmas,
                          cpl::core::Window area)
{  //, size& pisigma) const {
  size pisigma_result;
  cpl_apertures* apertures_res = cpl::core::Error::throw_errors_with(
      cpl_apertures_extract_window, inImage.ptr(), sigmas.ptr(),
      EXPAND_WINDOW(area), &pisigma_result);

  return std::make_tuple(std::make_shared<Apertures>(apertures_res),
                         pisigma_result);
}

std::shared_ptr<Apertures>
Apertures::extract_mask(const cpl::core::ImageBase& inImage,
                        const cpl::core::Mask& selection)
{
  cpl_apertures* apertures_res = cpl::core::Error::throw_errors_with(
      cpl_apertures_extract_mask, inImage.ptr(), selection.ptr());
  return std::make_shared<Apertures>(apertures_res);
}

std::shared_ptr<Apertures>
Apertures::extract_sigma(const cpl::core::ImageBase& inImage, double sigma)
{
  cpl_apertures* apertures_res = cpl::core::Error::throw_errors_with(
      cpl_apertures_extract_sigma, inImage.ptr(), sigma);
  return std::make_shared<Apertures>(apertures_res);
}

}  // namespace drs
}  // namespace cpl
