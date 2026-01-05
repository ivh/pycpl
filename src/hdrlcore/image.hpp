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

#ifndef PYHDRL_CORE_IMAGE_HPP_
#define PYHDRL_CORE_IMAGE_HPP_

#include <memory>
#include <optional>
#include <string>

#include <cpl_error.h>
#include <cpl_image_bpm.h>
#include <cpl_type.h>
#include <hdrl_image.h>
#include <hdrl_mode_defs.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace core
{

class Image
{
 public:
  Image(pycpl_image i1, pycpl_image i2);
  Image(int width, int height);
  Image(hdrl_image* to_steal);
  Image(const Image& other)
      : m_interface(Error::throw_errors_with(hdrl_image_duplicate,
                                             other.m_interface)) {};

  ~Image() { hdrl_image_delete(m_interface); };

  int get_width() const;
  int get_height() const;
  int get_size() const;
  Value get_pixel(cpl_size ypos, cpl_size xpos);
  void set_pixel(cpl_size ypos, cpl_size xpos, Value value);
  // TODO: setup window...
  std::shared_ptr<Image> extract(pycpl_window w);
  // Image extract(cpl_size llx, cpl_size lly, cpl_size urx, cpl_size ury);
  void reject_value(cpl_value mode);
  void reject_from_mask(pycpl_mask map);
  void turn(int rot);
  void copy_into(std::shared_ptr<Image> other, cpl_size ypos, cpl_size xpos);
  void insert_into(pycpl_image image, std::optional<pycpl_image> error,
                   cpl_size ypos, cpl_size xpos);
  pycpl_mask get_mask();
  std::shared_ptr<Image> duplicate() const;
  cpl_error_code reject(cpl_size ypos, cpl_size xpos);
  bool is_rejected(cpl_size ypos, cpl_size xpos);
  cpl_size count_rejected();
  void accept(cpl_size ypos, cpl_size xpos);
  void accept_all();
  pycpl_image get_image();
  pycpl_image get_error();
  hdrl_image* ptr() const;
  // Dump functions
  std::string dump_structure() const;
  std::string dump(std::optional<pycpl_window> window) const;
  // Math
  Value get_sqsum();
  Value get_sum();
  double get_stdev();
  Value get_median();
  Value get_mean();
  Value get_weighted_mean();
  Value get_sigclip_mean(double kappa_low, double kappa_high, int niter);
  Value get_minmax_mean(double nlow, double nhigh);
  Value get_mode(double histo_min, double histo_max, double bin_size,
                 hdrl_mode_type method, cpl_size error_niter);

  std::shared_ptr<Image> div_image_create(const std::shared_ptr<Image> other);
  void div_image(const std::shared_ptr<Image> other);
  void div_scalar(Value value);

  std::shared_ptr<Image> mul_image_create(const std::shared_ptr<Image> other);
  void mul_image(const std::shared_ptr<Image> other);
  void mul_scalar(Value value);

  std::shared_ptr<Image> sub_image_create(const std::shared_ptr<Image> other);
  void sub_image(const std::shared_ptr<Image> other);
  void sub_scalar(Value value);

  std::shared_ptr<Image> add_image_create(const std::shared_ptr<Image> other);
  void add_image(const std::shared_ptr<Image> other);
  void add_scalar(Value value);

  std::shared_ptr<Image> pow_scalar_create(const Value exponent);
  void pow_scalar(const Value exponent);

  std::shared_ptr<Image> exp_scalar_create(const Value base);
  void exp_scalar(const Value base);

 protected:
  // Image(const Image& other);
  hdrl_image* m_interface;
};

}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_IMAGE_HPP_