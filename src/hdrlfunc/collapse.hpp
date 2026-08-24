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

#ifndef PYHDRL_FUNC_COLLAPSE_HPP_
#define PYHDRL_FUNC_COLLAPSE_HPP_

#include <memory>
#include <string>

#include <cpl_type.h>
#include <hdrl_mode_defs.h>
#include <hdrl_parameter.h>
#include <pybind11/pybind11.h>

#include "hdrlcore/imagelist.hpp"


namespace py = pybind11;

namespace hdrl
{
namespace func
{

using hdrl::core::ImageList;

// Parent class
class Collapse
{
 public:
  // Different constructors for each mode
  // Mean, WeightedMean and Median
  Collapse(std::string m);
  // MinMax
  Collapse(double nlow, double nhigh);
  // Mode
  Collapse(double histo_min, double histo_max, double bin_size,
           hdrl_mode_type mode_method, cpl_size error_niter);
  // Sigclip
  Collapse(double kappa_low, double kappa_high, int niter);
  hdrl_parameter* ptr();
  // static functions that are called from hdrl.core.ImageList bindings
  // static py::object collapse(ImageList himlist, collapse_type collapse);
  py::object compute(std::shared_ptr<ImageList> himlist);
  static py::object collapse_mean(std::shared_ptr<ImageList> himlist);
  static py::object collapse_weighted_mean(std::shared_ptr<ImageList> himlist);
  static py::object collapse_median(std::shared_ptr<ImageList> himlist);
  static py::object
  collapse_sigclip(std::shared_ptr<ImageList> himlist, double kappa_low,
                   double kappa_high, int niter);
  static py::object collapse_minmax(std::shared_ptr<ImageList> himlist,
                                    double nlow, double nhigh);
  static py::object
  collapse_mode(std::shared_ptr<ImageList> himlist, double histo_min,
                double histo_max, double bin_size, hdrl_mode_type mode_method,
                cpl_size error_niter);
  std::string get_mode();
  // mode == minmax
  double get_nhigh();
  double get_nlow();
  // mode == mode
  double get_histo_min();
  double get_histo_max();
  double get_bin_size();
  hdrl_mode_type get_method();
  cpl_size get_error_niter();
  // mode == sigclip
  double get_kappa_high();
  double get_kappa_low();
  int get_niter();

 protected:
  hdrl_parameter* m_interface;
  std::string mode;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_COLLAPSE_HPP_