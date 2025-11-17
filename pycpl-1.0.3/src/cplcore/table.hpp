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

#ifndef PYCPL_TABLE_HPP
#define PYCPL_TABLE_HPP

#include <complex>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <cpl_table.h>

#include "cplcore/propertylist.hpp"
#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{
class Table
{
 public:
  /**
   * @brief Take ownership of a cpl_table struct ptr
   */
  Table(cpl_table* to_steal) noexcept;

  /**
   * @brief Create an empty table structure.
   * @param length Number of rows in table.
   *
   * This function allocates and initialises memory for a table data container.
   * A new table is created with no columns, but the size of the columns that
   * will be created is defined in advance, to ensure that all columns will
   * be created with the same length. All table rows are marked a priori as
   * selected. This should be considered the normal status of a table, as
   * long as no row selection has been applied to it.
   *
   * @return Pointer to new table.
   */
  Table(size length);

  /**
   * @brief Create an empty column in a table.
   * @param name Name of the new column.
   * @param type Type of the new column.
   *
   * This function allocates memory for a new column of specified @em type,
   * excluding @em array types (for creating a column of arrays use the
   * function @c cpl_table_new_column_array(), where the column depth
   * must also be specified). The new column name must be different from
   * any other column name in the table. All the elements of the new column
   * are marked as invalid.
   */
  void new_column(const std::string& name, cpl_type type);

  /**
   * @brief Create an empty column of arrays in a table.
   * @param name Name of the new column.
   * @param type Type of the new column.
   * @param depth Depth of the new column.
   *
   * This function allocates memory for a new column of specified array
   */
  void new_column_array(const std::string& name, cpl_type type, size depth);

  /**
   * @brief Create in table a new @em integer column obtained from existing
   * data.
   * @param data Existing data buffer.
   * @param name Name of the new column.
   *
   * This function creates a new column of type @c CPL_TYPE_INT that will
   * encapsulate the given data. The size of the input data array is not
   * checked in any way, and it is expected to match the number of rows
   * assigned to the given table. The pointed data values are all taken
   * as valid: invalid values should be marked using the functions
   */
  void wrap_int(std::vector<int>& data, const std::string& name);

  // /**
  //  * @brief Create in table a new @em long @em long column obtained from
  //  existing data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_LONG_LONG
  //  * that will encapsulate the given data. See the description of
  //  *
  //  * @return Reference to this, which has been modified.
  //  */

  void wrap_long_long(std::vector<long long>& data, const std::string& name);

  // /**
  //  * @brief Create in table a new @em float column obtained from existing
  //  data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_FLOAT
  //  * that will encapsulate the given data. See the description of
  //  *
  //  * @return Reference to this, which has been modified.
  //  */
  void wrap_float(std::vector<float> data, const std::string& name);

  // /**
  //  * @brief Create in table a new @em double column obtained from existing
  //  data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_DOUBLE
  //  * that will encapsulate the given data. See the description of
  //  *
  //  * @return Reference to this, which has been modified.
  //  */
  void wrap_double(std::vector<double>& data, const std::string& name);

  // /**
  //  * @brief Create in table a new @em float complex column obtained from
  //  existing data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_FLOAT_COMPLEX
  //  * that will encapsulate the given data. See the description of
  //  *
  //  * @return Reference to this, which has been modified.
  //  */
  void wrap_float_complex(std::vector<std::complex<float>> data,
                          const std::string& name);


  // /**
  //  * @brief Create in table a new @em double complex column from existing
  //  data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_FLOAT_COMPLEX
  //  * that will encapsulate the given data. See the description of
  //  *
  //  * @return Reference to this, which has been modified.
  //  */
  void wrap_double_complex(std::vector<std::complex<double>> data,
                           const std::string& name);

  // /**
  //  * @brief Create in table a new @em string column obtained from existing
  //  data.
  //  * @param data Existing data buffer.
  //  * @param name Name of the new column.
  //  *
  //  * This function creates a new column of type @c CPL_TYPE_STRING that will
  //  * encapsulate the given data. See the description of @c
  //  cpl_table_wrap_int()
  //  * for further details, especially with regard to memory managment. In the
  //  * specific case of @em string columns the described restrictions applies
  //  * also to the single column elements (strings). To deallocate specific
  //  * column elements the functions @c cpl_table_set_invalid() and
  //  *
  //  * @return Reference to this, which has been modified.
  //  */
  void
  wrap_string(const std::vector<std::string>& data, const std::string& name);


  // /**
  //  * @brief Unwrap a table column
  //  * @param name Name of the column.
  //  *
  //  * This function deallocates all the memory associated to a table column,
  //  * with the exception of its data buffer. This type of destructor
  //  * should be used on columns created with the @c cpl_table_wrap_<type>()
  //  * constructors, if the data buffer specified then was not allocated
  //  * using the functions of the @c cpl_memory module. In such a case, the
  //  * data buffer should be deallocated separately. See the documentation
  //  * of the functions @c cpl_table_wrap_<type>().
  //  *
  //  * @return Pointer to internal data buffer.
  //  */
  // void* unwrap(const std::string& name);

  /**
   * @brief Give to a table the same structure of another table.
   * @param mtable Pointer to model table.
   *
   * This function assignes to a columnless table the same column structure
   * (names, types, units) of a given model table. All columns are physically
   * created in the new table, and they are initialised to contain just
   * invalid elements.
   */
  void copy_structure(const Table& other);

  /**
   * @brief Delete a table.
   *
   * This function deletes a table, releasing all the memory associated
   * to it, including any existing column.
   */
  ~Table();

  /**
   * @brief Get the number of rows in a table.
   *
   * Get the number of rows in a table.
   *
   * @return Number of rows in the table.
   */
  size get_nrow() const;

  /**
   * @brief Get the number of columns in a table.
   *
   * Get the number of columns in a table.
   *
   * @return Number of columns in the table.
   */
  size get_ncol() const;

  /**
   * @brief Get the type of a table column.
   * @param name Column name.
   *
   * Get the type of a column.
   *
   * @throws InvalidTypeError if the string, any column, or table was NULL
   * @return Column type
   */
  cpl_type get_column_type(const std::string& name) const;

  /**
   * @brief Give a new unit to a table column.
   * @param name Column name.
   * @param unit New unit or NULL for unitless
   *
   * The input unit string is duplicated before being used as the column
   * unit. If @em unit is a @c NULL pointer, the column will be unitless.
   * The unit associated to a column has no effect on any operation performed
   * on columns, and it must be considered just an optional description of
   * the content of a column. It is however saved to a FITS file when using
   * cpl_table_save().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_column_unit(const std::string& name, const std::string* unit);

  /**
   * @brief Get the unit of a table column.
   * @param name Column name.
   *
   * Return the unit of a column if present, otherwise a NULL pointer is
   * returned.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Unit of column, or empty optional if it had no unit
   */
  std::optional<std::string> get_column_unit(const std::string& name) const;

  std::pair<std::complex<double>, int>
  get_complex_double(const std::string& name, size row);

  std::pair<std::complex<float>, int>
  get_complex_float(const std::string& name, size row);

