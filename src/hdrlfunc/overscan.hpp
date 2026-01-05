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

#ifndef PYHDRL_FUNC_OVERSCAN_HPP_
#define PYHDRL_FUNC_OVERSCAN_HPP_

#include <memory>
#include <optional>
#include <string>

#include <hdrl_overscan.h>
#include <hdrl_parameter.h>
#include <hdrl_utils.h>

#include <pybind11/pytypes.h>

#include "hdrlcore/image.hpp"
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/collapse.hpp"

namespace py = pybind11;

namespace hdrl
{
namespace func
{

class Overscan
{
 public:
  Overscan(const std::string& direction, double ccd_ron, int box_hsize,
           Collapse collapse, const hdrl::core::pycpl_window& region);

  py::object compute(std::shared_ptr<hdrl::core::Image> input_image);
  py::object
  correct(std::shared_ptr<hdrl::core::Image> input_image,
          std::optional<hdrl::core::pycpl_window> region = std::nullopt);
  hdrl_parameter* ptr();

  // Getters for Python
  std::string get_direction() const { return m_direction; }

  double get_ccd_ron() const { return m_ccd_ron; }

  int get_box_hsize() const { return m_box_hsize; }

  hdrl::core::pycpl_window get_overscan_region() const;

 private:
  hdrl_parameter* m_interface = nullptr;
  hdrl_overscan_compute_result* m_result = nullptr;

  // Save constructor args
  std::string m_direction;
  double m_ccd_ron;
  int m_box_hsize;
  hdrl::core::pycpl_window m_overscan_region;
  hdrl_direction m_corr_dir;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_OVERSCAN_HPP_