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

#include "hdrlfunc/fringe.hpp"

#include <cpl_image.h>
#include <cpl_imagelist.h>
#include <cpl_table.h>
#include <hdrl_fringe.h>
#include <hdrl_image.h>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

Fringe::Fringe()
{
  // No initialization needed for fringe functions
}

Fringe::ComputeResult
Fringe::compute(std::shared_ptr<ImageList> ilist_fringe,
                hdrl::core::pycpl_imagelist ilist_obj, pycpl_mask stat_mask,
                Collapse collapse_params)
{
  hdrl_image* master;
  cpl_image* contrib_map;
  cpl_table* qctable;

  // Check for null input
  if (!ilist_fringe) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "ilist_fringe cannot be None");
  }

  // Handle optional ilist_obj parameter
  cpl_imagelist* obj_list = nullptr;
  if (ilist_obj.il != nullptr) {
    obj_list = ilist_obj.il;
  }

  hdrl::core::Error::throw_errors_with(
      hdrl_fringe_compute, ilist_fringe.get()->ptr(),
      obj_list,               // const cpl_imagelist* ilist_obj
      stat_mask.m,            // const cpl_mask* stat_mask
      collapse_params.ptr(),  // const hdrl_parameter* collapse_params
      &master,                // hdrl_image** master
      &contrib_map,           // cpl_image** contrib_map
      &qctable                // cpl_table** qctable
  );

  // Create the result struct
  ComputeResult result = {std::make_shared<Image>(Image(master)),
                          hdrl::core::pycpl_image(contrib_map),
                          hdrl::core::pycpl_table(qctable)};

  return result;
}

Fringe::CorrectResult
Fringe::correct(std::shared_ptr<ImageList> ilist_fringe,
                hdrl::core::pycpl_imagelist ilist_obj, pycpl_mask stat_mask,
                std::shared_ptr<Image> masterfringe)
{
  cpl_table* qctable;

  // Check for null input
  if (!ilist_fringe) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "ilist_fringe cannot be None");
  }

  if (!masterfringe) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "masterfringe cannot be None");
  }

  // Handle optional ilist_obj parameter
  cpl_imagelist* obj_list = nullptr;
  if (ilist_obj.il != nullptr) {
    obj_list = ilist_obj.il;
  }

  hdrl::core::Error::throw_errors_with(
      hdrl_fringe_correct,
      ilist_fringe.get()->ptr(),  // hdrl_imagelist* ilist_fringe
      obj_list,                   // const cpl_imagelist* ilist_obj
      stat_mask.m,                // const cpl_mask* stat_mask
      masterfringe.get()->ptr(),  // const hdrl_image* masterfringe
      &qctable                    // cpl_table** qctable
  );

  // Create the result struct
  CorrectResult result = {hdrl::core::pycpl_table(qctable)};

  return result;
}

}  // namespace func
}  // namespace hdrl