  /**
   * @brief Give a new format to a table column.
   * @param name Column name.
   * @param format New format or NULL for default
   *
   * The input format string is duplicated before being used as the column
   * format. If @em format is a @c NULL pointer, "%%s" will be used if
   * the column is of type @c CPL_TYPE_STRING, "% 1.5e" if the column is
   * of type @c CPL_TYPE_FLOAT or @c CPL_TYPE_DOUBLE, and "% 7d" if it is
   * of type @c CPL_TYPE_INT. The format associated to a column has no
   * effect on any operation performed on columns, and it is used just
   * in the @c printf() calls made while printing a table using the
   * function @c cpl_table_dump(). This information is lost after saving
   * the table in FITS format using @c cpl_table_save().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_column_format(const std::string& name, const std::string* format);

  /**
   * @brief Get the format of a table column.
   * @param name Column name.
   *
   * Return the format of a column.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Format of column.
   */
  std::string get_column_format(const std::string& name) const;

  /**
   * @brief Modify depth of a column of arrays
   * @param name Column name.
   * @param depth New column depth.
   *
   * This function is applicable just to columns of arrays. The contents
   * of the arrays in the specified column will be unchanged up to the
   * lesser of the new and old depths. If the depth is increased, the
   * extra array elements would be flagged as invalid. The pointers to
   * array data may change, therefore pointers previously retrieved by
   * calling @c cpl_array_get_data_int(), @c cpl_array_get_data_string(),
   * etc. should be discarded.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_column_depth(const std::string& name, size depth);

  /**
   * @brief Get the depth of a table column.
   * @param name Column name.
   *
   * Get the depth of a column. Columns of type @em array always have positive
   * depth, while columns listing numbers or character strings have depth 0.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Column depth
   */
  size get_column_depth(const std::string& name) const;

  /**
   * @brief Get the number of dimensions of a table column of arrays.
   * @param name Column name.
   *
   * Get the number of dimensions of a column. If a column is not an array
   * column, or if it has no dimensions, 1 is returned.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Column number of dimensions, or 0 in case of failure.
   */
  size get_column_dimensions(const std::string& name) const;

  /**
   * @brief Set the dimensions of a table column of arrays.
   * @param name Column name.
   * @param dimensions Array containing the sizes of the column dimensions
   *
   * Set the number of dimensions of a column. If the @em dimensions array
   * has size less than 2, nothing is done and no error is returned.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @throws IncompatibleInputError if the Product of dimensions != column depth
   * @throws TypeMismatchError if the column isn't an array column
   * (CPL_TYPE_POINTER)
   */
  void set_column_dimensions(const std::string& name,
                             const std::vector<int>& dimensions);

  /**
   * @brief Get size of one dimension of a table column of arrays.
   * @param name Column name.
   * @param indx Indicate dimension to query (0 = x, 1 = y, 2 = z, etc.).
   *
   * Get the size of one dimension of a column. If a column is not an array
   * column, or if it has no dimensions, 1 is returned.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @throws AcecssOutOfRange if the indx is <0 or >= number of dimensions
   * @throws UnsupportedModeError if the column isn't an array column
   * (CPL_TYPE_POINTER)
   * @return Size of queried dimension of the column.
   */
  size get_column_dimension(const std::string& name, size indx) const;

  /**
   * @brief Write a value to a numerical table column element.
   * @param name Name of table column to access.
   * @param row Position where to write value.
   * @param value Value to write.
   *
   * Write a value to a numerical column element. The value is cast to the
   * accessed column type according to the C casting rules. The written value
   * is automatically marked as valid. To invalidate a column value use
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set(const std::string& name, size row, double value);

  void
  set_complex(const std::string& name, size row, std::complex<double> value);

  /**
   * @brief Write a character string to a @em string table column element.
   * @param name Name of table column to access.
   * @param row Position where to write the character string.
   * @param value Character string to write.
   *
   * Write a string to a table column of type @c CPL_TYPE_STRING. The written
   * value can also be a @c NULL pointer, that is equivalent to a call to
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_string(const std::string& name, size row, const std::string& value);

  /**
   * @brief Write an array to an @em array table column element.
   * @param name Name of table column to access.
   * @param row Position where to write the array.
   * @param array Array to write.
   *
   * Write an array to a table column of type @em array. The written
   * value can also be a @c NULL pointer, that is equivalent to a call
   * to @c cpl_table_set_invalid(). Note that the array is copied,
   * therefore the original can be modified without affecting the
   * table element. To "plug" an array directly into a table element,
   * use the function @c cpl_table_get_data_array(). Beware that the
   * "plugged" array must have the same type and depth declared for
   * the column.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_array(const std::string& name, size row, const cpl_array* array);

  /**
   * @brief Write a value to a numerical column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a numerical column segment. The value is cast
   * to the type of the accessed column according to the C casting rules.
   * The written values are automatically marked as valid. To invalidate
   * a column interval use @c cpl_table_set_column_invalid() instead.
   * If the sum of @em start and @em count exceeds the number of table
   * rows, the column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window(const std::string& name, size start, size count,
                          double value);

  /**
   * @brief Write a value to an @em integer column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to an @em integer column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_int(const std::string& name, size start, size count,
                              int value);

  /**
   * @brief Write a value to an @em long @em long column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to an @em long @em long column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_long_long(const std::string& name, size start,
                                    size count, long long value);

  /**
   * @brief Write a value to a @em float column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a @em float column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_float(const std::string& name, size start, size count,
                                float value);

  /**
   * @brief Write a value to a @em double column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a @em double column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_double(const std::string& name, size start,
                                 size count, double value);

  /**
   * @brief Write a value to a complex column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a complex column segment. The value is cast
   * to the type of the accessed column according to the C casting rules.
   * The written values are automatically marked as valid. To invalidate
   * a column interval use @c cpl_table_set_column_invalid() instead.
   * If the sum of @em start and @em count exceeds the number of table
   * rows, the column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_complex(const std::string& name, size start,
                                  size count, std::complex<double> value);

  /**
   * @brief Write a value to a @em float complex column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a @em float complex column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_float_complex(const std::string& name, size start,
                                        size count, std::complex<float> value);

  /**
   * @brief Write a value to a @em double complex column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the value.
   * @param count Number of values to write.
   * @param value Value to write.
   *
   * Write the same value to a @em double complex column segment. The written
   * values are automatically marked as valid. To invalidate a column
   * interval use @c cpl_table_set_column_invalid() instead. If the sum
   * of @em start and @em count exceeds the number of table rows, the
   * column is filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void
  fill_column_window_double_complex(const std::string& name, size start,
                                    size count, std::complex<double> value);

  /**
   * @brief Write a character string to a @em string column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the character string.
   * @param count Number of strings to write.
   * @param value Character string to write.
   *
   * Write the same value to a @em string column segment. If the input string
   * is not a @c NULL pointer, it is duplicated for each accessed column
   * element. If the input string is a @c NULL pointer, this call is equivalent
   * to a call to @c cpl_table_set_column_invalid(). If the sum of @em start
   * and @em count exceeds the number of rows in the table, the column is
   * filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_string(const std::string& name, size start,
                                 size count, const std::string& value);

  /**
   * @brief Write an array to an @em array column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin to write the array.
   * @param count Number of arrays to write.
   * @param array Array to write.
   *
   * Write the same array to a segment of an array column. If the input array
   * is not a @c NULL pointer, it is duplicated for each accessed column
   * element. If the input array is a @c NULL pointer, this call is equivalent
   * to a call to @c cpl_table_set_column_invalid(). If the sum of @em start
   * and @em count exceeds the number of rows in the table, the column is
   * filled up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_column_window_array(const std::string& name, size start, size count,
                                const cpl_array* array);

  /**
   * @brief Copy existing data to a table @em integer column.
   * @param name Name of the column.
   * @param data Existing data buffer.
   *
   * The input data values are copied to the specified column. The size of the
   * input array is not checked in any way, and it is expected to be compatible
   * with the number of rows in the given table. The copied data values are
   * all taken as valid: invalid values should be marked using the functions
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void copy_data_int(const std::string& name, std::vector<int>& data);
  //
  //     /**
  //      * @brief Copy existing data to a table @em long @em long column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void
  copy_data_long_long(const std::string& name, std::vector<long long>& data);

  //     /**
  //      * @brief Copy existing data to a table @em float column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void copy_data_float(const std::string& name, std::vector<float>& data);

  //     /**
  //      * @brief Copy existing data to a table @em double column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void copy_data_double(const std::string& name, std::vector<double>& data);

  //     /**
  //      * @brief Copy existing data to a table @em float complex column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void copy_data_float_complex(const std::string& name,
                               std::vector<std::complex<float>>& data);

  //     /**
  //      * @brief Copy existing data to a table @em double complex column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void copy_data_double_complex(const std::string& name,
                                std::vector<std::complex<double>>& data);


  //     /**
  //      * @brief Copy existing data to a table @em string column.
  //      * @param name Name of the column.
  //      * @param data Existing data buffer.
  //      *
  //      * See the description of @c cpl_table_copy_data_int() for details.
  //      * In the particular case of a string column, it should be noted that
  //      * the data are copied in-depth, i.e., also the pointed strings are
  //      * duplicated. Strings contained in the existing table column are
  //      * deallocated before being replaced by the new ones.
  //      *
  //      * @throws DataNotFoundError if the column of the given name wasn't
  //      found.
  //      * @return Reference to this, which has been modified.
  //      */
  void
  copy_data_string(const std::string& name, std::vector<std::string>& data);

