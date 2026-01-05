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

#ifndef PYHDRL_CORE_IMAGELIST_HPP_
#define PYHDRL_CORE_IMAGELIST_HPP_

#include <memory>
#include <optional>
#include <string>

#include <cpl_type.h>
#include <hdrl_imagelist.h>
#include <hdrl_imagelist_io.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/image.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace core
{

class ImageList
{
 public:
  ImageList();
  ImageList(hdrl_imagelist* to_steal);
  ImageList(pycpl_imagelist l1, pycpl_imagelist l2);
  ImageList(const ImageList& other)
      : m_interface(Error::throw_errors_with(hdrl_imagelist_duplicate,
                                             other.m_interface)) {};
  ~ImageList();
  std::shared_ptr<Image> operator[](int index) const;

  int get_size() const;
  int get_size_x();
  int get_size_y();
  std::shared_ptr<Image> get_at(int index) const;
  void set(std::shared_ptr<Image> himg, int index);
  void append(std::shared_ptr<Image> image);
  std::shared_ptr<Image> pop(cpl_size index);
  void empty();
  int is_consistent();
  std::shared_ptr<ImageList> duplicate();
  std::string dump_structure() const;
  std::string dump(std::optional<pycpl_window> window) const;

  void add_imagelist(const ImageList& himglist);
  void sub_imagelist(const ImageList& himglist);
  void mul_imagelist(const ImageList& himglist);
  void div_imagelist(const ImageList& himglist);

  void add_image(const Image himg);
  void sub_image(const Image himg);
  void mul_image(const Image himg);
  void div_image(const Image himg);

  void add_scalar(Value val);
  void sub_scalar(Value val);
  void mul_scalar(Value val);
  void div_scalar(Value val);
  void pow_scalar(Value val);

  hdrl_imagelist* ptr();

  // hdrl::core::pycpl_image get_image();
  // hdrl::core::pycpl_image get_error();
 protected:
  std::vector<std::shared_ptr<Image>> m_images;
  hdrl_imagelist* m_interface;
};

}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_IMAGELIST_HPP_