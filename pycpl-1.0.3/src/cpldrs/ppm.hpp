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

#ifndef PYCPL_PPM_HPP
#define PYCPL_PPM_HPP

#include <tuple>
#include <vector>

#include "cplcore/bivector.hpp"
#include "cplcore/matrix.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

/**
 * @brief
 *   Match 2-D distributions of points.
 *
 * @param data        List of data points (e.g., detected stars positions).
 * @param use_data    Number of @em data points used for preliminary match.
 * @param err_data    Error on @em data points positions.
 * @param pattern     List of pattern points (e.g., expected stars positions).
 * @param use_pattern Number of @em pattern points used for preliminary match.
 * @param err_pattern Error on @em pattern points positions.
 * @param tolerance   Max relative difference of angles and scales from
 *                    their median value for match acceptance.
 * @param radius      Search radius applied in final matching (@em data units).
 *
 * @return A tuple of all function return variables, including:
 *      - Indexes of identified data points (pattern-to-data). (the original
 * return value)
 *      - mdata, the list of identified @em data points
 *      - mpattern, the list of matching @em pattern points
 *      - lin_scale, the Linear transformation scale factor
 *      - lin_angle, the Linear transformation rotation angle
 *
 *
 * A point is described here by its coordinates on a cartesian plane.
 * The input matrices @em data and @em pattern must have 2 rows, as
 * their column vectors are the points coordinates.
 *
 * This function attemps to associate points in @em data to points in
 * @em pattern, under the assumption that a transformation limited to
 * scaling, rotation, and translation, would convert positions in
 * @em pattern into positions in @em data. Association between points
 * is also indicated in the following as "match", or "identification".
 *
 * Point identification is performed in two steps. In the first step
 * only a subset of the points is identified (preliminary match). In
 * the second step the identified points are used to define a first-guess
 * transformation from @em pattern points to @em data points, that is
 * applied to identify all the remaining points as well. The second
 * step would be avoided if a @em use_pattern equal to the number of
 * points in @em pattern is given, and exactly @em use_pattern points
 * would be identified already in the first step.
 *
 * First step:
 *
 * All possible triangles (sub-patterns) are built using the first
 * @em use_data points from @em data and the first @em use_pattern
 * points from @em pattern. The values of @em use_data and @em use_pattern
 * must always be at least 3 (however, see the note at the end),
 * and should not be greater than the length of the corresponding
 * lists of points. The point-matching algorithm goes as follow:
 *
 * @code
 * For every triplet of points:
 *    Select one point as the reference. The triangle coordinates
 *    are defined by
 *
 *                ((Rmin/Rmax)^2, theta_min - theta_max)
 *
 *    where Rmin (Rmax) is the shortest (longest) distance from the
 *    reference point to one of the two other points, and theta_min
 *    (theta_max) is the view angle in [0; 2pi[ to the nearest
 *    (farthest) point.
 *
 *    Triangles are computed by using each point in the triplet
 *    as reference, thereby computing 3 times as many triangles
 *    as needed.
 *
 *    The accuracy of triangle patterns is robust against distortions
 *    (i.e., systematic inaccuracies in the points positions) of the
 *    second order. This is because, if the points positions had
 *    constant statistical uncertainty, the relative uncertainty in
 *    the triangle coordinates would be inversely proportional to
 *    the triangle size, while if second order distortions are
 *    present the systematic error on points position would be
 *    directly proportional to the triangle size.
 *
 * For every triangle derived from the @em pattern points:
 *    Match with nearest triangle derived from @em data points
 *    if their distance in the parameter space is less than their
 *    uncertainties (propagated from the points positions uncertainties
 *    @em err_data and @em err_pattern). For every matched pair of
 *    triangles, record their scale ratio, and their orientation
 *    difference. Note that if both @em err_data and @em err_pattern
 *    are zero, the tolerance in triangle comparison will also be
 *    zero, and therefore no match will be found.
 *
 * Get median scale ratio and median angle of rotation, and reject
 * matches with a relative variation greater than @em tolerance from
 * the median of either quantities. The estimator of all the rotation
 * angles a_i is computed as
 *
 *             atan( med sin(a_i) / med cos(a_i) )
 *
 * @endcode
 *
 * Second step:
 *
 * From the safely matched triangles, a list of identified points is
 * derived, and the best transformation from @em pattern points to
 * @em data points (in terms of best rotation angle, best scaling
 * factor, and best shift) is applied to attempt the identification of
 * all the points that are still without match. This matching is made
 * by selecting for each @em pattern point the @em data point which is
 * closest to its transformed position, and at a distance less than
 * @em radius.
 *
 * The returned array of integers is as long as the number of points in
 * @em pattern, and each element reports the position of the matching
 * point in @em data (counted starting from zero), or is invalid if no
 * match was found for the @em pattern point. For instance, if element
 * N of the array has value M, it means that the Nth point in @em pattern
 * matches the Mth point in @em data. A NULL pointer is returned in case
 * no point was identified.
 *
 * If @em mdata and @em mpattern are both valid pointers, two more
 * matrices will be returned with the coordinates of the identified
 * points. These two matrices will both have the same size: 2 rows,
 * and as many columns as successfully identified points. Matching
 * points will be in the same column of both matrices. Those matrix
 * should in the end be destroyed using cpl_matrix_delete().
 *
 * If @em lin_scale is a valid pointer, it is returned with a good estimate
 * of the scale (distance_in_data = lin_scale * distance_in_pattern).
 * This makes sense only in case the transformation between @em pattern
 * and @em data is an affine transformation. In case of failure,
 * @em lin_scale is set to zero.
 *
 * If @em lin_angle is a valid pointer, it is returned with a good
 * estimate of the rotation angle between @em pattern and @em data
 * in degrees (counted counterclockwise, from -180 to +180, and with
 * data_orientation = pattern_orientation + lin_angle). This makes
 * sense only in case the transformation between @em pattern and
 * @em data is an affine transformation. In case of failure,
 * @em lin_angle is set to zero.
 *
 * The returned values for @em lin_scale and @em lin_angle have the only
 * purpose of providing a hint on the relation between @em pattern points
 * and @em data points. This function doesn't attempt in any way to
 * determine or even suggest a possible transformation between @em pattern
 * points and @em data points: this function just matches points, and it
 * is entriely a responsibility of the caller to fit the appropriate
 * transformation between one coordinate system and the other.
 * A polynomial transformation of degree 2 from @em pattern to @em data
 * may be fit in the following way (assuming that @em mpattern and
 * @em mdata are available):
 *
 * @code
 *
 * int             degree  = 2;
 * int             npoints = cpl_matrix_get_ncol(mdata);
 * double         *dpoints = cpl_matrix_get_data(mdata);
 * cpl_vector     *data_x  = cpl_vector_wrap(npoints, dpoints);
 * cpl_vector     *data_y  = cpl_vector_wrap(npoints, dpoints + npoints);
 * cpl_polynomial *x_trans = cpl_polynomial_new(degree);
 * cpl_polynomial *y_trans = cpl_polynomial_new(degree);
 *
 * cpl_polynomial_fit(x_trans, mpattern, NULL, data_x, NULL, CPL_FALSE,
 *                    NULL, degree);
 * cpl_polynomial_fit(y_trans, mpattern, NULL, data_y, NULL, CPL_FALSE,
 *                    NULL, degree);
 *
 * @endcode
 *
 * @note
 * The basic requirement for using this function is that the searched
 * point pattern (or at least most of it) is contained in the data.
 * As an indirect consequence of this, it would generally be appropriate
 * to have more points in @em data than in @em pattern (and analogously,
 * to have @em use_data greater than @em use_pattern), even if this is
 * not strictly necessary.
 *
 * Also, @em pattern and @em data should not contain too few points
 * (say, less than 5 or 4) or the identification may risk to be incorrect:
 * more points enable the construction of many more triangles, reducing
 * the risk of ambiguity (multiple valid solutions). Special situations,
 * involving regularities in patterns (as, for instance, input @em data
 * containing just three equidistant points, or the case of a regular
 * grid of points) would certainly provide an answer, and this answer
 * would very likely be wrong (the human brain would fail as well,
 * and for exactly the same reasons).
 *
 * The reason why a two steps approach is encouraged here is mainly to
 * enable an efficient use of this function: in principle, constructing
 * all possible triangles using @em all of the available points is never
 * wrong, but it could become very slow: a list of N points implies the
 * evaluation of N*(N-1)*(N-2)/2 triangles, and an even greater number
 * of comparisons between triangles. The possibility of evaluating
 * first a rough transformation based on a limited number of identified
 * points, and then using this transformation for recovering all the
 * remaining points, may significantly speed up the whole identification
 * process. However it should again be ensured that the main requirement
 * (i.e., that the searched point pattern must be contained in the data)
 * would still be valid for the selected subsets of points: a random
 * choice would likely lead to a matching failure (due to too few, or
 * no, common points).
 *
 * A secondary reason for the two steps approach is to limit the effect
 * of another class of ambiguities, happening when either or both of
 * the input matrices contains a very large number of uniformely
 * distributed points. The likelihood to find several triangles that
 * are similar by chance, and at all scales and orientations, may
 * increase to unacceptable levels.
 *
 * A real example may clarify a possible way of using this function:
 * let @em data contain the positions (in pixel) of detected stars
 * on a CCD. Typically hundreds of star positions would be available,
 * but only the brightest ones may be used for preliminary identification.
 * The input @em data positions will therefore be opportunely ordered
 * from the brightest to the dimmest star positions. In order to
 * identify stars, a star catalogue is needed. From a rough knowledge
 * of the pointing position of the telescope and of the size of the
 * field of view, a subset of stars can be selected from the catalogue:
 * they will be stored in the @em pattern list, ordered as well by their
 * brightness, and with their RA and Dec coordinates converted into
 * standard coordinates (a gnomonic coordinate system centered on the
 * telescope pointing, i.e., a cartesian coordinate system), no matter
 * in what units of arc, and no matter what orientation of the field.
 * For the first matching step, the 10 brightest catalogue stars may
 * be selected (selecting less stars would perhaps be unsafe, selecting
 * more would likely make the program slower without producing any
 * better result). Therefore @em use_pattern would be set to 10.
 * From the data side, it would generally be appropriate to select
 * twice as many stars positions, just to ensure that the searched
 * pattern is present. Therefore @em use_data would be set to 20.
 * A reasonable value for @em tolerance and for @em radius would be
 * respectively 0.1 (a 10% variation of scales and angles) and 20
 * (pixels).
 */