  /**
   * @brief Shift the position of numeric or complex column values.
   * @param name Name of table column to shift.
   * @param shift Shift column values by so many rows.
   *
   * The position of all column values is shifted by the specified amount.
   * If @em shift is positive, all values will be moved toward the bottom
   * of the column, otherwise toward its top. In either case as many column
   * elements as the amount of the @em shift will be left undefined, either
   * at the top or at the bottom of the column according to the direction
   * of the shift. These column elements will be marked as invalid. This
   * function is applicable just to numeric and complex columns, and not
   * to strings and or array types. The selection flags are always set back
   * to "all selected" after this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void shift_column(const std::string& name, size shift);

  /**
   * @brief Flag a column element as invalid.
   * @param name Name of table column to access.
   * @param row Row where to write a @em null.
   *
   * In the case of either a @em string or an @em array column, the
   * corresponding string or array is set free and its pointer is set
   * to @c NULL. For other data types, the corresponding table element
   * is flagged internally as invalid.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_invalid(const std::string& name, size row);

  /**
   * @brief Invalidate a column segment.
   * @param name Name of table column to access.
   * @param start Position where to begin invalidation.
   * @param count Number of column elements to invalidate.
   *
   * All the column elements in the specified interval are invalidated.
   * In the case of either a @em string or an @em array column, the
   * corresponding strings or arrays are set free. If the sum of @em start
   * and @em count exceeds the number of rows in the table, the column is
   * invalidated up to its end.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void set_column_invalid(const std::string& name, size start, size count);

  /**
   * @brief Check if a column element is valid.
   * @param name Name of table column to access.
   * @param row Column element to examine.
   *
   * Check if a column element is valid.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return 1 if the column element is valid, 0 if invalid
   */
  bool is_valid(const std::string& name, size row) const;

  /**
   * @brief Count number of invalid values in a table column.
   * @param name Name of table column to examine.
   *
   * Count number of invalid elements in a table column.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Number of invalid elements in a table column
   */
  size count_invalid(const std::string& name) const;

  /**
   * @brief Check if a column contains at least one invalid value.
   * @param name Name of table column to access.
   *
   * Check if there are invalid elements in a column. In case of columns
   * of arrays, invalid values within an array are not considered: an
   * invalid element here means that an array element is not allocated,
   * i.e., it is a @c NULL pointer. In order to detect invalid elements
   * within an array element, this element must be extracted using
   * the function @c cpl_table_get_array(), and then use the function
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return 1 if the column contains at least one invalid element, 0 if not.
   */
  bool has_invalid(const std::string& name) const;

  /**
   * @brief Check if a column contains at least one valid value.
   * @param name Name of table column to access.
   *
   * Check if there are valid elements in a column. In case of columns
   * of arrays, invalid values within an array are not considered: an
   * invalid element here means that an array element is not allocated,
   * i.e., it is a @c NULL pointer. In order to detect valid elements
   * within an array element, this element must be extracted using
   * the function @c cpl_table_get_array(), and then use the function
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return 1 if the column contains at least one valid value, 0 if not.
   */
  bool has_valid(const std::string& name) const;

  /**
   * @brief Write a numerical value to invalid @em integer column elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * In general, a numeric column element that is flagged as invalid is
   * undefined and should not be read. It is however sometimes convenient to
   * read such values, f.ex. via calls to @c cpl_table_get_data_int() and
   * memcpy(). In order to avoid that such usage causes uninitialized memory to
   * be read, the invalid elements may be set to a value specified by a call to
   * this function. Note that only existing invalid elements will be filled as
   * indicated: new invalid column elements would still have their actual
   * values left undefined. Also, any further processing of the column would
   * not take care of maintaining the assigned value to a given invalid column
   * element: therefore the value should be applied just before it is actually
   * needed.
   * An invalid numerical column element remains invalid after this call,
   * and the usual method of checking whether the element is invalid or not
   * should still be used.
   * This function can be applied also to columns of arrays of integers. In
   * this case the call will cause the array element to be flagged as valid.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_invalid_int(const std::string& name, int code);

  /**
   * @brief Write a numerical value to invalid @em long @em long column
   * elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_invalid_long_long(const std::string& name, long long code);

  /**
   * @brief Write a numerical value to invalid @em float column elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_invalid_float(const std::string& name, float code);

  /**
   * @brief Write a numerical value to invalid @em double column elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_invalid_double(const std::string& name, double code);

  /**
   * @brief Write a numerical value to invalid @em float complex column
   * elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void
  fill_invalid_float_complex(const std::string& name, std::complex<float> code);

  /**
   * @brief Write a numerical value to invalid @em double complex column
   * elements.
   * @param name Column name.
   * @param code Value to write to invalid column elements.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void fill_invalid_double_complex(const std::string& name,
                                   std::complex<double> code);

  /**
   * @brief Delete a column from a table.
   * @param name Name of table column to delete.
   *
   * Delete a column from a table. If the table is left without columns,
   * also the selection flags are lost.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void erase_column(const std::string& name);

  /**
   * @brief Move a column from a table to another.
   * @param name Name of column to move.
   * @param from_table Source table.
   *
   * Move a column from a table to another.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void move_column(const std::string& name, Table& from_table);

  /**
   * @brief Copy a column from a table to another.
   * @param to_name New name of copied column.
   * @param from_table Source table.
   * @param from_name Name of column to copy.
   *
   * Copy a column from a table to another. The column is duplicated. A column
   * may be duplicated also within the same table.
   *
   * @throws DataNotFoundError if the column named from_name wasn't found in
   * from_table
   * @throws IncompatibleInputError if the number of rows of both tables doesn't
   * match
   * @throws IllegalOutputError if the column named to_name already exists in
   * to_table
   */
  void duplicate_column(const std::string& to_name, const Table& from_table,
                        const std::string& from_name);

