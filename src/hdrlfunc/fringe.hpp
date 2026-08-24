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

#ifndef PYHDRL_FUNC_FRINGE_HPP_
#define PYHDRL_FUNC_FRINGE_HPP_

#include <memory>

#include "hdrlcore/image.hpp"
#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/collapse.hpp"

namespace hdrl
{
namespace func
{

class Fringe
{
 public:
  // Constructors
  Fringe();

  struct CorrectResult
  {
    hdrl::core::pycpl_table qctable;
  };

  // Compute methods
  void compute(std::shared_ptr<hdrl::core::ImageList> ilist_fringe,
               hdrl::core::pycpl_imagelist ilist_obj,
               hdrl::core::pycpl_mask stat_mask,
               hdrl::func::Collapse collapse_params);
  std::shared_ptr<hdrl::core::Image> get_master() const;
  hdrl::core::pycpl_image get_contrib_map() const;
  hdrl::core::pycpl_table get_qctable() const;

  CorrectResult
  correct(std::shared_ptr<hdrl::core::ImageList> ilist_fringe,
          hdrl::core::pycpl_imagelist ilist_obj,
          hdrl::core::pycpl_mask stat_mask,
          std::shared_ptr<hdrl::core::Image> masterfringe = nullptr);

 protected:
  void ensure_compute() const;
  std::shared_ptr<hdrl::core::Image> m_master;
  hdrl::core::pycpl_image m_contrib_map;
  hdrl::core::pycpl_table m_qctable;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_FRINGE_HPP_
