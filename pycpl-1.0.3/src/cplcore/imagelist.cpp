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

#include "cplcore/imagelist.hpp"

#include <cstdio>
#include <sstream>

#include <cpl_imagelist.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{
cpl_size
ImageList::size() const
{
  return Error::throw_errors_with(cpl_imagelist_get_size, m_interface);
}

ImageList::ImageList()
    : m_interface(Error::throw_errors_with(cpl_imagelist_new))
{
}

ImageList::ImageList(std::vector<std::shared_ptr<ImageBase>> images)
    : m_interface(Error::throw_errors_with(cpl_imagelist_new))
{
  for (std::shared_ptr<ImageBase> i : images) {
    append(i);
  }
}

ImageList::ImageList(cpl_imagelist* to_steal) : m_interface(to_steal)
{
  for (int i = 0; i < cpl_imagelist_get_size(to_steal); i++) {
    cpl_image* item = Error::throw_errors_with(cpl_imagelist_get, to_steal, i);
    m_images.push_back(ImageBase::make_image(item));
  }
}

ImageList::~ImageList()
{
  cpl_imagelist_unwrap(m_interface);
  // Remove references from vector
}

std::shared_ptr<ImageBase>
ImageList::operator[](int index) const
{
  return m_images[index];
}

std::string
ImageList::dump_structure() const
{
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(cpl_imagelist_dump_structure, m_interface, stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
ImageList::dump(std::optional<Window> window) const
{
  cpl_size nlist = this->size();
  cpl::core::Window w = window.value_or(cpl::core::Window::All);
  if (w != cpl::core::Window::All) {
    std::ostringstream oss;
    if ((w.llx > w.urx) || (w.lly > w.ury)) {
      oss << "Invalid image window definition: ";
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION, oss.str());
    }
    for (cpl_size i = 0; i < nlist; i++) {
      int dw = this->get_at(i)->get_width();
      int dh = this->get_at(i)->get_height();
      if ((w.llx < 0) || (w.lly < 0) || (w.urx >= dw) || (w.ury >= dh)) {
        oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << ","
            << w.ury << ")";
        oss << " exceeds image at idx=" << i << " bounds (0,0," << dw - 1 << ","
            << dh - 1 << ")";
        throw cpl::core::AccessOutOfRangeError(PYCPL_ERROR_LOCATION, oss.str());
      }
    }
  }

  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = cpl::core::Window::All;
  }

  // Find the min image bounds for all images if no window specified
  if (w == cpl::core::Window::All && nlist > 0) {
    int minw = this->get_at(0)->get_width();
    int minh = this->get_at(0)->get_height();
    for (cpl_size i = 1; i < nlist; i++) {
      int dw = this->get_at(i)->get_width();
      int dh = this->get_at(i)->get_height();
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
  Error::throw_errors_with(cpl_imagelist_dump_window, m_interface,
                           EXPAND_WINDOW(w), stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

void
ImageList::append(std::shared_ptr<ImageBase> image)
{
  Error::throw_errors_with(cpl_imagelist_set, m_interface,
                           image.get()->m_interface, size());
  m_images.push_back(image);
}

std::shared_ptr<ImageBase>
ImageList::get_at(int index) const
{
  return m_images.at(index);
}

void
ImageList::multiply_image(const ImageBase* img)
{
  Error::throw_errors_with(cpl_imagelist_multiply_image, m_interface,
                           img->m_interface);
}

void
ImageList::subtract_image(const ImageBase* img)
{
  Error::throw_errors_with(cpl_imagelist_subtract_image, m_interface,
                           img->m_interface);
}

void
ImageList::add_image(const ImageBase* img)
{
  Error::throw_errors_with(cpl_imagelist_add_image, m_interface,
                           img->m_interface);
}

void
ImageList::divide_image(const ImageBase* img)
{
  Error::throw_errors_with(cpl_imagelist_divide_image, m_interface,
                           img->m_interface);
}

void
ImageList::power(double exponent)
{
  Error::throw_errors_with(cpl_imagelist_power, m_interface, exponent);
}

void
ImageList::logarithm(double base)
{
  Error::throw_errors_with(cpl_imagelist_logarithm, m_interface, base);
}

void
ImageList::multiply_scalar(double factor)
{
  Error::throw_errors_with(cpl_imagelist_multiply_scalar, m_interface, factor);
}

void
ImageList::add_scalar(double addend)
{
  Error::throw_errors_with(cpl_imagelist_add_scalar, m_interface, addend);
}

void
ImageList::subtract_scalar(double subtrahend)
{
  Error::throw_errors_with(cpl_imagelist_subtract_scalar, m_interface,
                           subtrahend);
}

void
ImageList::divide_scalar(double divisor)
{
  Error::throw_errors_with(cpl_imagelist_divide_scalar, m_interface, divisor);
}

void
ImageList::exponential(double base)
{
  Error::throw_errors_with(cpl_imagelist_exponential, m_interface, base);
}

std::shared_ptr<ImageBase>
ImageList::collapse_create() const
{
  cpl_image* res =
      Error::throw_errors_with(cpl_imagelist_collapse_create, m_interface);
  return ImageBase::make_image(res);
}

std::shared_ptr<ImageBase>
ImageList::collapse_median_create() const
{
  cpl_image* res = Error::throw_errors_with(
      cpl_imagelist_collapse_median_create, m_interface);
  return ImageBase::make_image(res);
}

std::shared_ptr<ImageBase>
ImageList::collapse_minmax_create(cpl_size nlow, cpl_size nhigh) const
{
  cpl_image* res = Error::throw_errors_with(
      cpl_imagelist_collapse_minmax_create, m_interface, nlow, nhigh);
  return ImageBase::make_image(res);
}

void
ImageList::multiply(const ImageList& in2)
{
  Error::throw_errors_with(cpl_imagelist_multiply, m_interface,
                           in2.m_interface);
}

void
ImageList::add(const ImageList& in2)
{
  Error::throw_errors_with(cpl_imagelist_add, m_interface, in2.m_interface);
}

void
ImageList::divide(const ImageList& in2)
{
  Error::throw_errors_with(cpl_imagelist_divide, m_interface, in2.m_interface);
}

void
ImageList::subtract(const ImageList& in2)
{
  Error::throw_errors_with(cpl_imagelist_subtract, m_interface,
                           in2.m_interface);
}

void
ImageList::normalise(cpl_norm mode)
{
  Error::throw_errors_with(cpl_imagelist_normalise, m_interface, mode);
}

void
ImageList::threshold(double lo_cut, double hi_cut, double assign_lo_cut,
                     double assign_hi_cut)
{
  Error::throw_errors_with(cpl_imagelist_threshold, m_interface, lo_cut, hi_cut,
                           assign_lo_cut, assign_hi_cut);
}

std::pair<std::shared_ptr<ImageBase>, std::shared_ptr<ImageBase>>
ImageList::collapse_sigclip_create(double kappalow, double kappahigh,
                                   double keepfrac, cpl_collapse_mode mode)
{
  const cpl_image* first_im = cpl_imagelist_get(m_interface, 0);
  // Get the dimensions of the first image, since its whats expected of contrib
  const cpl_size contrib_nx = cpl_image_get_size_x(first_im);
  const cpl_size contrib_ny = cpl_image_get_size_y(first_im);
  // contrib must be of type CPL_TYPE_INT
  cpl_image* contrib_ptr = cpl_image_new(contrib_nx, contrib_ny, CPL_TYPE_INT);
  try {
    cpl_image* res = Error::throw_errors_with(
        cpl_imagelist_collapse_sigclip_create, m_interface, kappalow, kappahigh,
        keepfrac, mode, contrib_ptr);
    return {ImageBase::make_image(res), ImageBase::make_image(contrib_ptr)};
  }
  catch (const std::exception& e)  // Catch to delete the contrib, then rethrow
  {
    cpl_image_delete(contrib_ptr);
    throw e;
  }
}

std::shared_ptr<ImageList>
ImageList::swap_axis_create(cpl_swap_axis mode) const
{
  cpl_imagelist* res = Error::throw_errors_with(cpl_imagelist_swap_axis_create,
                                                m_interface, mode);
  return std::make_shared<ImageList>(res);
}

std::shared_ptr<ImageList>
ImageList::duplicate()
{
  cpl_imagelist* res =
      Error::throw_errors_with(cpl_imagelist_duplicate, m_interface);
  return std::make_shared<ImageList>(res);
}

void
ImageList::save(const std::filesystem::path& filename, const PropertyList& pl,
                unsigned mode, cpl_type type) const
{
  Error::throw_errors_with(cpl_imagelist_save, m_interface, filename.c_str(),
                           type, pl.ptr().get(), mode);
}

bool
ImageList::is_uniform()
{
  int res = Error::throw_errors_with(cpl_imagelist_is_uniform, m_interface);
  // 0=uniform, 1=empty, positive=non-uniform, negative=error
  if (res == 0) {
    return true;
  } else if (res == 1) {
    throw cpl::core::IllegalInputError(
        PYCPL_ERROR_LOCATION,
        "ImageList is empty");  // Or should we throw an error for being an
                                // empty list?
  } else {
    return false;
  }
}

void
ImageList::insert(std::shared_ptr<ImageBase> img, long pos)
{
  if (pos == size()) {  // Just append to the end
    append(img);
    return;
  }
  // Otherwise we have to do this lovely method I want to replace: move image
  // references in a new list including the new one
  cpl_imagelist* newList = cpl_imagelist_new();
  for (int i = 0; i < size(); i++) {
    if (i < pos) {
      Error::throw_errors_with(cpl_imagelist_set, newList,
                               cpl_imagelist_get(m_interface, i), i);
    } else {
      if (i == pos) {
        Error::throw_errors_with(cpl_imagelist_set, newList, img->m_interface,
                                 i);
      }
      Error::throw_errors_with(cpl_imagelist_set, newList,
                               cpl_imagelist_get(m_interface, i), i + 1);
    }
  }
  cpl_imagelist_unwrap(m_interface);  // unwrap the old interface
  m_interface = newList;
  // Error::throw_errors_with(cpl_imagelist_set, m_interface, img->m_interface,
  // pos);
  m_images.insert(m_images.begin() + pos, img);
}

// While I would like to use the cpl_imagelist_set function, its behaviour to
// deallocate the image at pos can lead to segfaults. Thus this is more of a
// workaround
void
ImageList::set(std::shared_ptr<ImageBase> img, long pos)
{
  cpl_imagelist* newList = cpl_imagelist_new();
  for (int i = 0; i < size(); i++) {
    if (i < pos) {
      Error::throw_errors_with(cpl_imagelist_set, newList,
                               cpl_imagelist_get(m_interface, i), i);
    } else if (i == pos) {
      Error::throw_errors_with(cpl_imagelist_set, newList, img->m_interface,
                               pos);
    } else {
      Error::throw_errors_with(cpl_imagelist_set, newList,
                               cpl_imagelist_get(m_interface, i), i);
    }
  }

  cpl_imagelist_unwrap(m_interface);  // unwrap the old interface
  m_interface = newList;
  m_images[pos] = img;
}

std::shared_ptr<ImageBase>
ImageList::pop(long pos)
{
  // Make a copy to return to prevent the reference count to immediately drop to
  // 0 before python gc
  std::shared_ptr<ImageBase> poppedImage = m_images[pos];
  Error::throw_errors_with(cpl_imagelist_unset, m_interface, pos);
  m_images.erase(m_images.begin() + pos);  // Remove the shared ptr
  return poppedImage;
}

std::shared_ptr<ImageList>
ImageList::cast(cpl_type type)
{
  cpl_imagelist* copy = cpl_imagelist_new();
  Error::throw_errors_with(cpl_imagelist_cast, copy, m_interface, type);
  return std::make_shared<ImageList>(copy);
}

void
ImageList::empty()
{
  long len = size();
  for (int i = 0; i < len; i++) {
    this->pop(0);
  }
}

std::shared_ptr<ImageList>
load_imagelist(const std::filesystem::path& name, cpl_type type, size position,
               Window area)
{
  cpl_imagelist* image_list;

  if (area == Window::All) {
    image_list = Error::throw_errors_with(cpl_imagelist_load, name.c_str(),
                                          type, position);
    return std::make_shared<ImageList>(Error::throw_errors_with(
        cpl_imagelist_load, name.c_str(), type, position));
  } else {
    image_list =
        Error::throw_errors_with(cpl_imagelist_load_window, name.c_str(), type,
                                 position, EXPAND_WINDOW(area));
  }
  return std::make_shared<ImageList>(image_list);
}

std::shared_ptr<ImageBase>
image_from_accepted(const ImageList& in)
{
  return ImageBase::make_image(
      Error::throw_errors_with(cpl_image_new_from_accepted, in.ptr()));
}

const cpl_imagelist*
ImageList::ptr() const
{
  return m_interface;
}

cpl_imagelist*
ImageList::ptr()
{
  return m_interface;
}


}  // namespace core
}  // namespace cpl