  /**
   * @brief Rename a table column.
   * @param from_name Name of table column to rename.
   * @param to_name New name of column.
   *
   * This function is used to change the name of a column.
   *
   * @throws DataNotFoundError if the column named from_name wasn't found.
   * @throws IllegalOutputError if the column named to_name already exists.
   */
  void name_column(const std::string& from_name, const std::string& to_name);

  /**
   * @brief Check if a column with a given name exists.
   * @param name Name of table column.
   *
   * Check if a column with a given name exists in the specified table.
   *
   * @return 1 if column exists, 0 if column doesn't exist.
   */
  int has_column(const std::string& name) const;

  /**
   * @brief Get table columns names.
   *
   * The returned CPL array of strings should be finally destroyed
   * using cpl_array_delete().
   *
   * @return Array of table columns names.
   */
  std::vector<std::string> get_column_names() const;

  /**
   * @brief Resize a table to a new number of rows.
   * @param new_length New number of rows in table.
   *
   * The contents of the columns will be unchanged up to the lesser of the
   * new and old sizes. If the table is expanded, the extra table rows would
   * just contain invalid elements. The table selection flags are set back
   * to "all selected". The pointer to column data may change, therefore
   * pointers previously retrieved by calling @c cpl_table_get_data_int(),
   */
  void set_size(size new_length);

  /**
   * @brief Make a copy of a table.
   *
   * The copy operation is done "in depth": columns data are duplicated
   * too, not just their pointers. Also the selection flags of the original
   * table are transferred to the new table.
   *
   * @return Pointer to the new table.
   */
  Table(const Table& other);

  /**
   * @brief Create a table from a section of another table.
   * @param start First row to be copied to new table.
   * @param count Number of rows to be copied.
   *
   * A number of consecutive rows are copied from an input table to a
   * newly created table. The new table will have the same structure of
   * the original table (see function @c cpl_table_compare_structure() ).
   * If the sum of @em start and @em count goes beyond the end of the
   * input table, rows are copied up to the end. All the rows of the
   * new table are selected, i.e., existing selection flags are not
   * transferred from the old table to the new one.
   *
   * @return Pointer to the new table.
   */
  Table extract(size start, size count) const;

  /**
   * @brief Create a new table from the selected rows of another table.
   *
   * A new table is created, containing a copy of all the selected
   * rows of the input table. In the output table all rows are selected.
   *
   * @return Pointer to new table.
   */
  Table extract_selected() const;

  /**
   * @brief Get array of indexes to selected table rows
   *
   * Get array of indexes to selected table rows. If no rows are selected,
   * an array of size zero is returned. The returned array must be deleted
   * using @c cpl_array_delete().
   *
   * @return Indexes to selected table rows.
   */
  cpl_array* where_selected() const;

  /**
   * @brief Delete the selected rows of a table.
   *
   * A portion of the table data is physically removed. The pointer to
   * column data may change, therefore pointers previously retrieved by
   * calling @c cpl_table_get_data_int(), @c cpl_table_get_data_string(),
   * etc., should be discarded. The table selection flags are set back to
   * "all selected".
   */
  void erase_selected();

  /**
   * @brief Delete a table segment.
   * @param start First row to delete.
   * @param count Number of rows to delete.
   *
   * A portion of the table data is physically removed. The pointers to column
   * data may change, therefore pointers previously retrieved by calling
   */
  void erase_window(size start, size count);

  /**
   * @brief Insert a segment of rows into table data.
   * @param start Row where to insert the segment.
   * @param count Length of the segment.
   *
   * Insert a segment of empty rows, just containing invalid elements.
   * Setting @em start to a number greater than the column length is legal,
   * and has the effect of appending extra rows at the end of the table:
   * this is equivalent to expanding the table using @c cpl_table_set_size().
   * The input @em column may also have zero length. The pointers to column
   * data values may change, therefore pointers previously retrieved by
   * calling @c cpl_table_get_data_int(), @c cpl_table_get_data_string(),
   * etc., should be discarded. The table selection flags are set back to
   * "all selected".
   */
  void insert_window(size start, size count);

  /**
   * @brief Compare the structure of two tables.
   * @param table2 Pointer to another table.
   *
   * Two tables have the same structure if they have the same number
   * of columns, with the same names, the same types, and the same units.
   * The order of the columns is not relevant.
   *
   * @return 0 if the tables have the same structure, 1 otherwise.
   *     In case of error, -1 is returned.
   */
  bool compare_structure(const Table& table2) const;

  /**
   * @brief Merge two tables.
   * @param insert_table Table to be inserted in the target table.
   * @param row Row where to insert the insert table.
   *
   * The input tables must have the same structure, as defined by the function
   */
  void insert(const Table& insert_table, size row);

  /**
   * @brief Cast a numeric or complex column to a new numeric or complex type
   * column.
   * @param from_name Name of table column to cast.
   * @param to_name Name of new table column.
   * @param type Type of new table column.
   *
   * A new column of the specified type is created, and the content of the
   * given numeric column is cast to the new type. If the input column type
   * is identical to the specified type the column is duplicated as is done
   * by the function @c cpl_table_duplicate_column(). Note that a column of
   * arrays is always cast to another column of arrays of the specified type,
   * unless it has depth 1. Consistently, a column of numbers can be cast
   * to a column of arrays of depth 1.
   * Here is a complete summary of how any (legal) @em type specification
   * would be interpreted, depending on the type of the input column:
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * specified type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth > 1)
   * specified type = CPL_TYPE_XXX
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   * specified type = CPL_TYPE_XXX
   * to_name   type = CPL_TYPE_XXX
   *
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   *
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   *
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_YYY
   * to_name   type = CPL_TYPE_YYY
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER
   * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth > 1)
   * specified type = CPL_TYPE_YYY
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   *
   * from_name type = CPL_TYPE_XXX | CPL_TYPE_POINTER (depth = 1)
   * specified type = CPL_TYPE_YYY
   * to_name   type = CPL_TYPE_YYY
   *
   * from_name type = CPL_TYPE_XXX
   * specified type = CPL_TYPE_YYY | CPL_TYPE_POINTER
   * to_name   type = CPL_TYPE_YYY | CPL_TYPE_POINTER (depth = 1)
   *
   * @throws DataNotFoundError if the column named from_name wasn't found.
   * @throws IllegalOutputError if the column named to_name already exists and
   * from_name != to_name.
   * @throws IllegalInputError if given type isn't INT, LONG, LONG_LONG, FLOAT,
   * DOUBLE, or COMPLEX
   */
  void cast_column(const std::string& from_name, const std::string& to_name,
                   cpl_type type);

