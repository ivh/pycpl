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

#include "cpldrs/fft_bindings.hpp"

#include "cpldrs/fft.hpp"

namespace py = pybind11;

using size = cpl::core::size;

void
bind_fft(py::module& m)
{
  py::module mfft = m.def_submodule("fft", "FFT operations via fftw wrappers");
  py::enum_<cpl_fft_mode>(mfft, "Mode", py::arithmetic())
      .value("FORWARD", CPL_FFT_FORWARD)
      .value("BACKWARD", CPL_FFT_BACKWARD)
      .value("NOSCALE", CPL_FFT_NOSCALE)
      .value("FIND_MEASURE", CPL_FFT_FIND_MEASURE)
      .value("FIND_PATIENT", CPL_FFT_FIND_PATIENT)
      .value("FIND_EXHAUSTIVE", CPL_FFT_FIND_EXHAUSTIVE)
      .export_values();
  ;

  mfft.def("fft_image", &cpl::drs::fft_image, py::arg("other"),
           py::arg("transform"), py::arg("find").none(true) = py::none(),
           py::arg("scale") = true,
           R"mydelim(
    Perform a FFT operation on an image

    Parameters
    ----------
    - other: The frameset from which the product frames are taken.
    - transform: cpl.drs.fft.FORWARD or cpl.drs.fft.FORWARD
    - find: based on enum, time spent searching (cpl.drs.fft.FIND_MEASURE,
            cpl.drs.fft.FIND_PATIENT, cpl.drs.fft.FIND_EXHAUSTIVE)
    - scale: true or false, whether or not to transform without scaling (only
             effects backwards transforms)

    Return
    ------
    output image of the FFT operation

    Notes
    -----
    This function performs an FFT on an image, using FFTW. CPL may be configured
    without this library, in this case an otherwise valid call will set and throw
    UnsupportedModeError.

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
    transformations can be achieved by passing cpl.drs.FIND_MEASURE to the find
    param.

    For a larger number of transformations a further reduction may be achived
    cpl.drs.FIND_PATIENT and for an even larger number of
    transformations a further reduction may be achived with the flag
    cpl.drs.FIND_EXHAUSTIVE.

    If many transformations are to be done then a reduction in the time required
    to perform the transformations can be achieved by using cpl_fft_imagelist().

    Raises
    ------
    cpl.core.IllegalInputError
      if the mode is illegal
    cpl.core.TypeMismatchError
      if the image types are incompatible with each other
    cpl.core.UnsupportedModeError
      if FFTW has not been installed
    )mydelim")
      .def("fft_imagelist", &cpl::drs::fft_imagelist, py::arg("other"),
           py::arg("transform"), py::arg("find").none(true) = py::none(),
           py::arg("scale") = true,
           R"mydelim(
    Perform a FFT operation on the images in an imagelist

    Parameters
    ----------
    other : cpl.core.ImageList
      Input imagelist to transform from
    transform : cpl.drs.fft.Mode
      cpl.drs.fft.FORWARD or cpl.drs.fft.FORWARD
    find : cpl.drs.fft.Mode or None, default=None
      based on enum, time spent searching (cpl.drs.fft.FIND_MEASURE, cpl.drs.fft.FIND_PATIENT, cpl.drs.fft.FIND_EXHAUSTIVE)
    scale : bool, default=True
      true or false, whether or not to transform without scaling (only effects backwards transforms)

    Returns
    -------
    cpl.core.ImageList
      output imagelist to store transformed images

    Notes
    -----
    Convenience function for running cpl.drs.fft.image() on all images in the input imagelist
    )mydelim");
}