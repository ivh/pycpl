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

#ifndef PYCPL_DETECTOR_HPP
#define PYCPL_DETECTOR_HPP

#include <optional>
#include <tuple>

#include "cplcore/image.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

namespace detector
{

/**
 * @brief Interpolate any bad pixels in an image and delete the bad pixel map
 * @param toClean The image to clean
 *
 * The value of a bad pixel is interpolated from the good pixels among the
 * 8 nearest. (If all but one of the eight neighboring pixels are bad, the
 * interpolation becomes a nearest neighbor interpolation). For integer
 * images the interpolation in done with floating-point and rounded to the
 * nearest integer.
 *
 * If there are pixels for which all of the eight neighboring pixels are bad,
 * a subsequent interpolation pass is done, where the already interpolated
 * pixels are included as source for the interpolation.
 *
 * The interpolation passes are repeated until all bad pixels have been
 * interpolated. In the worst case, all pixels will be interpolated from a
 * single good pixel.
 *
 * @throw DataNotFoundError if all pixels are bad
 */
void interpolate_rejected(const cpl::core::ImageBase& toClean);

/**
 * @brief Compute the bias in a rectangle.
 * @param bias_image Input image
 * @param zone_def Zone where the bias is to be computed.
 * @param ron_hsize to specify half size of squares (<0 to use default)
 * @param ron_nsamp to specify the nb of samples (<0 to use default)
 *
 * @return tuple of the bias in the frame and the error on the bias in format
 * {bias, error}
 * @see get_noise_window() for in depth explaination of zone_def, ron_hsize and
 * ron_nsamp
 */
std::tuple<double, double>
get_bias_window(const cpl::core::ImageBase& bias_image,
                std::optional<std::tuple<size, size, size, size>> zone_def,
                size ron_hsize, size ron_nsamp);

/**
 * @brief Compute the readout noise in a rectangle.
 * @param diff Input image, usually a difference frame.
 * @param zone_def Zone where the readout noise is to be computed.
 * @param ron_hsize to specify half size of squares (<0 to use default)
 * @param ron_nsamp to specify the nb of samples (<0 to use default)
 * @param noise Output parameter: noise in the frame.
 * @param error Output parameter: error on the noise.
 *
 * This function is meant to compute the readout noise in a frame by means of a
 * MonteCarlo approach. The input is a frame, usually a difference between two
 * frames taken with the same settings for the acquisition system, although no
 * check is done on that, it is up to the caller to feed in the right kind of
 * frame.
 *
 * The provided zone is an array of four integers specifying the zone to take
 * into account for the computation. The integers specify ranges as xmin, xmax,
 * ymin, ymax, where these coordinates are given in the FITS notation (x from 1
 * to lx, y from 1 to ly and bottom to top). Specify NULL instead of an array of
 * four values to use the whole frame in the computation.
 *
 * The algorithm will create typically 100 9x9 windows on the frame, scattered
 * optimally using a Poisson law. In each window, the standard deviation of all
 * pixels in the window is computed and this value is stored. The readout noise
 * is the median of all computed standard deviations, and the error is the
 * standard deviation of the standard deviations.
 *
 * Both noise and error are returned by modifying a passed double. If you do
 * not care about the error, pass NULL.
 *
 * @return tuple of the noise in the frame and the error on the noise in format
 * {noise, error}
 */
std::tuple<double, double>
get_noise_window(const cpl::core::ImageBase& diff,
                 std::optional<std::tuple<size, size, size, size>> zone_def,
                 size ron_hsize, size ron_nsamp);


/**
 * @brief Compute the readout noise in a ring.
 * @param diff Input image, usually a difference frame.
 * @param zone_def Zone where the readout noise is to be computed.
 * @param ron_hsize to specify half size of squares (<0 to use default)
 * @param ron_nsamp to specify the nb of samples (<0 to use default)
 * @param noise On success, *noise is the noise in the frame.
 * @param error NULL, or on success, *error is the error in the noise
 *
 * The provided zone is an array of two integers and two floats specifying the
 * zone to take into account for the computation. The first two intergers
 * specify the centre position of the ring as x, y, while the 2 floats specify
 * the ring start and end radiuses r1 and r2
 * @return tuple of the noise in the frame and the error on the noise in format
 * {noise, error}
 * @throws IllegalInputError if the internal radius is bigger than the
 * @throws DataNotFoundError if an insufficient number of samples were found
 * @see get_noise_window() for in depth explaination of zone_def, ron_hsize and
 * ron_nsamp
 */
std::tuple<double, double>
get_noise_ring(const cpl::core::ImageBase& diff,
               std::tuple<size, size, double, double> zone_def, size ron_hsize,
               size ron_nsamp);

}  // namespace detector
}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_DETECTOR_HPP