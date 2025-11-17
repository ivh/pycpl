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

#ifndef PYCPL_BIVECTOR_HPP
#define PYCPL_BIVECTOR_HPP

#include <cpl_bivector.h>

#include "cplcore/types.hpp"
#include "cplcore/vector.hpp"

/**
 * Wraps the cpl_bivector struct as a C++ class cpl::core::Biector
 * Implementing all operations that a cpl_bivector can do.
 *
 * This class is optional from the Python programmer's perspective, as they can
 * use a tuple of 2 python list, of which there should be an automatic
 * conversion to this bivector, OR they can use a tuple of 2 CPL vectors. This
 * class does, however, add methods that you can use aside from those on a tuple
 */
namespace cpl
{
namespace core
{

using size = cpl::core::size;

/**
 * A @em cpl_bivector is composed of two vectors of the same size. It can
 * be used to store 1d functions, with the x and y positions of the
 * samples, offsets in x and y or simply positions in an image.
 * This module provides among other things functions for interpolation and
 * for sorting one vector according to another.
 */
class Bivector
{
 public:
  Bivector(cpl_bivector* to_steal) noexcept;
  Bivector& operator=(const Bivector& other);
  /**
   * @brief Create a new cpl_bivector
   * @param n Positive number of points
   *
   * @return 1 newly allocated cpl_bivector
   */
  Bivector(size n);

  /**
   * @brief Create a new cpl_bivector from two cpl_vectors
   * @param x the x cpl_vector
   * @param y the y cpl_vector
   */
  Bivector(cpl::core::Vector&& x, cpl::core::Vector&& y);

  /**
   * @brief Duplicate a cpl_bivector
   *
   * @return 1 newly allocated cpl_bivector
   * @throws IllegalInputError if the input bivector contains vectors of
   */
  Bivector(const Bivector& in);

  /**
   * @brief Delete a cpl_bivector
   */
  ~Bivector() = default;

  /**
   * @brief Free memory associated to a cpl_bivector, excluding the two vectors.
   *
   * @return The 2 vectors moved out of this bivector
   */
  static std::pair<cpl::core::Vector, cpl::core::Vector>
  unwrap_vectors(Bivector&& self);

  /**
   * @brief Dump a cpl_bivector to a string
   *
   * Comment lines start with the hash character.
   *
   * @return String with the bivector contents.
   */
  std::string dump() const;

  /**
   * @brief Read a list of values from an ASCII file and create a cpl_bivector
   * @param filename Name of the input ASCII file
   *
   * The input ASCII file must contain two values per line.
   *
   * Two columns of numbers are expected in the input file.
   *
   * In addition to normal files, FIFO (see man mknod) are also supported.
   *
   * @return 1 newly allocated cpl_bivector
   * @throws FileIoError if the file cannot be read
   */
  static Bivector read(const std::filesystem::path& filename);

  /**
   * @brief Copy contents of a bivector into self's bivector
   * @param other source cpl_vector
   *
   */
  void copy(const Bivector& other);

  /**
   * @brief Get the size of the cpl_bivector
   *
   * @return The size or a negative number on error
   * @throws IllegalInputError if the input bivector contains vectors of
   */
  size get_size() const;

  /**
   * @brief Get a pointer to the x vector of the cpl_bivector
   *
   * The returned pointer refers to an already created cpl_vector.
   *
   * @return Pointer to the x vector
   */
  cpl::core::Vector& get_x();

  /**
   * @brief Get a pointer to the y vector of the cpl_bivector
   *
   * The returned pointer refers to an already created cpl_vector.
   *
   * @return Pointer to the y vector
   */
  cpl::core::Vector& get_y();

  /**
   * @brief Set the x vector of this cpl_bivector, yielding the old x vector
   * @param new_x New vector to replace x vector, moved in.
   *
   * @return Original x vector that has been replaced
   */
  cpl::core::Vector set_x(cpl::core::Vector&& new_x);

  /**
   * @brief Set the y vector of this cpl_bivector, yielding the old y vector
   * @param new_x New vector to replace y vector, moved in.
   *
   * @return Original y vector that has been replaced
   */
  cpl::core::Vector set_y(cpl::core::Vector&& new_y);

  /**
   * @brief Get a pointer to the x vector of the cpl_bivector
   *
   * @return Pointer to the x vector
   */
  const cpl::core::Vector& get_x() const;

  /**
   * @brief Get a pointer to the y vector of the cpl_bivector
   *
   * @return Pointer to the y vector
   */
  const cpl::core::Vector& get_y() const;

  /**
   * @brief Get a pointer to the x data part of the cpl_bivector
   *
   * @return Pointer to the double x array
   */
  double* get_x_data();

  /**
   * @brief Get a pointer to the y data part of the cpl_bivector
   *
   * @return Pointer to the double y array
   */
  double* get_y_data();

  /**
   * @brief Get a pointer to the x data part of the cpl_bivector
   *
   * @return Pointer to the double x array
   */
  const double* get_x_data() const;

  /**
   * @brief Get a pointer to the y data part of the cpl_bivector
   *
   * @return Pointer to the double y array
   */
  const double* get_y_data() const;

  /**
   * @brief Linear interpolation of a 1d-function
   * @param fref Reference 1d-function
   *
   * fref must have both its abscissa and ordinate defined.
   * self must have its abscissa defined and its ordinate allocated.
   *
   * The linear interpolation will be done from the values in fref to the
   * abscissa points in self.
   *
   * For each abscissa point in self, fref must either have two neigboring
   * abscissa points such that xref_i < xout_j < xref{i+1}, or a single
   * identical abscissa point, such that xref_i == xout_j.
   *
   * This is ensured by monotonely growing abscissa points in both self and fref
   * (and by min(xref) <= min(xout) and max(xout) < max(xref)).
   *
   * However, for efficiency reasons (since fref can be very long) the
   * monotonicity is only verified to the extent necessary to actually perform
   * the interpolation.
   *
   * This input requirement implies that extrapolation is not allowed.
   *
   * @throws DataNotFoundError if self has an endpoint which is out of range
   * @throws IllegalInputError if the monotonicity requirement on the 2 input
   */
  void interpolate_linear(const Bivector& fref);

  /**
   * @brief Sort a cpl_bivector
   * @param other Input cpl_bivector to sort, may equal self
   * @param dir CPL_SORT_ASCENDING or CPL_SORT_DESCENDING
   * @param mode CPL_SORT_BY_X or CPL_SORT_BY_Y
   *
   * The values in the input are sorted according to direction and mode, and the
   * result is placed self which must be of the same size as other.
   *
   * As for qsort():
   * If two members compare as equal, their order in the sorted array is
   * undefined.
   *
   * In place sorting is supported.
   *
   * @throws IncompatibleInputError if self and other have different sizes
   * @throws IllegalInputError if dir is neither CPL_SORT_DESCENDING nor
   * @throws UnsupportedModeError if self and other are the same or point to the
   */
  void sort(const Bivector& other, cpl_sort_direction dir, cpl_sort_mode mode);

  bool operator==(const Bivector& other) const;
  bool operator!=(const Bivector& other) const;

  std::unique_ptr<struct _cpl_bivector_, void (*)(cpl_bivector*)> ptr();
  std::unique_ptr<const struct _cpl_bivector_, void (*)(const cpl_bivector*)>
  ptr() const;

 private:
  cpl::core::Vector m_x;
  cpl::core::Vector m_y;
};

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_BIVECTOR_HPP