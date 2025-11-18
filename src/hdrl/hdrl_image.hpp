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

#ifndef PYCPL_HDRL_IMAGE_HPP
#define PYCPL_HDRL_IMAGE_HPP

#include <memory>
#include <optional>

#include <hdrl_image.h>

#include "cplcore/error.hpp"
#include "cplcore/image.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/types.hpp"
#include "hdrl/hdrl_types.hpp"

namespace cpl
{
namespace hdrl
{

/**
 * @brief HDRL Image - an image with associated error and bad pixel mask
 *
 * An HDRL image consists of:
 * - A data image (cpl_image)
 * - An error image (cpl_image)
 * - A bad pixel mask (cpl_mask)
 */
class Image
{
 public:
  /**
   * @brief Create a new HDRL image with given dimensions
   * @param nx Width in pixels
   * @param ny Height in pixels
   */
  Image(cpl::core::size nx, cpl::core::size ny);

  /**
   * @brief Create an HDRL image from a CPL image and optional error image
   * @param data The data image
   * @param error The error image (optional, will be created if null)
   */
  Image(std::shared_ptr<cpl::core::ImageBase> data,
        std::shared_ptr<cpl::core::ImageBase> error = nullptr);

  /**
   * @brief Wrap an existing hdrl_image pointer (takes ownership)
   */
  explicit Image(hdrl_image* to_steal);

  /**
   * @brief Duplicate an existing HDRL image
   */
  Image(const Image& other);

  /**
   * @brief Move constructor
   */
  Image(Image&& other) noexcept;

  /**
   * @brief Copy assignment
   */
  Image& operator=(const Image& other);

  /**
   * @brief Move assignment
   */
  Image& operator=(Image&& other) noexcept;

  /**
   * @brief Destructor
   */
  ~Image();

  /**
   * @brief Get the underlying hdrl_image pointer (non-owning)
   */
  hdrl_image* ptr() { return m_image; }
  const hdrl_image* ptr() const { return m_image; }

  /**
   * @brief Get the data image
   */
  std::shared_ptr<cpl::core::ImageBase> get_image();
  std::shared_ptr<const cpl::core::ImageBase> get_image() const;

  /**
   * @brief Get the error image
   */
  std::shared_ptr<cpl::core::ImageBase> get_error();
  std::shared_ptr<const cpl::core::ImageBase> get_error() const;

  /**
   * @brief Get the bad pixel mask
   */
  std::shared_ptr<cpl::core::Mask> get_mask();
  std::shared_ptr<const cpl::core::Mask> get_mask() const;

  /**
   * @brief Get a pixel value with error at position (x, y)
   * @param xpos X position (1-indexed)
   * @param ypos Y position (1-indexed)
   * @return Value with data and error, and rejection status
   */
  std::pair<Value, bool> get_pixel(cpl::core::size xpos, cpl::core::size ypos) const;

  /**
   * @brief Set a pixel value with error at position (x, y)
   * @param xpos X position (1-indexed)
   * @param ypos Y position (1-indexed)
   * @param value Value with data and error
   */
  void set_pixel(cpl::core::size xpos, cpl::core::size ypos, const Value& value);

  /**
   * @brief Get image dimensions
   */
  cpl::core::size get_size_x() const;
  cpl::core::size get_size_y() const;

  /**
   * @brief Extract a sub-region from the image
   * @param llx Lower-left x coordinate (1-indexed)
   * @param lly Lower-left y coordinate (1-indexed)
   * @param urx Upper-right x coordinate (1-indexed)
   * @param ury Upper-right y coordinate (1-indexed)
   */
  std::shared_ptr<Image> extract(cpl::core::size llx, cpl::core::size lly,
                                  cpl::core::size urx, cpl::core::size ury) const;

  /**
   * @brief Reject (mark as bad) a pixel
   */
  void reject(cpl::core::size xpos, cpl::core::size ypos);

  /**
   * @brief Reject pixels based on a mask
   */
  void reject_from_mask(std::shared_ptr<const cpl::core::Mask> mask);

  /**
   * @brief Check if a pixel is rejected
   */
  bool is_rejected(cpl::core::size xpos, cpl::core::size ypos) const;

  /**
   * @brief Count rejected pixels
   */
  cpl::core::size count_rejected() const;

  /**
   * @brief Accept (un-mark) a pixel
   */
  void accept(cpl::core::size xpos, cpl::core::size ypos);

  /**
   * @brief Accept all pixels (clear bad pixel mask)
   */
  void accept_all();

 private:
  hdrl_image* m_image;
};

}  // namespace hdrl
}  // namespace cpl

#endif  // PYCPL_HDRL_IMAGE_HPP
