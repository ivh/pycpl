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

#ifndef PYCPL_WCS_HPP
#define PYCPL_WCS_HPP

#include <exception>
#include <string>
#include <tuple>
#include <vector>

#include <cpl_array.h>
#include <cpl_wcs.h>

#include "cplcore/matrix.hpp"
#include "cplcore/propertylist.hpp"

namespace cpl
{
namespace drs
{

class WCSLibError : public std::exception
{
 public:
  WCSLibError(cpl_array* status_arr);
  ~WCSLibError();
  std::string message;
  std::vector<std::tuple<int, std::string>> error_list;
  virtual const char* what() const throw();
  std::vector<std::tuple<int, std::string>> getList();
};

class WCS
{
 public:
  WCS(cpl_wcs* to_steal);
  /**
   * @brief Create a wcs structure by parsing a propertylist.
   * @param plist The input propertylist
   *
   * The function allocates memory for a WCS structure. A pointer to the WCSLIB
   * header information is created by parsing the FITS WCS keywords from the
   * header of a file. A few ancillary items are also filled in.
   *
   * It is allowed to pass a cpl_propertylist with a valid WCS structure and
   * NAXIS = 0, such a propertylist can be created by cpl_wcs_platesol().
   * In this case a cpl_wcs object is returned for which the dimensional
   * information (accessible via cpl_wcs_get_image_dims()) will be NULL.
   *
   * The returned property must be destroyed using the wcs destructor
   *
   * Now used as the primary constructor for a WCS object
   * @return The newly created and filled cpl_wcs object
   */
  WCS(const cpl::core::PropertyList& plist);

  /**
   * @brief Destroy a WCS structure
   *
   */
  ~WCS();

  /**
   * @brief Convert between physical and world coordinates.
   * @param from The input coordinate matrix
   * @param to The output coordinate matrix
   * @param status The output status array
   * @param transform The transformation mode
   *
   * This function converts between several types of coordinates. These include:
   * -- physical coordinates: The physical location on a detector (i.e.
   * pixel coordinates)
   * -- world coordinates: The real astronomical coordinate system for the
   * observations. This may be spectral, celestial,
   * time, etc.
   * -- standard coordinates: These are an intermediate relative coordinate
   * representation, defined as a distance from
   * a reference point in the natural units of the
   * world coordinate system. Any defined projection
   * geometry will have already been included in
   * the definition of standard coordinates.
   *
   * The supported conversion modes are:
   * -- CPL_WCS_PHYS2WORLD:  Converts input physical to world coordinates
   * -- CPL_WCS_WORLD2PHYS:  Converts input world to physical coordinates
   * -- CPL_WCS_WORLD2STD:   Converts input world to standard coordinates
   * -- CPL_WCS_PHYS2STD:    Converts input physical to standard coordinates
   *
   * The input cpl_matrix @b from has to be filled with coordinates. The number
   * of rows equals the number of objects and the number of columns has to be
   * equal to the value of the NAXIS keyword in the @b wcs structure. The same
   * convention is used for the output cpl_matrix @b to. For example, if an
   * image contains NAXIS = 2 and 100 stars with positions X,Y, the new matrix
   * will be created:
   *
   * The output matrix and status arrays will be allocated here, and thus will
   * need to be freed by the calling routine. The status array is used to flag
   * input coordinates where there has been some sort of failure in the
   * transformation.
   *
   * @return Reference to this.
   */
  cpl::core::Matrix
  convert(const cpl::core::Matrix& from, cpl_wcs_trans_mode transform) const;

  /**
   * @brief Do a 2d plate solution given physical and celestial coordinates
   * @param ilist The input property list containing the first pass WCS
   * @param cel The celestial coordinate matrix
   * @param xy The physical coordinate matrix
   * @param niter The number of fitting iterations
   * @param thresh The threshold for the fitting rejection cycle
   * @param fitmode The fitting mode (see below)
   * @param outmode The output mode (see below)
   * @param olist The output property list containing the new WCS
   *
   * This function allows for the following type of fits:
   * -- CPL_WCS_PLATESOL_4:  Fit for zero point, 1 scale and 1 rotation.
   * -- CPL_WCS_PLATESOL_6:  Fit for zero point, 2 scales, 1 rotation, 1 shear.
   *
   * This function allows the zeropoint to be defined by shifting either the
   * physical or the celestial coordinates of the reference point:
   * -- CPL_WCS_MV_CRVAL: Keeps the physical point fixed and shifts the
   * celestial
   * -- CPL_WCS_MV_CRPIX: Keeps the celestial point fixed and shifts the
   * physical
   *
   * The output property list contains WCS relevant information only.
   *
   * The matrices @em cel, and @em xy have to be set up in the same way as it
   * is required for @b cpl_wcs_convert(). See the documentation of
   *
   */
  static cpl::core::PropertyList
  platesol(const cpl::core::PropertyList& ilist, const cpl::core::Matrix& cel,
           const cpl::core::Matrix& xy, int niter, float thresh,
           cpl_wcs_platesol_fitmode fitmode, cpl_wcs_platesol_outmode outmode);

  /**
   * @brief Accessor to get the dimensionality of the image associated with a
   * WCS
   *
   * @return The image dimensionality,
   *     or zero on error
   */
  int get_image_naxis() const;

  /**
   * @brief Accessor to get the axis lengths of the image associated with a WCS
   *
   * @return An int vector with the image axis sizes
   *
   * Returns int vector as cpl_wcs-test.c asserts the return array from this
   * functions is of CPL_TYPE_INT
   */
  std::vector<int> get_image_dims() const;

  /**
   * @brief Accessor to get the CRVAL vector for a WCS
   *
   * @return double vector of  CRVALia keyvalues for each coord axis,
   * Returns double vector as cpl_wcs-test.c asserts the return array from this
   * functions is of CPL_TYPE_DOUBLE
   */
  std::vector<double> get_crval() const;

  /**
   * @brief Accessor to get the CRPIX vector for a WCS
   *
   * @return A double vector with the CRPIXja keyvalues for each pixel axis,
   * Returns double vector as cpl_wcs-test.c asserts the return array from this
   * functions is of CPL_TYPE_DOUBLE
   */
  std::vector<double> get_crpix() const;

  /**
   * @brief Accessor to get the CTYPE vector for a WCS
   *
   * @return A handle to an array with the CTYPEja keyvalues for each pixel
   * axis, or NULL on error.
   */
  std::vector<std::string> get_ctype() const;

  /**
   * @brief Accessor to get the CUNIT vector for a WCS
   *
   * @return A handle to an array with the CUNITja keyvalues for each pixel
   * axis, or NULL on error.
   */
  std::vector<std::string> get_cunit() const;  // TODO: cpl seems to return null

  /**
   * @brief Accessor to get the CD matrix for a WCS
   *
   * @return A handle to a matrix with the CDi_ja linear transformation matrix,
   *     or NULL on error.
   */
  cpl::core::Matrix get_cd() const;  // TODO: cpl seems to return null
 private:
  cpl_wcs* m_interface;
};

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_WCS_HPP