  /**
   * @brief Add the values of two numeric or complex table columns.
   * @param to_name Name of target column.
   * @param from_name Name of source column.
   *
   * The columns are summed element by element, and the result of the sum is
   * stored in the target column. The columns' types may differ, and in that
   * case the operation would be performed using the standard C upcasting
   * rules, with a final cast of the result to the target column type.
   * Invalid elements are propagated consistently: if either or both members
   * of the sum are invalid, the result will be invalid too. Underflows and
   * overflows are ignored.
   */
  void add_columns(const std::string& to_name, const std::string& from_name);

  /**
   * @brief Subtract two numeric or complex table columns.
   * @param to_name Name of target column.
   * @param from_name Name of column to be subtracted from target column.
   *
   * The columns are subtracted element by element, and the result of the
   * subtraction is stored in the target column. See the documentation of
   * the function @c cpl_table_add_columns() for further details.
   */
  void
  subtract_columns(const std::string& to_name, const std::string& from_name);

  /**
   * @brief Multiply two numeric or complex table columns.
   * @param to_name Name of target column.
   * @param from_name Name of column to be multiplied with target column.
   *
   * The columns are multiplied element by element, and the result of the
   * multiplication is stored in the target column. See the documentation of
   * the function @c cpl_table_add_columns() for further details.
   */
  void
  multiply_columns(const std::string& to_name, const std::string& from_name);

  /**
   * @brief Divide two numeric or complex table columns.
   * @param to_name Name of target column.
   * @param from_name Name of column dividing the target column.
   *
   * The columns are divided element by element, and the result of the
   * division is stored in the target column. The columns' types may
   * differ, and in that case the operation would be performed using
   * the standard C upcasting rules, with a final cast of the result
   * to the target column type. Invalid elements are propagated consistently:
   * if either or both members of the division are invalid, the result
   * will be invalid too. Underflows and overflows are ignored, but a
   * division by exactly zero will set an invalid column element.
   */
  void divide_columns(const std::string& to_name, const std::string& from_name);

  /**
   * @brief Add a constant value to a numerical or complex column.
   * @param name Column name.
   * @param value Value to add.
   *
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * are not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void add_scalar(const std::string& name, double value);

  /**
   * @brief Subtract a constant value from a numerical or complex column.
   * @param name Column name.
   * @param value Value to subtract.
   *
   * See the description of the function @c cpl_table_add_scalar().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void subtract_scalar(const std::string& name, double value);

  /**
   * @brief Multiply a numerical or complex column by a constant.
   * @param name Column name.
   * @param value Multiplication factor.
   *
   * See the description of the function @c cpl_table_add_scalar().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void multiply_scalar(const std::string& name, double value);

  /**
   * @brief Divide a numerical or complex column by a constant.
   * @param name Column name.
   * @param value Divisor value.
   *
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void divide_scalar(const std::string& name, double value);

  /**
   * @brief Add a constant complex value to a numerical or complex column.
   * @param name Column name.
   * @param value Value to add.
   *
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * are not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void add_scalar_complex(const std::string& name, std::complex<double> value);

  /**
   * @brief Subtract a constant complex value from a numerical or complex
   * column.
   * @param name Column name.
   * @param value Value to subtract.
   *
   * See the description of the function @c cpl_table_add_scalar_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void
  subtract_scalar_complex(const std::string& name, std::complex<double> value);

  /**
   * @brief Multiply a numerical or complex column by a complex constant.
   * @param name Column name.
   * @param value Multiplication factor.
   *
   * See the description of the function @c cpl_table_add_scalar_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void
  multiply_scalar_complex(const std::string& name, std::complex<double> value);

  /**
   * @brief Divide a numerical or complex column by a complex constant.
   * @param name Column name.
   * @param value Divisor value.
   *
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void
  divide_scalar_complex(const std::string& name, std::complex<double> value);

  /**
   * @brief Compute the absolute value of column values.
   * @param name Table column name.
   *
   * Each column element is replaced by its absolute value.
   * Invalid elements are not modified by this operation.
   * If the column is complex, its type will be turned to
   * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
   * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
   * and any pointer retrieved by calling @c cpl_table_get_data_float_complex()
   * and @c cpl_array_get_data_double_complex() should be discarded.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void abs_column(const std::string& name);

  /**
   * @brief Compute the logarithm of column values.
   * @param name Table column name.
   * @param base Logarithm base.
   *
   * Each column element is replaced by its logarithm in the specified base.
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * not modified by this operation, but zero or negative elements are
   * invalidated by this operation. In case of complex numbers, values
   * very close to the origin may cause an overflow. The imaginary part
   * of the result is chosen in the interval [-pi/ln(base),pi/ln(base)],
   * so it should be kept in mind that doing the logarithm of exponential
   * of a complex number will not always express the phase angle with the
   * same number. For instance, the exponential in base 2 of (5.00, 5.00)
   * is (-30.33, -10.19), and the logarithm in base 2 of the latter will
   * be expressed as (5.00, -4.06).
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void logarithm_column(const std::string& name, double base);

  /**
   * @brief Compute the power of numerical column values.
   * @param name Name of column of numerical type
   * @param exponent Constant exponent.
   *
   * Each column element is replaced by its power to the specified exponent.
   * For float and float complex the operation is performed in single precision,
   * otherwise it is performed in double precision and then rounded if the
   * column is of an integer type. Results that would or do cause domain errors
   * or overflow are marked as invalid.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void power_column(const std::string& name, double exponent);

  /**
   * @brief Compute the exponential of column values.
   * @param name Column name.
   * @param base Exponential base.
   *
   * Each column element is replaced by its exponential in the specified base.
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void exponential_column(const std::string& name, double base);

  /**
   * @brief Compute the complex conjugate of column values.
   * @param name Column name.
   *
   * Each column element is replaced by its complex conjugate.
   * The operation is always performed in double precision, with a final
   * cast of the result to the target column type. Invalid elements are
   * not modified by this operation.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void conjugate_column(const std::string& name);

  /**
   * @brief Compute the real part value of table column elements.
   * @param name Column name.
   *
   * Each column element is replaced by its real party value only.
   * Invalid elements are not modified by this operation.
   * If the column is complex, its type will be turned to
   * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
   * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
   * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(),
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void real_column(const std::string& name);

  /**
   * @brief Compute the imaginary part value of table column elements.
   * @param name Column name.
   *
   * Each column element is replaced by its imaginary party value only.
   * Invalid elements are not modified by this operation.
   * If the column is complex, its type will be turned to
   * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
   * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
   * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(),
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void imag_column(const std::string& name);

  /**
   * @brief Compute the phase angle value of table column elements.
   * @param name Column name.
   *
   * Each column element is replaced by its phase angle value.
   * The phase angle will be in the range of [-pi,pi].
   * Invalid elements are not modified by this operation.
   * If the column is complex, its type will be turned to
   * real (CPL_TYPE_FLOAT_COMPLEX will be changed into CPL_TYPE_FLOAT,
   * and CPL_TYPE_DOUBLE_COMPLEX will be changed into CPL_TYPE_DOUBLE),
   * and any pointer retrieved by calling @c cpl_table_get_data_float_complex(),
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   */
  void arg_column(const std::string& name);

  /**
   * @brief Remove from a table columns and rows just containing invalid
   * elements.
   *
   * Table columns and table rows just containing invalid elements are deleted
   * from the table, i.e. a column or a row is deleted only if all of its
   * elements are invalid. The selection flags are set back to "all selected"
   * even if no rows or columns are removed. The pointers to data may change,
   * therefore pointers previously retrieved by @c cpl_table_get_data_int(),
   */
  void erase_invalid_rows();

