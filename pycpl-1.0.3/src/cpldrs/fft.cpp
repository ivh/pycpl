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


/**
 * Wraps the cpl drs fft functions as Python functions with type converison
 */

#include "cpldrs/fft.hpp"

#include <cpl_type.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{

// FIXME: Review that. CPL can be built without FFT support, but although
//        provisions are made here to cope with that the expression is not
//        used anywhere. The question is, is this obsolete and can be removed,
//        or is there something missing which should be implemented
#if 0
#ifdef CPL_FFTWF_INSTALLED
constexpr const bool can_fft = true;
#else
constexpr const bool can_fft = false;
#endif
#endif

std::shared_ptr<cpl::core::ImageBase>
fft_image(const cpl::core::ImageBase& other, cpl_fft_mode transform,
          cpl_fft_mode* find, bool scale)
{
  unsigned outputType = other.get_type();
  if (transform == CPL_FFT_FORWARD && !other.is_complex()) {
    outputType |=
        CPL_TYPE_COMPLEX;  // FFT will convert the real type to complex
  }
  auto output = cpl::core::ImageBase::make_image(
      other.get_width(), other.get_height(), cpl_type(outputType));
  cpl_fft_mode mode =
      find == nullptr ? transform : cpl_fft_mode(transform | *find);

  if (!scale)
    mode = cpl_fft_mode(mode | CPL_FFT_NOSCALE);
  cpl::core::Error::throw_errors_with(cpl_fft_image, output->ptr(), other.ptr(),
                                      mode);
  return output;
}

// NOTE: CPL source just passes each image into the fft_image function. Just
// going to do the same here unless otherwised apposed to ensure same checks are
// made
std::shared_ptr<cpl::core::ImageList>
fft_imagelist(const cpl::core::ImageList& others, cpl_fft_mode transform,
              cpl_fft_mode* find, bool scale)
{
  std::shared_ptr<cpl::core::ImageList> output =
      std::make_shared<cpl::core::ImageList>();
  for (int i = 0; i < others.size(); i++) {
    output->append(fft_image(*others.get_at(i), transform, find, scale));
  }
  return output;
}

}  // namespace drs
}  // namespace cpl