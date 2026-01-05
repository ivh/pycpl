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

#include "strehl.hpp"

#include <cstddef>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::InvalidTypeError;

Strehl::Strehl()
{
  wavelength_ = 0.0;
  m1_ = 0.0;
  m2_ = 0.0;
  pixel_scale_x_ = 0.0;
  pixel_scale_y_ = 0.0;
  flux_radius_ = 0.0;
  bkg_radius_low_ = 0.0;
  bkg_radius_high_ = 0.0;
}

Strehl::Strehl(double wavelength, double m1, double m2, double pixel_scale_x,
               double pixel_scale_y, double flux_radius, double bkg_radius_low,
               double bkg_radius_high)
{
  wavelength_ = wavelength;
  m1_ = m1;
  m2_ = m2;
  pixel_scale_x_ = pixel_scale_x;
  pixel_scale_y_ = pixel_scale_y;
  flux_radius_ = flux_radius;
  bkg_radius_low_ = bkg_radius_low;
  bkg_radius_high_ = bkg_radius_high;
}

// Compute Strehl
StrehlResult
Strehl::compute(std::shared_ptr<Image> himage)
{
  if (himage == nullptr) {
    throw InvalidTypeError(HDRL_ERROR_LOCATION,
                           "Image provided to Strehl compute must not be None");
  }

  this->parameter_create_();
  hdrl_strehl_result res = Error::throw_errors_with(
      hdrl_strehl_compute, himage.get()->ptr(), m_interface);

  return StrehlResult(res);
}

void
Strehl::parameter_create_(void)
{
  m_interface = hdrl::core::Error::throw_errors_with(
      hdrl_strehl_parameter_create, wavelength_, m1_, m2_, pixel_scale_x_,
      pixel_scale_y_, flux_radius_, bkg_radius_low_, bkg_radius_high_);
}

// Getter methods
double
Strehl::get_wavelength() const
{
  return wavelength_;
}

double
Strehl::get_m1() const
{
  return m1_;
}

double
Strehl::get_m2() const
{
  return m2_;
}

double
Strehl::get_pixel_scale_x() const
{
  return pixel_scale_x_;
}

double
Strehl::get_pixel_scale_y() const
{
  return pixel_scale_y_;
}

double
Strehl::get_flux_radius() const
{
  return flux_radius_;
}

double
Strehl::get_bkg_radius_low() const
{
  return bkg_radius_low_;
}

double
Strehl::get_bkg_radius_high() const
{
  return bkg_radius_high_;
}

hdrl_parameter*
Strehl::ptr()
{
  return m_interface;
}

StrehlResult::StrehlResult()
{
  strehl_value_ = {0.0, 0.0};
  star_x_ = 0.0;
  star_y_ = 0.0;
  star_peak_ = {0.0, 0.0};
  star_flux_ = {0.0, 0.0};
  star_background_ = {0.0, 0.0};
  computed_background_error_ = 0.0;
  nbackground_pixels_ = 0;
}

StrehlResult::StrehlResult(hdrl_strehl_result res)
{
  strehl_value_ = res.strehl_value;
  star_x_ = res.star_x;
  star_y_ = res.star_y;
  star_peak_ = res.star_peak;
  star_flux_ = res.star_flux;
  star_background_ = res.star_background;
  computed_background_error_ = res.computed_background_error;
  nbackground_pixels_ = res.nbackground_pixels;
}

// Getter methods
double
StrehlResult::get_strehl_value() const
{
  return strehl_value_.data;
}

double
StrehlResult::get_strehl_error() const
{
  return strehl_value_.error;
}

double
StrehlResult::get_star_x() const
{
  return star_x_;
}

double
StrehlResult::get_star_y() const
{
  return star_y_;
}

double
StrehlResult::get_star_peak() const
{
  return star_peak_.data;
}

double
StrehlResult::get_star_peak_error() const
{
  return star_peak_.error;
}

double
StrehlResult::get_star_flux() const
{
  return star_flux_.data;
}

double
StrehlResult::get_star_flux_error() const
{
  return star_flux_.error;
}

double
StrehlResult::get_star_background() const
{
  return star_background_.data;
}

double
StrehlResult::get_star_background_error() const
{
  return star_background_.error;
}

double
StrehlResult::get_computed_background_error() const
{
  return computed_background_error_;
}

size_t
StrehlResult::get_nbackground_pixels() const
{
  return nbackground_pixels_;
}

}  // namespace func
}  // namespace hdrl
