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

#ifndef PYCPL_FFT_HPP
#define PYCPL_FFT_HPP

#include <memory>

#include <cpl_fft.h>

#include "cplcore/image.hpp"
#include "cplcore/imagelist.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

/*---------------------------------------------------------------------------*/
/**
  @brief    Perform a FFT operation on an image
  @param  other Input image to transform from, use self for in-place transform
  @param  transform  CPL_FFT_FORWARD or CPL_FFT_BACKWARD
  @param  find based on enum, time spent searching (CPL_FFT_FIND_MEASURE,
  CPL_FFT_FIND_PATIENT, CPL_FFT_FIND_EXHAUSTIVE)
  @param  scale  true or false, whether or not to transform without scaling
  (only effects backwards transforms)
  @return output image of the FFT operation

  This function performs an FFT on an image, using FFTW. CPL may be configured
  without this library, in this case an otherwise valid call will set and return
  the error CPL_ERROR_UNSUPPORTED_MODE.

  The input and output images must match in precision level. Integer images are
  not supported.

  In a forward transform the input image may be non-complex. In this case a
  real-to-complex transform is performed. This will only compute the first
  nx/2 + 1 columns of the transform. In this transform it is allowed to pass
  an output image with nx/2 + 1 columns.

  Similarly, in a backward transform the output image may be non-complex. In
  this case a complex-to-real transform is performed. This will only transform
  the first nx/2 + 1 columns of the input. In this transform it is allowed to
  pass an input image with nx/2 + 1 columns.

  Per default the backward transform scales (divides) the result with the
  number of elements transformed (i.e. the number of pixels in the result
  image). This scaling can be turned off with CPL_FFT_NOSCALE.

  If many transformations in the same direction are to be done on data of the
  same size and type, a reduction in the time required to perform the
  transformations can be achieved by adding the flag CPL_FFT_FIND_MEASURE
  to the first transformation.
  For a larger number of transformations a further reduction may be achived
  with the flag CPL_FFT_FIND_PATIENT and for an even larger number of
  transformations a further reduction may be achived with the flag
  CPL_FFT_FIND_EXHAUSTIVE.

  If many transformations are to be done then a reduction in the time required
  to perform the transformations can be achieved by using cpl_fft_imagelist().
  @throw IllegalInputError if the mode is illegal
  @throw TypeMismatchError if the image types are incompatible with each other
  @throw UnssupportedModeError if FFTW has not been installed
 */
std::shared_ptr<cpl::core::ImageBase>
fft_image(const cpl::core::ImageBase& other, cpl_fft_mode transform,
          cpl_fft_mode* find, bool scale);
/*----------------------------------------------------------------------------*/
/**
  @brief  Perform a FFT operation on the images in an imagelist
  @param  other Input imagelist to transform from
  @param  transform  CPL_FFT_FORWARD or CPL_FFT_BACKWARD
  @param  find based on enum, time spent searching (CPL_FFT_FIND_MEASURE,
  CPL_FFT_FIND_PATIENT, CPL_FFT_FIND_EXHAUSTIVE)
  @param  scale  true or false, whether or not to transform without scaling
  (only effects backwards transforms)
  @return output imagelist to store transformed images
  @see cpl_fft_image()

 */
/*----------------------------------------------------------------------------*/
std::shared_ptr<cpl::core::ImageList>
fft_imagelist(const cpl::core::ImageList& others, cpl_fft_mode transform,
              cpl_fft_mode* find, bool scale);

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_FFT_HPP