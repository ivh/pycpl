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

#ifndef PYHDRL_FUNC_FRINGE_HPP_
#define PYHDRL_FUNC_FRINGE_HPP_

#include <memory>

#include "hdrlcore/image.hpp"
#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/parameter.hpp"
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/collapse.hpp"

namespace hdrl
{
namespace func
{
using hdrl::core::Image;
using hdrl::core::ImageList;
using hdrl::core::Parameter;
using hdrl::core::pycpl_image;
using hdrl::core::pycpl_mask;
using hdrl::func::Collapse;

class Fringe
{
 public:
  // Constructors
  Fringe();

  // Result structures
  struct ComputeResult
  {
    std::shared_ptr<Image> master;
    hdrl::core::pycpl_image contrib_map;
    hdrl::core::pycpl_table qctable;
  };

  struct CorrectResult
  {
    hdrl::core::pycpl_table qctable;
  };

  // Compute methods
  ComputeResult compute(std::shared_ptr<ImageList> ilist_fringe,
                        hdrl::core::pycpl_imagelist ilist_obj,
                        pycpl_mask stat_mask, Collapse collapse_params);

  CorrectResult
  correct(std::shared_ptr<ImageList> ilist_fringe,
          hdrl::core::pycpl_imagelist ilist_obj, pycpl_mask stat_mask,
          std::shared_ptr<Image> masterfringe);

 protected:
  // No internal state needed for fringe functions
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_FRINGE_HPP_