std::tuple<std::vector<int>, cpl::core::Matrix, cpl::core::Matrix, double,
           double>
match_points(const cpl::core::Matrix& data, size use_data, double err_data,
             const cpl::core::Matrix& pattern, size use_pattern,
             double err_pattern, double tolerance, double radius);
// Dont forget to return: cpl_matrix **mdata, cpl_matrix **mpattern,
// double *lin_scale, double *lin_angle)
/**
 * @brief
 *   Match 1-D patterns.
 *
 * @param peaks     List of observed positions (e.g., of emission peaks)
 * @param lines     List of positions in searched pattern (e.g., wavelengths)
 * @param min_disp  Min expected scale (e.g., spectral dispersion in A/pixel)
 * @param max_disp  Max expected scale (e.g., spectral dispersion in A/pixel)
 * @param tolerance Tolerance for interval ratio comparison
 *
 * @return List of all matching points positions
 *
 * This function attempts to find the reference pattern @em lines in a
 * list of observed positions @em peaks. In the following documentation
 * a terminology drawn from the context of arc lamp spectra calibration
 * is used for simplicity: the reference pattern is then a list of
 * wavelengths corresponding to a set of reference arc lamp emission
 * lines - the so-called line catalog; while the observed positions are
 * the positions (in pixel) on the CCD, measured along the dispersion
 * direction, of any significant peak of the signal. To identify the
 * observed peaks means to associate them with the right reference
 * wavelength. This is attempted here with a point-pattern matching
 * technique, where the pattern is contained in the vector @em lines,
 * and is searched in the vector @em peak.
 *
 * In order to work, this method just requires a rough expectation
 * value of the spectral dispersion (in Angstrom/pixel), and a line
 * catalog. The line catalog @em lines should just include lines that
 * are expected somewhere in the CCD exposure of the calibration lamp
 * (note, however, that a catalog including extra lines at its blue
 * and/or red ends is still allowed).
 *
 * Typically, the arc lamp lines candidates @em peak will include
 * light contaminations, hot pixels, and other unwanted signal,
 * but only in extreme cases does this prevent the pattern-recognition
 * algorithm from identifying all the spectral lines. The pattern
 * is detected even in the case @em peak contained more arc lamp
 * lines than actually listed in the input line catalog.
 *
 * This method is based on the assumption that the relation between
 * wavelengths and CCD positions is with good approximation @em locally
 * linear (this is always true, for any modern spectrograph).
 *
 * The ratio between consecutive intervals pairs in wavelength and in
 * pixel is invariant to linear transformations, and therefore this
 * quantity can be used in the recognition of local portions of the
 * searched pattern. All the examined sub-patterns will overlap, leading
 * to the final identification of the whole pattern, notwithstanding the
 * overall non-linearity of the relation between pixels and wavelengths.
 *
 * Ambiguous cases, caused by exceptional regularities in the pattern,
 * or by a number of undetected (but expected) peaks that disrupt the
 * pattern on the data, are recovered by linear interpolation and
 * extrapolation of the safely identified peaks.
 *
 * More details about the applied algorithm can be found in the comments
 * to the function code.
 *
 * The original cpl function contained return parameters @em seq_peaks
 * and @em seq_lines are array reporting the positions of matching peaks
 * and wavelengths in the input @em peaks and @em lines
 * vectors, however this functionality is not yet supported: and these
 * arguments should always be set to NULL or a CPL_ERROR_UNSUPPORTED_MODE would
 * be set. As a result they are not present in these bindings and are simply
 * passed NULL
 */
cpl::core::Bivector
match_positions(const cpl::core::Vector& peaks, const cpl::core::Vector& lines,
                double min_disp, double max_disp, double tolerance);

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_PPM_HPP