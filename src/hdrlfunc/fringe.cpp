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

using hdrl::core::Image;
using hdrl::core::ImageList;
using hdrl::core::pycpl_image;
using hdrl::core::pycpl_imagelist;
using hdrl::core::pycpl_mask;
using hdrl::core::pycpl_table;
using hdrl::func::Collapse;

Fringe::Fringe()
{
  // No initialization needed for fringe functions
}

void
Fringe::compute(std::shared_ptr<ImageList> ilist_fringe,
                pycpl_imagelist ilist_obj, pycpl_mask stat_mask,
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

  m_master = std::make_shared<Image>(Image(master));
  m_contrib_map = pycpl_image(contrib_map);
  m_qctable = pycpl_table(qctable);
}

Fringe::CorrectResult
Fringe::correct(std::shared_ptr<ImageList> ilist_fringe,
                pycpl_imagelist ilist_obj, pycpl_mask stat_mask,
                std::shared_ptr<Image> masterfringe)
{
  cpl_table* qctable;

  // Check for null input
  if (!ilist_fringe) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "ilist_fringe cannot be None");
  }

  if (!masterfringe) {
    ensure_compute();
    masterfringe = m_master;
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
  CorrectResult result = {pycpl_table(qctable)};

  return result;
}

void
Fringe::ensure_compute() const
{
  if (!m_master) {
    throw hdrl::core::NullInputError(
        HDRL_ERROR_LOCATION,
        "Fringe results not available. Call compute() first.");
  }
}

std::shared_ptr<Image>
Fringe::get_master() const
{
  ensure_compute();
  return m_master;
}

pycpl_image
Fringe::get_contrib_map() const
{
  ensure_compute();
  return m_contrib_map;
}

pycpl_table
Fringe::get_qctable() const
{
  ensure_compute();
  return m_qctable;
}

}  // namespace func
}  // namespace hdrl