  /**
   * @brief Remove from a table all columns just containing invalid elements,
   *     and then all rows containing at least one invalid element.
   *
   * Firstly, all columns consisting just of invalid elements are deleted
   * from the table. Next, the remaining table rows containing at least
   * one invalid element are also deleted from the table. The selection
   * flags are set back to "all selected" even if no rows or columns are
   * erased. The pointers to data may change, therefore pointers previously
   * retrieved by calling @c cpl_table_get_data_int(), etc., should be
   * discarded.
   *
   * The function is similar to the function cpl_table_erase_invalid_rows(),
   * except for the criteria to remove rows containing invalid elements after
   * all invalid columns have been removed. While cpl_table_erase_invalid_rows()
   * requires all elements to be invalid in order to remove a row from the
   * table, this function requires only one (or more) elements to be invalid.
   */
  void erase_invalid();

  /**
   * @brief Get maximum value in a numerical column.
   * @param name Column name.
   *
   * See the description of the function @c cpl_table_get_column_mean().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Maximum value. See documentation of @c cpl_table_get_column_mean().
   */
  double get_column_max(const std::string& name) const;

  /**
   * @brief Get minimum value in a numerical column.
   * @param name Column name.
   *
   * See the description of the function @c cpl_table_get_column_mean().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Minimum value. See documentation of @c cpl_table_get_column_mean().
   */
  double get_column_min(const std::string& name) const;

  /**
   * @brief Get position of maximum in a numerical column.
   * @param name Column name.
   * @param row Returned position of maximum value.
   *
   * Invalid column values are excluded from the search. The @em row argument
   * will be assigned the position of the maximum value, where rows are counted
   * starting from 0. If more than one column element correspond to the max
   * value, the position with the lowest row number is returned. In case of
   * error, @em row is left untouched. The table selection flags have no
   * influence on the result.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Reference to this.
   */
  size get_column_maxpos(const std::string& name) const;

  /**
   * @brief Get position of minimum in a numerical column.
   * @param name Column name.
   * @param row Returned position of minimum value.
   *
   * See the description of the function @c cpl_table_get_column_maxpos().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Reference to this.
   */
  size get_column_minpos(const std::string& name) const;

  /**
   * @brief Compute the mean value of a numerical column.
   * @param name Column name.
   *
   * Invalid column values are excluded from the computation. The table
   * selection flags have no influence on the result.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Mean value. In case of error 0.0 is returned.
   */
  double get_column_mean(const std::string& name) const;

  /**
   * @brief
   *   Compute the mean value of a numerical or complex column.
   *
   * @param table Pointer to table.
   * @param name  Column name.
   *
   * @return Mean value.
   *
   *
   * Invalid column values are excluded from the computation. The table
   * selection flags have no influence on the result.
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @throws InvildTypeError if the column is not of a complex type
   * @return Mean value. In case of error 0.0 is returned.
   */
  std::complex<double> get_column_mean_complex(const std::string& name) const;

  /**
   * @brief Compute the median value of a numerical column.
   * @param name Column name.
   *
   * See the description of the function @c cpl_table_get_column_mean().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Median value. See documentation of @c cpl_table_get_column_mean().
   */
  double get_column_median(const std::string& name) const;

  /**
   * @brief Find the standard deviation of a table column.
   * @param name Column name.
   *
   * Invalid column values are excluded from the computation of the
   * standard deviation. If just one valid element is found, 0.0 is
   * returned but no error is set. The table selection flags have no
   * influence on the result.
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Standard deviation. See documentation of
   */
  double get_column_stdev(const std::string& name) const;

  /**
   * @brief Sort table rows according to columns values.
   * @param reflist Names of reference columns with corresponding sorting mode.
   *
   * The table rows are sorted according to the values of the specified
   * reference columns. The reference column names are listed in the input
   */
  void sort(const PropertyList& reflist);

  /**
   * @brief Select from selected rows only those within a table segment.
   * @param start First row of table segment.
   * @param count Length of segment.
   *
   * All the selected table rows that are outside the specified interval are
   * unselected. If the sum of @em start and @em count goes beyond the end
   * of the input table, rows are checked up to the end of the table. See
   * also the function @c cpl_table_or_selected_window().
   *
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_window(size start, size count);

  /**
   * @brief Select from unselected rows only those within a table segment.
   * @param start First row of table segment.
   * @param count Length of segment.
   *
   * All the unselected table rows that are within the specified interval are
   * selected. If the sum of @em start and @em count goes beyond the end of
   * the input table, rows are checked up to the end of the table. See also
   * the function @c cpl_table_and_selected_window().
   *
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_window(size start, size count);

  /**
   * @brief Select unselected table rows, and unselect selected ones.
   *
   * Select unselected table rows, and unselect selected ones.
   *
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size not_selected();

  /**
   * @brief Flag a table row as selected.
   * @param row Row to select.
   *
   * Flag a table row as selected. Any previous selection is kept.
   */
  void select_row(size row);

  /**
   * @brief Flag a table row as unselected.
   * @param row Row to unselect.
   *
   * Flag a table row as unselected. Any previous selection is kept.
   */
  void unselect_row(size row);

  /**
   * @brief Select all table rows.
   *
   * The table selection flags are reset, meaning that they are
   * all marked as selected. This is the initial state of any
   * table.
   */
  void select_all();

  /**
   * @brief Unselect all table rows.
   *
   * The table selection flags are all unset, meaning that no table
   * rows are selected.
   */
  void unselect_all();

