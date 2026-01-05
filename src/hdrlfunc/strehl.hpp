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

#ifndef PYHDRL_FUNC_STREHL_HPP_
#define PYHDRL_FUNC_STREHL_HPP_

#include <hdrl_parameter.h>
#include <hdrl_strehl.h>
#include <hdrl_types.h>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hdrlcore/image.hpp"
#include "hdrlcore/parameter.hpp"

namespace py = pybind11;

namespace hdrl
{
namespace func
{

using hdrl::core::Error;
using hdrl::core::Image;
using hdrl::core::Parameter;

class StrehlResult;

class Strehl
{
 public:
  Strehl();
  Strehl(double wavelength, double m1, double m2, double pixel_scale_x,
         double pixel_scale_y, double flux_radius, double bkg_radius_low,
         double bkg_radius_high);
  StrehlResult compute(std::shared_ptr<Image> himage);

  hdrl_parameter* ptr();
  void parameter_create_(void);

  // Getter methods
  double get_wavelength() const;
  double get_m1() const;
  double get_m2() const;
  double get_pixel_scale_x() const;
  double get_pixel_scale_y() const;
  double get_flux_radius() const;
  double get_bkg_radius_low() const;
  double get_bkg_radius_high() const;

 private:
  double wavelength_;
  double m1_;
  double m2_;
  double pixel_scale_x_;
  double pixel_scale_y_;
  double flux_radius_;
  double bkg_radius_low_;
  double bkg_radius_high_;

 protected:
  hdrl_parameter* m_interface;
};

// Python binding function
void bind_strehl(py::module& m);

class StrehlResult
{
 public:
  StrehlResult();
  StrehlResult(hdrl_strehl_result res);

  double get_strehl_value() const;
  double get_strehl_error() const;
  double get_star_x() const;
  double get_star_y() const;
  double get_star_peak() const;
  double get_star_peak_error() const;
  double get_star_flux() const;
  double get_star_flux_error() const;
  double get_star_background() const;
  double get_star_background_error() const;
  double get_computed_background_error() const;
  size_t get_nbackground_pixels() const;

 protected:
  hdrl_value strehl_value_;
  double star_x_;
  double star_y_;
  hdrl_value star_peak_;
  hdrl_value star_flux_;
  hdrl_value star_background_;
  double computed_background_error_;
  size_t nbackground_pixels_;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_STREHL_HPP_
