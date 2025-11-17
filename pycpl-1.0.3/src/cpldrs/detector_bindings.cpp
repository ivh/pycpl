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

#include "cpldrs/detector_bindings.hpp"

#include <pybind11/stl.h>

#include "cpldrs/detector.hpp"

namespace py = pybind11;

void
bind_detector(py::module& m)
{
  py::module detector = m.def_submodule(
      "detector", "High-level functions to compute detector features.");

  detector
      .def("get_noise_window", &cpl::drs::detector::get_noise_window,
           py::arg("diff_image"), py::arg("zone_def").none(true) = py::none(),
           py::arg("ron_hsize") = -1,
           py::arg("ron_nsamp") =
               -1,  // Function suggests to use less than 0 to get defaults.
                    // Actual defaults are not detailed for users but in the
                    // source it is ron_hsize=4, ron_nsamp=100
           R"pydoc(
    Compute the noise in a rectangle.

    This function is meant to compute the noise in a frame by means of a
    MonteCarlo approach. The input is a frame, usually a difference between two
    frames taken with the same settings for the acquisition system, although no
    check is done on that, it is up to the caller to feed in the right kind of
    frame.

    If the input image is the difference of two bias frames taken with the same settings
    then the returned noise measure will be sqrt(2) times the image sensor read noise

    Parameters
    ----------
    diff_image: cpl.core.Image
        Input image, usually a difference frame.
    zone_def: tuple(int, int, int, int), optional
        Tuple to describe the window where the bias is to be computed in the format (xmin, xmax, ymin, ymax), using PyCPL notation where the bottom left is (0,0)
    ron_hsize: int, optional
        to specify half size of squares, default 4
    ron_nsamp: int, optional
        to specify the nb of samples, default 1000

    Returns
    -------
    tuple(float, float)
        The noise in the frame and the error of the noise in the format (noise, error).

    Raises
    ------
    cpl.core.IllegalInputError
        if the specified window (zone_def) is invalid

    Notes
    -----
    The algorithm will create typically 100 9x9 windows on the frame, scattered
    optimally using a Poisson law. In each window, the standard deviation of all
    pixels in the window is computed and this value is stored.

    The output `noise` is the median of all computed standard deviations, and the error is the
    standard deviation of the standard deviations.

    See Also
    --------
    cpl.drs.detector.get_noise_ring : Computes noise using a ring.
      )pydoc")
      .def("get_bias_window", &cpl::drs::detector::get_bias_window,
           py::arg("bias_image"), py::arg("zone_def").none(true) = py::none(),
           py::arg("ron_hsize") = -1,
           py::arg("ron_nsamp") =
               -1,  // Function suggests to use less than 0 to get defaults.
                    // Actual defaults are not detailed for users but in the
                    // source it is ron_hsize=4, ron_nsamp=100
           R"pydoc(
    Compute the bias in a rectangle.

    This function is meant to compute the bias level from an image by means of a
    MonteCarlo approach. The input image would normally be a bias frame although
    no check is done on that, it is up to the caller to feed in the right kind of
    frame.

    Parameters
    ----------
    bias_image: cpl.core.Image
        Input image, normally a bias frame
    zone_def: tuple(int, int, int, int), optional
        Tuple to describe the window where the bias is to be computed in the
        format (xmin, xmax, ymin, ymax), using PyCPL notation where the bottom
        left pixel is (0,0)
    ron_hsize: int, optional
        to specify half size of squares default 4
    ron_nsamp: int, optional
        to specify the nb of samples, default 1000

    Returns
    -------
    tuple(float, float)
        The bias in the frame and the error of the bias in the format (bias, error)

    Raises
    ------
    cpl.core.IllegalInputError
        if the specified window (zone_def) is invalid

    Notes
    -----
    The algorithm will create typically 100 9x9 windows on the frame, scattered
    optimally using a Poisson law. In each window, the mean of all pixels in the
    window is computed and this value is stored.

    The output `bias` is the median of all computed means, and the error is the
    standard deviation of the means.
      )pydoc")
      .def("get_noise_ring", &cpl::drs::detector::get_noise_ring,
           py::arg("diff_image"), py::arg("zone_def"),
           py::arg("ron_hsize") = -1,
           py::arg("ron_nsamp") =
               -1,  // Function suggests to use less than 0 to get defaults.
                    // Actual defaults are not detailed for users but in the
                    // source it is ron_hsize=4, ron_nsamp=100
           R"pydoc(
    Compute the noise in a ring.

    This function is meant to compute the noise in a frame by means of a
    MonteCarlo approach. The input is a frame, usually a difference between two
    frames taken with the same settings for the acquisition system, although no
    check is done on that, it is up to the caller to feed in the right kind of
    frame.

    If the input image is the difference of two bias frames taken with the same settings
    then the returned noise measure will be sqrt(2) times the image sensor read noise

    Parameters
    ----------
    diff_image: cpl.core.Image
        Input image, usually a difference frame.
    zone_def: tuple(int, int, float, float)
        Tuple to describe the window where the bias is to be computed in the
        format (x, y, r1, r2). The first two intergers specify the centre position
        of the ring as x, y, using PyCPL notation where the bottom left is (0,0).
        Floats r1 and r2 specify the ring start and end radiuses.
    ron_hsize: int, optional
        to specify half size of squares default 4
    ron_nsamp: int, optional
        to specify the nb of samples, default 1000

    Returns
    -------
    tuple(float, float)
        The noise in the frame and the error of the noise in the format (noise, error).

    Raises
    ------
    cpl.core.IllegalInputError
        if the internal radius (r1) is bigger than the external one (r2) in `zone_def`
    cpl.core.DataNotFoundError
        If an insufficient number of samples were found inside the ring

    Notes
    -----
    The algorithm will create typically 100 9x9 windows on the frame, scattered
    optimally using a Poisson law. In each window, the standard deviation of all
    pixels in the window is computed and this value is stored. The `output` noise
    is the median of all computed standard deviations, and the error is the
    standard deviation of the standard deviations.

    See Also
    --------
    cpl.drs.detector.get_noise_window : Computes noise using a rectangle.
      )pydoc")
      .def("interpolate_rejected", &cpl::drs::detector::interpolate_rejected,
           py::arg("to_clean"),
           R"pydoc(
    Interpolate any bad pixels in an image in place

    Parameters
    ----------
    to_clean: cpl.core.Image
        The image to clean

    Raises
    ------
    cpl.core.DataNotFoundError
        if all pixels are bad

    Notes
    -----
    The value of a bad pixel is interpolated from the good pixels among the
    8 nearest. (If all but one of the eight neighboring pixels are bad, the
    interpolation becomes a nearest neighbor interpolation). For integer
    images the interpolation in done with floating-point and rounded to the
    nearest integer.

    If there are pixels for which all of the eight neighboring pixels are bad,
    a subsequent interpolation pass is done, where the already interpolated
    pixels are included as source for the interpolation.

    The interpolation passes are repeated until all bad pixels have been
    interpolated. In the worst case, all pixels will be interpolated from a
    single good pixel.
      )pydoc");
}
