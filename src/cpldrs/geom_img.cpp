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

#include "cpldrs/geom_img.hpp"

#include <tuple>

#include <cpl_bivector.h>
#include <cpl_image.h>
#include <cpl_vector.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{
namespace geom
{

std::tuple<cpl::core::Bivector, cpl::core::Vector>
img_offset_fine(const cpl::core::ImageList& ilist,
                const cpl::core::Bivector& estimates,
                const cpl::core::Bivector& anchors, size s_hx, size s_hy,
                size m_hx, size m_hy)
{
  // create pointer to pass into function
  cpl_vector* correl_ptr = cpl_vector_new(ilist.size());
  cpl_bivector* offsets = cpl::core::Error::throw_errors_with(
      cpl_geom_img_offset_fine, ilist.ptr(), estimates.ptr().get(),
      anchors.ptr().get(), s_hx, s_hy, m_hx, m_hy,
      correl_ptr);  // Get offsets

  // Create correl return object using passed pointer
  cpl::core::Vector correl_return(correl_ptr);
  return std::make_tuple(cpl::core::Bivector(offsets), correl_return);
}

std::tuple<std::shared_ptr<cpl::core::ImageBase>,
           std::shared_ptr<cpl::core::ImageBase>, double, double>
img_offset_saa(const cpl::core::ImageList& ilist,
               const cpl::core::Bivector& offs, cpl_kernel kernel, size rejmin,
               size rejmax, cpl_geom_combine union_flag)
{
  // Store output variables ppos_x and ppos_y
  double ppos_x;
  double ppos_y;
  // Get the two images first
  cpl_image** res = cpl::core::Error::throw_errors_with(
      cpl_geom_img_offset_saa, ilist.ptr(), offs.ptr().get(), kernel, rejmin,
      rejmax, union_flag, &ppos_x, &ppos_y);
  // Generate different image objects with different templates depending on
  // input type
  std::shared_ptr<cpl::core::ImageBase> combined =
      cpl::core::ImageBase::make_image(res[0]);
  std::shared_ptr<cpl::core::ImageBase> contribution =
      cpl::core::ImageBase::make_image(res[1]);

  // Return values are CPL based where bottom left is (1,1): take 1 away from
  // both to make it pycpl based
  ppos_x -= 1;  // Can't use cpl_to_coord from ../cplcore/coords.hpp
                // unfortunately due to it using different data types
  ppos_y -= 1;
  // Free the pointer array as we have captured the contents
  free(res);
  // Convert them to pycpl images
  return std::make_tuple(combined, contribution, ppos_x, ppos_y);
}

std::tuple<std::shared_ptr<cpl::core::ImageBase>,
           std::shared_ptr<cpl::core::ImageBase>, std::optional<size>>
img_offset_combine(const cpl::core::ImageList& ilist,
                   const cpl::core::Bivector& offs, size s_hx, size s_hy,
                   size m_hx, size m_hy, size min_rej, size max_rej,
                   cpl_geom_combine union_flag, bool refine,
                   std::optional<cpl::core::Bivector> anchors,
                   std::optional<cpl::core::Vector> sigmas)
{
  size pisigma_result;
  cpl_bivector* anchors_ptr =
      anchors.has_value() ? anchors.value().ptr().release() : nullptr;
  const cpl_vector* sigmas_ptr =
      sigmas.has_value() ? sigmas.value().ptr() : nullptr;

  // Get the two images first
  cpl_image** res = cpl::core::Error::throw_errors_with(
      cpl_geom_img_offset_combine, ilist.ptr(), offs.ptr().get(), refine,
      anchors_ptr, sigmas_ptr, &pisigma_result, s_hx, s_hy, m_hx, m_hy, min_rej,
      max_rej, union_flag);
  // Generate different image objects with different templates depending on
  // input type
  // Generate different image objects with different templates depending on
  // input type
  std::shared_ptr<cpl::core::ImageBase> combined =
      cpl::core::ImageBase::make_image(res[0]);
  std::shared_ptr<cpl::core::ImageBase> contribution =
      cpl::core::ImageBase::make_image(res[1]);

  cpl_bivector_unwrap_vectors(anchors_ptr);

  // Free the pointer array as we have captured the contents
  free(res);
  // Package all results in a tuple
  if (sigmas.has_value()) {
    return std::make_tuple(combined, contribution, pisigma_result);
  } else {
    return std::make_tuple(combined, contribution, std::nullopt);
  }
}

}  // namespace geom
}  // namespace drs
}  // namespace cpl