  /**
   * @brief Select from selected table rows all rows with an invalid value in
   *     a specified column.
   * @param name Column name.
   *
   * For all the already selected table rows, all the rows containing valid
   * values at the specified column are unselected. See also the function
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_invalid(const std::string& name);

  /**
   * @brief Select from unselected table rows all rows with an invalid value in
   *     a specified column.
   * @param name Column name.
   *
   * For all the unselected table rows, all the rows containing invalid
   * values at the specified column are selected. See also the function
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_invalid(const std::string& name);

  /**
   * @brief Select from selected table rows, by comparing @em integer column
   *     values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_int(const std::string& name, cpl_table_select_operator op,
                        int value);

  /**
   * @brief Select from unselected table rows, by comparing @em integer column
   *     values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_int(const std::string& name, cpl_table_select_operator op,
                       int value);

  /**
   * @brief Select from selected table rows, by comparing @em long @em long
   * column values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_long_long(const std::string& name,
                              cpl_table_select_operator op, long long value);

  /**
   * @brief Select from unselected table rows, by comparing @em long @em long
   * column values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_long_long(const std::string& name,
                             cpl_table_select_operator op, long long value);

  /**
   * @brief Select from selected table rows, by comparing @em float column
   * values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_float(const std::string& name, cpl_table_select_operator op,
                          float value);

  /**
   * @brief Select from unselected table rows, by comparing @em float column
   *     values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_float(const std::string& name, cpl_table_select_operator op,
                         float value);

  /**
   * @brief Select from selected table rows, by comparing @em double column
   * values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_double(const std::string& name,
                           cpl_table_select_operator op, double value);

  /**
   * @brief Select from unselected table rows, by comparing @em double column
   *     values with a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_double(const std::string& name, cpl_table_select_operator op,
                          double value);

  /**
   * @brief Select from selected table rows, by comparing @em float complex
   * column values with a complex constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO.
   * If the table has no rows, no error is set, and 0 is returned. See
   * also the function @c cpl_table_or_selected_float_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_float_complex(const std::string& name,
                                  cpl_table_select_operator op,
                                  std::complex<float> value);

  /**
   * @brief Select from unselected table rows, by comparing @em float complex
   * column values with a complex constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO. If the table has no
   * rows, no error is set, and 0 is returned. See also the description
   * of the function @c cpl_table_and_selected_float_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_float_complex(const std::string& name,
                                 cpl_table_select_operator op,
                                 std::complex<float> value);

  /**
   * @brief Select from selected table rows, by comparing @em double complex
   * column values with a complex constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference value. All table rows not
   * fulfilling the comparison are unselected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO.
   * If the table has no rows, no error is set, and 0 is returned. See
   * also the function @c cpl_table_or_selected_double_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected_double_complex(const std::string& name,
                                   cpl_table_select_operator op,
                                   std::complex<double> value);

  /**
   * @brief Select from unselected table rows, by comparing @em double complex
   * column values with a complex constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param value Reference value.
   *
   * For all the unselected table rows, the values of the specified
   * column are compared with the reference value. The table rows
   * fulfilling the comparison are selected. An invalid element never
   * fulfills any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO and @c CPL_NOT_EQUAL_TO. If the table has no
   * rows, no error is set, and 0 is returned. See also the description
   * of the function @c cpl_table_and_selected_double_complex().
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_double_complex(const std::string& name,
                                  cpl_table_select_operator op,
                                  std::complex<double> value);

  /**
   * @brief Select from selected table rows, by comparing @em string column
   * values with a character string.
   * @param name Column name.
   * @param op Relational operator.
   * @param string Reference character string.
   *
   * For all the already selected table rows, the values of the specified
   * column are compared with the reference string. The comparison function
   * used is the C standard @c strcmp(), but in case the relational ops
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size
  and_selected_string(const std::string& name, cpl_table_select_operator op,
                      const std::string& string);

  /**
   * @brief Select from unselected table rows, by comparing column values with
   *     a constant.
   * @param name Column name.
   * @param op Relational operator.
   * @param string Reference value.
   *
   * For all the unselected table rows, the values of the specified column
   * are compared with the reference value. The comparison function used
   * is the C standard @c strcmp(), but in case the relational ops
   *
   * @throws DataNotFoundError if the column of the given name wasn't found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected_string(const std::string& name, cpl_table_select_operator op,
                          const std::string& string);

  /**
   * @brief Select from selected table rows, by comparing the values of two
   *     numerical columns.
   * @param name1 Name of first table column.
   * @param op Relational operator.
   * @param name2 Name of second table column.
   *
   * Either both columns must be numerical, or they both must be strings.
   * The comparison between strings is lexicographical. Comparison between
   * complex types and array types are not supported.
   *
   * For all the already selected table rows, the values of the specified
   * columns are compared. The table rows not fulfilling the comparison
   * are unselected. Invalid elements from either columns never fulfill
   * any comparison by definition. Allowed relational ops are
   *
   * @throws DataNotFoundError if the column of either of the names wasn't
   * found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size and_selected(const std::string& name1, cpl_table_select_operator op,
                    const std::string& name2);

  /**
   * @brief Select from unselected table rows, by comparing the values of two
   *     numerical columns.
   * @param name1 Name of first table column.
   * @param op Relational operator.
   * @param name2 Name of second table column.
   *
   * Either both columns must be numerical, or they both must be strings.
   * The comparison between strings is lexicographical. Comparison between
   * complex types and array types are not supported.
   *
   * Both columns must be numerical. For all the unselected table rows, the
   * values of the specified columns are compared. The table rows fulfilling
   * the comparison are selected. Invalid elements from either columns never
   * fulfill any comparison by definition. Allowed relational ops
   * are @c CPL_EQUAL_TO, @c CPL_NOT_EQUAL_TO, @c CPL_GREATER_THAN,
   *
   * @throws DataNotFoundError if the column of either of the names wasn't
   * found.
   * @return Current number of selected rows, or a negative number in case
   *     of error.
   */
  size or_selected(const std::string& name1, cpl_table_select_operator op,
                   const std::string& name2);

  /**
   * @brief Determine whether a table row is selected or not.
   * @param row Table row to check.
   *
   * Check if a table row is selected.
   *
   * @return 1 if row is selected, 0 if it is not selected, -1 in case of error.
   */
  bool is_selected(size row) const;

  /**
   * @brief Get number of selected rows in given table.
   *
   * Get number of selected rows in given table.
   *
   * @return Number of selected rows, or a negative number in case of error.
   */
  size count_selected() const;

  /**
   * @brief Describe the structure and the contents of a table in a string.
   *
   * This function is mainly intended for debug purposes. Some information
   * about the structure of a table and its contents is printed to terminal:
   *
   * - Number of columns, with their names and types
   * - Number of invalid elements for each column
   * - Number of rows and of selected rows
   *
   *
   * @return String with the table structure information.
   */
  std::string dump_structure() const;

  /**
   * @brief Print a table to a string.
   * @param start First row to print
   * @param count Number of rows to print
   *
   * This function is mainly intended for debug purposes.
   * All column elements are printed according to the column formats,
   * that may be specified for each table column with the function
   *
   * @return String containing the dump of the table contents.
   */
  std::string dump(size start, size count) const;

  /**
   * @brief Load a FITS table extension into a new @em cpl_table.
   * @param filename Name of FITS file with at least one table extension,
   * optional.
   * @param xtnum Number of extension to read, starting from 1.
   * @param check_nulls If set to 0, identified invalid values are not marked.
   *
   * The selected FITS file table extension is just read and converted into
   * the @em cpl_table conventions.
   *
   * @return New table, or @c NULL in case of failure.
   */
  static Table load(const std::string& filename, int xtnum, bool check_nulls);

  /**
   * @brief Load part of a FITS table extension into a new @em cpl_table.
   * @param filename Name of FITS file with at least one table extension.
   * @param xtnum Number of extension to read, starting from 1.
   * @param check_nulls If set to 0, identified invalid values are not marked.
   * @param selcol Array with the names of the columns to extract.
   * @param firstrow First table row to extract.
   * @param nrow Number of rows to extract.
   *
   * The selected FITS file table extension is just read in the specified
   * columns and rows intervals, and converted into the @em cpl_table
   * conventions. If @em selcol is NULL, all columns are selected.
   *
   * @return New table, or @c NULL in case of failure.
   */
  static Table
  load_window(const std::string& filename, int xtnum, bool check_nulls,
              const std::vector<std::string>& selcol_vector, size firstrow,
              size nrow);

