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

#include "hdrlcore/imagelist.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <cpl_image_io.h>
#include <cpl_imagelist_io.h>
#include <hdrl_image.h>
#include <hdrl_imagelist_basic.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/image.hpp"

// Need to have an Error class similar to pycpl one ... so we can use
// Error::throw_errors_with

namespace hdrl
{
namespace core
{

ImageList::ImageList()
{
  m_interface = Error::throw_errors_with(hdrl_imagelist_new);
}

ImageList::ImageList(hdrl_imagelist* to_steal) : m_interface(to_steal)
{
  // copied approach from PyCPL
  for (int i = 0; i < hdrl_imagelist_get_size(to_steal); i++) {
    hdrl_image* item =
        Error::throw_errors_with(hdrl_imagelist_get, to_steal, i);
    // TODO: check that this approach works ok - extracted from PyCPL
    // Image::make_image/image_maker struct
    m_images.push_back(std::make_shared<Image>(item));
  }
}

ImageList::ImageList(pycpl_imagelist il1, pycpl_imagelist il2)
{
  m_interface = Error::throw_errors_with(hdrl_imagelist_new);
  // m_interface = Error::throw_errors_with(hdrl_imagelist_create, il1.il,
  // il2.il);
  for (int i = 0; i < cpl_imagelist_get_size(il1.il); i++) {
    cpl_image* data = Error::throw_errors_with(cpl_imagelist_get, il1.il, i);
    cpl_image* err = Error::throw_errors_with(cpl_imagelist_get, il2.il, i);
    Image item = Image(
        data, err);  // cpl_image_duplicate(data),cpl_image_duplicate(err));
    append(std::make_shared<Image>(item));
    // TODO: check that this approach works ok - extracted from PyCPL
    // Image::make_image/image_maker struct
    // m_images.push_back(std::make_shared<Image>(item));
  }
}

ImageList::ImageList(const ImageList& other)
{
  m_interface =
      Error::throw_errors_with(hdrl_imagelist_duplicate, other.m_interface);
  m_images = other.m_images;
}

ImageList::ImageList(ImageList&& other) noexcept
    : m_images(std::move(other.m_images)), m_interface(other.m_interface)
{
  other.m_interface = nullptr;
}

ImageList&
ImageList::operator=(const ImageList& other)
{
  if (this != &other) {
    hdrl_imagelist* duplicated =
        Error::throw_errors_with(hdrl_imagelist_duplicate, other.m_interface);
    if (m_interface) {
      Error::throw_errors_with(hdrl_imagelist_unwrap, m_interface);
    }
    m_interface = duplicated;
    m_images = other.m_images;
  }
  return *this;
}

ImageList&
ImageList::operator=(ImageList&& other) noexcept
{
  if (this != &other) {
    if (m_interface) {
      Error::throw_errors_with(hdrl_imagelist_unwrap, m_interface);
    }
    m_interface = other.m_interface;
    m_images = std::move(other.m_images);
    other.m_interface = nullptr;
  }
  return *this;
}

ImageList::~ImageList()
{
  if (m_interface) {
    // TODO: might we use hdrl_imagelist_unwrap here?
    Error::throw_errors_with(hdrl_imagelist_unwrap, m_interface);
  }
}

std::shared_ptr<Image>
ImageList::operator[](int index) const
{
  return m_images[index];
}

std::shared_ptr<Image>
ImageList::get_at(int index) const
{
  // return Image(Error::throw_errors_with(hdrl_imagelist_get, m_interface,
  // index));
  return m_images.at(index);
}

int
ImageList::get_size() const
{
  return Error::throw_errors_with(hdrl_imagelist_get_size, m_interface);
}

int
ImageList::get_size_x()
{
  return Error::throw_errors_with(hdrl_imagelist_get_size_x, m_interface);
}

int
ImageList::get_size_y()
{
  return Error::throw_errors_with(hdrl_imagelist_get_size_y, m_interface);
}

// This comment is from PyCPL ImageList::set;
// While I would like to use the cpl_imagelist_set function, its behaviour to
// deallocate the image at pos can lead to segfaults. Thus this is more of a
// workaround

void
ImageList::set(std::shared_ptr<Image> himg, int index)
{
  // cpl_error_code err = Error::throw_errors_with(hdrl_imagelist_set,
  // m_interface,himg.ptr(), index);
  int current_size = get_size();
  hdrl_imagelist* newList = hdrl_imagelist_new();
  for (int i = 0; i < current_size; i++) {
    if (i < index) {
      Error::throw_errors_with(hdrl_imagelist_set, newList,
                               hdrl_imagelist_get(m_interface, i), i);
    } else if (i == index) {
      Error::throw_errors_with(hdrl_imagelist_set, newList, himg.get()->ptr(),
                               index);
    } else {
      Error::throw_errors_with(hdrl_imagelist_set, newList,
                               hdrl_imagelist_get(m_interface, i), i);
    }
  }

  hdrl_imagelist_unwrap(m_interface);  // unwrap the old interface
  m_interface = newList;
  m_images[index] = himg;
}

void
ImageList::append(std::shared_ptr<Image> himg)
{
  // since himg is a shared point, we use get to retrieve the stored pointer and
  // then get its contents using the member function ptr
  Error::throw_errors_with(hdrl_imagelist_set, m_interface, himg.get()->ptr(),
                           get_size());
  m_images.push_back(himg);
}

// TODO: Implement 'insert' functionality
/*void
ImageList::insert(Image himg, int index)
{
            if ( index== self.get_size()) {  // Just append to the end
               self.append(himg);
            return;
            }
            else {

                m_interface->images */
/*hdrl_imagelist* himglist= hdrl_imagelist_new();
for (int i = 0; i < self.get_size(); i++) {
    if (i < index) {
        Error::throw_errors_with(hdrl_imagelist_set, newList,
               hdrl_imagelist_get(m_interface, i), i);
    } else {
    if (i == pos) {
        Error::throw_errors_with(hdrl_imagelist_set, newList, himg->m_interface,
                 i);
    }
    Error::throw_errors_with(hdrl_imagelistset, newList,
               hdrl_imagelist_get(m_interface, i), i + 1);
    }
}*/

//            }
//} */

std::shared_ptr<Image>
ImageList::pop(cpl_size index)
{
  std::shared_ptr<Image> poppedImage = m_images[index];
  Error::throw_errors_with(hdrl_imagelist_unset, m_interface, index);
  m_images.erase(m_images.begin() + index);
  // Error::throw_errors_with(hdrl_imagelist_unset, m_interface, index);
  return poppedImage;
}

void
ImageList::empty()
{
  int len = get_size();
  for (int i = 0; i < len; i++) {
    this->pop(0);
  }
}

std::shared_ptr<ImageList>
ImageList::duplicate()
{
  hdrl_imagelist* res =
      Error::throw_errors_with(hdrl_imagelist_duplicate, m_interface);
  return std::make_shared<ImageList>(res);
}

int
ImageList::is_consistent()
{
  return Error::throw_errors_with(hdrl_imagelist_is_consistent, m_interface);
}

std::string
ImageList::dump_structure() const
{
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(hdrl_imagelist_dump_structure, m_interface, stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
ImageList::dump(std::optional<hdrl::core::pycpl_window> window) const
{
  cpl_size nlist = this->get_size();
  hdrl::core::pycpl_window w = window.value_or(hdrl::core::pycpl_window::All);
  if (w != hdrl::core::pycpl_window::All) {
    std::ostringstream oss;
    if ((w.llx > w.urx) || (w.lly > w.ury)) {
      oss << "Invalid image window definition: ";
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      throw hdrl::core::IllegalInputError(HDRL_ERROR_LOCATION, oss.str());
    }
    for (cpl_size i = 0; i < nlist; i++) {
      int dw = this->get_at(i).get()->get_width();
      int dh = this->get_at(i).get()->get_height();
      if ((w.llx < 0) || (w.lly < 0) || (w.urx >= dw) || (w.ury >= dh)) {
        oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << ","
            << w.ury << ")";
        oss << " exceeds image at idx=" << i << " bounds (0,0," << dw - 1 << ","
            << dh - 1 << ")";
        throw hdrl::core::AccessOutOfRangeError(HDRL_ERROR_LOCATION, oss.str());
      }
    }
  }

  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = hdrl::core::pycpl_window::All;
  }

  // Find the min image bounds for all images if no window specified
  if (w == hdrl::core::pycpl_window::All && nlist > 0) {
    int minw = this->get_at(0).get()->get_width();
    int minh = this->get_at(0).get()->get_height();
    for (cpl_size i = 1; i < nlist; i++) {
      int dw = this->get_at(i).get()->get_width();
      int dh = this->get_at(i).get()->get_height();
      if (dw < minw) {
        minw = dw;
      }
      if (dh < minh) {
        minh = dh;
      }
    }
    w = {0, 0, minw - 1, minh - 1};
  }

  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(hdrl_imagelist_dump_window, m_interface,
                           EXPAND_WINDOW(w), stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

void
ImageList::add_imagelist(const ImageList& himglist)
{
  Error::throw_errors_with(hdrl_imagelist_add_imagelist, m_interface,
                           himglist.m_interface);
}

void
ImageList::sub_imagelist(const ImageList& himglist)
{
  Error::throw_errors_with(hdrl_imagelist_sub_imagelist, m_interface,
                           himglist.m_interface);
}

void
ImageList::mul_imagelist(const ImageList& himglist)
{
  Error::throw_errors_with(hdrl_imagelist_mul_imagelist, m_interface,
                           himglist.m_interface);
}

void
ImageList::div_imagelist(const ImageList& himglist)
{
  Error::throw_errors_with(hdrl_imagelist_div_imagelist, m_interface,
                           himglist.m_interface);
}

void
ImageList::add_image(const Image himg)
{
  Error::throw_errors_with(hdrl_imagelist_add_image, m_interface, himg.ptr());
}

void
ImageList::sub_image(const Image himg)
{
  Error::throw_errors_with(hdrl_imagelist_sub_image, m_interface, himg.ptr());
}

void
ImageList::mul_image(const Image himg)
{
  Error::throw_errors_with(hdrl_imagelist_mul_image, m_interface, himg.ptr());
}

void
ImageList::div_image(const Image himg)
{
  Error::throw_errors_with(hdrl_imagelist_div_image, m_interface, himg.ptr());
}

void
ImageList::add_scalar(Value val)
{
  Error::throw_errors_with(hdrl_imagelist_add_scalar, m_interface, val.v);
}

void
ImageList::sub_scalar(Value val)
{
  Error::throw_errors_with(hdrl_imagelist_sub_scalar, m_interface, val.v);
}

void
ImageList::mul_scalar(Value val)
{
  Error::throw_errors_with(hdrl_imagelist_mul_scalar, m_interface, val.v);
}

void
ImageList::div_scalar(Value val)
{
  Error::throw_errors_with(hdrl_imagelist_div_scalar, m_interface, val.v);
}

void
ImageList::pow_scalar(Value val)
{
  Error::throw_errors_with(hdrl_imagelist_pow_scalar, m_interface, val.v);
}

hdrl_imagelist*
ImageList::ptr()
{
  return m_interface;
}

}  // namespace core
}  // namespace hdrl
