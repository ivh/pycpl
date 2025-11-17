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

#include "cplcore/filter_bindings.hpp"

#include <cpl_filter.h>

namespace py = pybind11;

void
bind_filters(py::module& m)
{
  py::enum_<_cpl_border_mode_>(
      m, "Border",
      "Supported border modes for use with filtering functions in "
      "cpl.core.Image and cpl.core.Mask. For a kernel of width 2n+1, the n "
      "left- and rightmost image/mask columns do not have elements for the "
      "whole kernel. The same holds for the top and bottom image/mask rows."
      "The border mode defines the filtering of such border pixels.")
      .value("FILTER", CPL_BORDER_FILTER,
             "Filter the border using the reduced number of pixels. If in "
             "median filtering the number of pixels is even choose the mean of "
             "the two central values, after the borders have been filled with "
             "a chess-like pattern of +- inf ")
      .value("ZERO", CPL_BORDER_ZERO,
             "Set the border of the filtered image/mask to zero.")
      .value("CROP", CPL_BORDER_CROP, "Crop the filtered image/mask.")
      .value("NOP", CPL_BORDER_NOP,
             "Do not modify the border of the filtered image/mask.")
      .value("COPY", CPL_BORDER_COPY,
             "Copy the border of the input image/mask. For an in-place "
             "operation this has the no effect, identical to CPL_BORDER_NOP.");

  py::enum_<_cpl_filter_mode_>(
      m, "Filter",
      "Supported filter modes for use with filtering functions in "
      "cpl.core.Image and cpl.core.Mask.")
      .value("EROSION", CPL_FILTER_EROSION,
             "Erosion filter for cpl.core.Mask filtering (see "
             "cpl.core.Mask.filter)")
      .value("DILATION", CPL_FILTER_DILATION,
             "Dilation filter for cpl.core.Mask filtering (see "
             "cpl.core.Mask.filter)")
      .value("OPENING", CPL_FILTER_OPENING,
             "Opening filter for cpl.core.Mask filtering (see "
             "cpl.core.Mask.filter)")
      .value("CLOSING", CPL_FILTER_CLOSING,
             "Closing filter for cpl.core.Mask filtering (see "
             "cpl.core.Mask.filter)")
      .value("LINEAR", CPL_FILTER_LINEAR, R"pydoc(
        A linear filter (for a cpl.core.Image.filter). The kernel elements are normalized
        with the sum of their absolute values. This implies that there must be
        at least one non-zero element in the kernel. The normalisation makes the
        kernel useful for filtering where flux conservation is desired.

        The kernel elements are thus used as weights like this::

            Kernel              Image
                                  ...
            1 2 3         ... 1.0 2.0 3.0 ...
            4 5 6         ... 4.0 5.0 6.0 ...
            7 8 9         ... 7.0 8.0 9.0 ...
                                  ...

        The filtered value corresponding to the pixel whose value is 5.0 is:

        .. math::

            \frac{(1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0)} {1+2+3+4+5+6+7+8+9}

        Filtering with cpl.core.Filter.LINEAR and a flat kernel can be done faster with cpl.core.Filter.AVERAGE.
        )pydoc")
      .value("LINEAR_SCALE", CPL_FILTER_LINEAR_SCALE, R"pydoc(
        A linear filter (for a cpl.core.Image.filter). Unlike cpl.core.Filter.LINEAR the kernel elements are not
        normalized, so the filtered image will have its flux scaled with the sum of the weights of the kernel.
        Examples of linear, scaling kernels are gradient operators and edge detectors.

        The kernel elements are thus applied like this::

            Kernel              Image
                                  ...
            1 2 3         ... 1.0 2.0 3.0 ...
            4 5 6         ... 4.0 5.0 6.0 ...
            7 8 9         ... 7.0 8.0 9.0 ...
                                  ...

        The filtered value corresponding to the pixel whose value is 5.0 is:

        .. math::

            1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0

        )pydoc")
      .value("AVERAGE", CPL_FILTER_AVERAGE, R"pydoc(
        An average filter, i.e. the output pixel is the arithmetic average of the surrounding
        (1 + 2 * hsizex)(1 + 2 * hsizey) pixels. The cost per pixel is O(hsizex*hsizey).

        The two images may have different pixel types. When the input and output pixel types are identical,
        the arithmetic is done with that type, e.g. int for two integer images. When the input and output pixel
        types differ, the arithmetic is done in double precision when one of the two images have pixel type
        cpl.core.Type.DOUBLE, otherwise float is used.
        )pydoc")
      .value("AVERAGE_FAST", CPL_FILTER_AVERAGE_FAST, R"pydoc(
        The same as cpl.core.Filter.AVERAGE, except that it uses a running average, which will lead to a significant
        loss of precision if there are large differences in the magnitudes of the input pixels. The cost per pixel
        is O(1) if all elements in the kernel are used, otherwise the filtering is done as for cpl.core.Filter.AVERAGE.
        )pydoc")
      .value("MEDIAN", CPL_FILTER_MEDIAN,
             "A median filter (for a cpl.core.Image). The pixel types of the "
             "input and output images must be identical.")
      .value("STDEV", CPL_FILTER_STDEV, R"pydoc(
        The filtered value is the standard deviation of the included input pixels::

            Kernel                      Image
                                        ...
            1   0   1           ... 1.0 2.0 3.0 ...
            0   1   0           ... 4.0 5.0 6.0 ...
            1   0   1           ... 7.0 8.0 9.0 ...
                                        ...

        The pixel with value 5.0 will have a filtered value of: std_dev(1.0, 3.0, 5.0, 7.0, 9.0)
        )pydoc")
      .value("STDEV_FAST", CPL_FILTER_STDEV_FAST, R"pydoc(
        The same as cpl.core.Filter.STDEV, except that it uses the same running method employed in cpl.core.Filter.AVERAGE_FAST,
        which will lead to a significant loss of precision if there are large differences in the magnitudes of the input pixels.
        As with cpl.core.Filter.AVERAGE_FAST, the cost per pixel is O(1) if all elements in the kernel are used, otherwise the
        filtering is done as for cpl.core.Filter.AVERAGE.
        )pydoc")
      .value("MORPHO", CPL_FILTER_MORPHO, R"pydoc(
        A morphological filter (for a cpl.core.Image). The kernel elements are
        normalized with the sum of their absolute values. This implies that
        there must be at least one non-zero element in the kernel. The
        normalisation makes the kernel useful for filtering where flux
        conservation is desired.

        The kernel elements are used as weights on the sorted values covered by the kernel::

            Kernel                Image
                                  ...
            1 2 3         ... 4.0 6.0 5.0 ...
            4 5 6         ... 3.0 1.0 2.0 ...
            7 8 9         ... 7.0 8.0 9.0 ...
                                  ...

        The filtered value corresponding to the pixel whose value is 5.0 is:
        .. math::

            \frac{(1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0)}{1+2+3+4+5+6+7+8+9}

        )pydoc")
      .value("MORPHO_SCALE", CPL_FILTER_MORPHO_SCALE, R"pydoc(
        A morphological filter (for a cpl.core.Image). Unlike cpl.core.Filter.MORPHO
        the kernel elements are not normalized, so the filtered image will have
        its flux scaled with the sum of the weights of the kernel.

        The kernel elements are thus applied to the the sorted values covered by the kernel::

            Kernel                Image
                                  ...
            1 2 3         ... 4.0 6.0 5.0 ...
            4 5 6         ... 3.0 1.0 2.0 ...
            7 8 9         ... 7.0 8.0 9.0 ...
                                  ...

        The filtered value corresponding to the pixel whose value is 5.0 is:
        .. math::

            1*1.0+2*2.0+3*3.0+4*4.0+5*5.0+6*6.0+7*7.0+8*8.0+9*9.0

        )pydoc");
}