  /**
   * @brief Save a @em cpl_table to a FITS file.
   * @param pheader Primary header entries.
   * @param header Table header entries.
   * @param filename Name of output FITS file.
   * @param mode Output mode.
   *
   * This function can be used to convert a CPL table into a binary FITS
   * table extension. If the @em mode is set to @c CPL_IO_CREATE, a new
   * FITS file will be created containing an empty primary array, with
   * just one FITS table extension. An existing (and writable) FITS file
   * with the same name would be overwritten. If the @em mode flag is set
   * to @c CPL_IO_EXTEND, a new table extension would be appended to an
   * existing FITS file. If @em mode is set to @c CPL_IO_APPEND it is possible
   * to add rows to the last FITS table extension of the output FITS file.
   *
   * Note that the modes @c CPL_IO_EXTEND and @c CPL_IO_APPEND require that
   * the target file must be writable (and do not take for granted that a file
   * is writable just because it was created by the same application,
   * as this depends on the system @em umask).
   *
   * When using the mode @c CPL_IO_APPEND additional requirements must be
   * fulfilled, which are that the column properties like type, format, units,
   * etc. must match as the properties of the FITS table extension to which the
   * rows should be added exactly. In particular this means that both tables use
   * the same null value representation for integral type columns!
   *
   * Two property lists may be passed to this function, both
   * optionally. The first property list, @em pheader, is just used if
   * the @em mode is set to @c CPL_IO_CREATE, and it is assumed to
   * contain entries for the FITS file primary header. In @em pheader any
   * property name related to the FITS convention, as SIMPLE, BITPIX,
   * NAXIS, EXTEND, BLOCKED, and END, are ignored: such entries would
   * be written anyway to the primary header and set to some standard
   * values.
   *
   * If a @c NULL @em pheader is passed, the primary array would be created
   * with just such entries, that are mandatory in any regular FITS file.
   * The second property list, @em header, is assumed to contain entries
   * for the FITS table extension header. In this property list any
   * property name related to the FITS convention, as XTENSION, BITPIX,
   * NAXIS, PCOUNT, GCOUNT, and END, and to the table structure, as
   * TFIELDS, TTYPEi, TUNITi, TDISPi, TNULLi, TFORMi, would be ignored:
   * such entries are always computed internally, to guarantee their
   * consistency with the actual table structure. A DATE keyword
   * containing the date of table creation in ISO8601 format is also
   * added automatically.
   *
   * - Invalid values in columns of type @c CPL_TYPE_FLOAT or
   *
   * - Invalid values in columns of type @c CPL_TYPE_INT  and
   *
   * - Using the mode @c CPL_IO_APPEND requires that the column properties of
   * the table to be appended are compared to the column properties of the
   * target FITS extension for each call, which introduces a certain overhead.
   * This means that appending a single table row at a time may not be
   * efficient and is not recommended. Rather than writing one row at a
   * time one should write table chunks containing a suitable number or rows.
   *
   * @return Reference to this.
   */
  void save(std::optional<cpl::core::PropertyList> pheader,
            const cpl::core::PropertyList& header, const std::string& filename,
            unsigned mode) const;
  /**
   * @brief
   *   Read a value from an @em integer column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return tuple containing Integer value read. Bool value 1 in case of an
   * invalid table element, or in case of error bool 0 is always returned and
   * the flag.
   *
   * @note
   *   For automatic conversion (always to type @em double), use the function
   *   @c cpl_table_get().
   */
  std::pair<int, int> get_int(const std::string& name, size row);

  /**
   * @brief
   *   Read a value from a @em float column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return Tuple containing Float value read. Bool value 1 in case of an
   * invalid table element, or in case of error bool 0 is always returned and
   * the flag.
   *
   *
   * Read a value from a column of type @c cpl.core.Type.FLOAT . See the
   * documentation of function get_int().
   */

  std::pair<float, int> get_float(const std::string& name, size row);
  /**
   * @brief
   *   Read a value from a @em double column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return Tuple containing double value read. Bool value 1 in case of an
   * invalid table element, or in case of error bool 0 is always returned and
   * the flag.
   *
   *
   * Read a value from a column of type @c cpl.core.Type.DOUBLE. See the
   * documentation of function get_int().
   */

  std::pair<double, int> get_double(const std::string& name, size row);
  /**
   * @brief
   *   Read a value from a numerical column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to be accessed.
   * @param row     Position of element to be read.
   *
   * @return Value read. In case of invalid table element, or in case of
   *   error, 0.0 is returned.
   *
   *
   * Rows are counted starting from 0. The @em null flag is used to
   * indicate whether the accessed table element is valid (0) or
   * invalid (1). The @em null flag also signals an error condition (-1).
   * The @em null argument can be left to @c NULL.
   */
  std::pair<double, int> get(const std::string& name, size row);

  /**
   * @brief
   *   Read a value from a @em long @em long column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return Tuple containing Long Long Integer value read. Bool value 1 in case
   * of an invalid table element, or in case of error bool 0 is always returned
   * and the flag.
   *
   *
   * Read a value from a column of type @c cpl.core.Type.LONG. See the
   * documentation of function get_int().
   */
  std::pair<long long, int> get_long_long(const std::string& name, size row);

  /**
   * @brief
   *   Read a value from a @em string column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return copy of the string in a std::string object.
   *
   *
   * Read a value from a column of type @c CPL_TYPE_STRING.
   * Rows are counted starting from 0.
   *
   */
  std::pair<std::string, int> get_string(const std::string& name, size row);

  /**
   * @brief
   *   Read an array from an @em array column.
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return Pointer to array. In case of an invalid column element, or in
   *   case of error, a @c NULL pointer is always returned.
   *

   *
   * Read a value from a column of any array type.
   * Rows are counted starting from 0.
   *
   * @note
   *   The returned array is a pointer to a table element, not its copy.
   *   Its manipulation will directly affect that element, while changing
   *   that element using @c cpl_table_set_array() will turn it into garbage.
   *   Therefore, if a real copy of an array column element is required,
   *   this function should be called as an argument of the function
   *   @c cpl_array_duplicate().
   */
  std::pair<const cpl_array*, int> get_array(const std::string& name, size row);

  /**
   * @brief
   *   template function for getting the data pointer to any numerical column
   * (including complex)
   *
   * @param table   Pointer to table.
   * @param name    Name of table column to access.
   * @param row     Position of element to be read.
   *
   * @return Pointer the column data
   *
   *
   * Get a pointer storing all data in column name.
   * Rows are counted starting from 0.
   *  The data buffer elements corresponding to invalid column elements would
   * in general contain garbage. To avoid this, the appropriate
   * cpl_table_fill... function should be called just before this function,
   * assigning to all the invalid column elements an @em ad @em hoc numerical
   * value. See the description of function @c fill_invalid_int() for further
   * details.
   *
   * @note
   *   Use at your own risk: direct manipulation of column data rules out
   *   any check performed by the table object interface, and may introduce
   *   inconsistencies between the information maintained internally, and
   *   the actual column data and structure.
   */
  template <typename T>
  std::pair<T*, int> get_data(const std::string& name);
  std::pair<char**, int> get_data_string(const std::string& name);

  std::shared_ptr<Table> duplicate();

  const cpl_table* ptr() const;
  /**
   * @brief Relieves self Table of ownership of the underlying cpl_table*
   * pointer, if it is owned.
   *
   * This is a counterpart to Table(cpl_table *to_steal);
   *
   * @note Make sure to use cpl_table_delete to delete the returned cpl_table*,
   *       or turn it back into a Table with Table(cpl_table *to_steal)
   */
  static cpl_table* unwrap(Table&& self);


 private:
  cpl_table* m_interface;
};


}  // namespace core
}  // namespace cpl

#endif  // PYCPL_TABLE_HPP