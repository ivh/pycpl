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


// Wrapper class for cpl_propertylist

#ifndef PYCPL_PROPERTYLIST_HPP
#define PYCPL_PROPERTYLIST_HPP

#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <cpl_propertylist.h>

#include "cplcore/property.hpp"
#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

class PropertyList
{
 public:
  PropertyList() = default;

  template <class InputIt>
  PropertyList(InputIt first, InputIt last) : m_props(first, last)
  {
  }

  /**
   * @brief Take ownership of a cpl_propertylist struct ptr
   */
  PropertyList(cpl_propertylist* to_steal) noexcept;

  /**
   * @brief Get the current size of a property list.
   *
   * The function reports the current number of elements stored in the property
   * list @em self.
   *
   * @return The property list's current size, or 0 if the list is empty. If an
   *     error occurs the function returns 0 and sets an appropriate error
   *     code.
   */
  size get_size() const;

  /**
   * @brief Check whether a property is present in a property list.
   * @param name The property name to look up.
   *
   * The function searches the property list @em self for a property with
   * the name @em name and reports whether it was found or not.
   *
   * @return The function returns 1 if the property is present, or 0 otherwise.
   *     If an error occurs the function returns 0 and sets an appropriate
   *     error code.
   */
  bool has(std::string_view name) const;

  /**
   * @brief Access property list elements by index.
   * @param position Index of the element to retrieve.
   *
   * The function returns a handle for the property list element, the property,
   * with the index @em position. Numbering of property list elements extends
   * from 0 to @b PropertyList::get_size() - 1. If @em position is less than 0
   * or greater equal than @b PropertyList::get_size() the function throws
   *
   * @throws IllegalInputError
   *
   * @return The function returns the property with index @em position, or @c
   * NULL if @em position is out of range.
   */
  const Property& get(long position) const;

  /**
   * @brief Access property list elements by index.
   * @param position Index of the element to retrieve.
   *
   * The function returns a handle for the property list element, the property,
   * with the index @em position. Numbering of property list elements extends
   * from 0 to @b PropertyList::get_size() - 1. If @em position is less than 0
   * or greater equal than @b PropertyList::get_size() the function throws
   *
   * @throws IllegalInputError
   *
   * @return The function returns the property with index @em position, or @c
   * NULL if @em position is out of range.
   */
  Property& get(long position);

  /**
   * @brief Access property list elements by property name.
   * @param name The name of the property to retrieve.
   *
   * The function returns a handle to the property list element, the property,
   * with the name @em name. If more than one property exist with the same
   *
   * @return The function returns the property with name @em name, or @c NULL
   *     if it does not exist.
   */
  std::optional<std::reference_wrapper<const Property>>
  get(std::string_view name) const;

  /**
   * @brief Access property list elements by property name.
   * @param name The name of the property to retrieve.
   *
   * The function returns a handle to the property list element, the property,
   * with the name @em name. If more than one property exist with the same
   *
   * @return The function returns the property with name @em name, or @c NULL
   *     if it does not exist.
   */
  std::optional<std::reference_wrapper<Property>> get(std::string_view name);

  /**
   * @brief Access property list elements by regex on the name of a property
   * @param regexp Regex to match names against
   * @param invert Return non-matching properties instead of matchine ones
   *
   * Returns the first Property whos name matches the given regex
   */
  std::optional<std::reference_wrapper<Property>>
  get_regexp(const std::string& regexp, bool invert);

  /**
   * @brief Append a property list..
   * @param other The property list to append.
   *
   * The function appends the property list @em other to the property list
   *
   */
  void append(const PropertyList& other);

  /**
   * @brief Append a value to the property list..
   * @param name The name to set for the new property
   * @param value The value
   *
   * The function appends the value @em other to the property list
   *
   * Uses variant to support all CPL cpl_propertylist_append_* types while
   * allowing type inference from the Python side
   *
   */
  void append(std::string name, Property::value_type value);
  /**
   * @brief Erase the given property from the property list
   * @param position Index of element to erase.
   *
   * The property is destroyed if not reference elsewhere.  Numbering of
   * property list elements extends from 0 to @b PropertyList::get_size() - 1.
   * If @em position is less than 0 or greater equal than @b
   * PropertyList::get_size() the function throws
   *
   * @return On success the function returns the number of erased entries. If
   *     an error occurs the function returns 0 and an appropriate error
   *     code is set.
   *
   * @throws IllegalInputError
   */
  int erase(long position);

  /**
   * @brief Erase the given property from a property list.
   * @param name Name of the property to erase.
   *
   * The function searches the property with the name @em name in the property
   * list @em self and removes it. The property is destroyed. If @em self
   * contains multiple duplicates of a property named @em name, only the
   * first one is erased.
   *
   * @return On success the function returns the number of erased entries. If
   *     an error occurs the function returns 0 and an appropriate error
   *     code is set.
   */
  int erase(std::string_view name);

  /**
   * @brief Erase all properties with name matching a given regular expression.
   * @param regexp Regular expression.
   * @param invert Flag inverting the sense of matching.
   *
   * The function searches for all the properties matching in the list
   *
   * The function expects POSIX 1003.2 compliant extended regular expressions.
   *
   * @return On success the function returns the number of erased entries or 0
   * if no entries are erased. If an error occurs the function returns -1 and an
   *     appropriate error code is set. In CPL versions earlier than 5.0, the
   *     return value in case of error is 0.
   */
  int erase_regexp(const std::string& regexp, bool invert);

  /**
   * @brief Remove all properties from a property list.
   *
   * The function removes all properties from @em self. Each property
   * is properly deallocated. After calling this function @em self is
   * empty.
   *
   */
  void clear();

  /**
   * @brief Append a property to a property list
   * @param property The property to append
   *
   * This function creates a new property and appends it to the end of a
   * property list. It will not check if the property already exists.
   *
   */
  void append(const Property& property);

  /**
   * @brief Prepend a property to a property list
   * @param property The property to prepend
   *
   * This function creates a new property and prepends it to the beginning of a
   * property list. It will not check if the property already exists.
   *
   */
  void prepend(const Property& property);

  /**
   * @brief Insert a property into a property list at the given position
   * @param here Name indicating the position where to insert the property
   * @param property The property to insert
   *
   * The function creates a new property and inserts it into the property list
   * Numbering of property
   * list elements extends from 0 to @b PropertyList::get_size() - 1. If @em
   * position is less than 0 or greater than @b PropertyList::get_size() the
   * function throws
   *
   * @throws IllegalInputError
   */
  void insert(long position, const Property& property);

  /**
   * @brief Insert a property into a property list at the given position
   * @param here Name indicating the position where to insert the property
   * @param property The property to insert
   *
   * The function creates a new property and inserts it into the property list
   *
   * @return True if insertion succeeded, false if insertion position not found.
   */
  bool insert(std::string_view here, const Property& property);

  /**
   * @brief Insert a property into a property list after the given position
   * @param after Name of the property after which to insert the property
   * @param property The property to insert
   *
   * The function creates a new property and inserts it into the property list
   *
   * @return True if insertion succeeded, false if insertion position not found.
   */
  bool insert_after(std::string_view after, const Property& property);

  /**
   * The comparison function, compare to determine the order of two deque
   * elements. The comparison function @em compare must return an integer less
   * than, equal or greater than zero if the first argument passed to it is
   * found, respectively, to be less than, equal, or be greater than the second
   * argument.
   */
  using compare_func =
      std::function<int(const Property& first, const Property& second)>;

  /**
   * @brief Sort a property list.
   * @param compare The function used to compare two properties.
   *
   * The function sorts the property list @em self in place, using the function
   * This function uses the stdlib function qsort().
   *
   * The function @em compare must be of the type PropertyList::compare_func.
   *
   */
  void sort(PropertyList::compare_func compare);

  /**
   * @brief Save a property list to a FITS file
   * @param filename Name of the file to write
   * @param mode The desired output options (combined with bitwise or)
   *
   * This function saves a property list to a FITS file, using cfitsio.
   * The data unit is empty.
   *
   * Supported output modes are CPL_IO_CREATE (create a new file) and
   * CPL_IO_EXTEND  (append to an existing file)
   *
   * @return Reference to this.
   */
  void save(const std::filesystem::path& filename, unsigned mode) const;

  /**
   * @brief Print a property list to a string.
   *
   * This function is mainly intended for debug purposes.
   *
   * @return String with the propertylist contents.
   */
  std::string dump() const;

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
  ptr() const;

  /*
   Following from here, these functions/types/members are made to look like C++
   containers
  */
  typedef typename std::deque<Property>::iterator iterator;
  typedef typename std::deque<Property>::const_iterator const_iterator;
  typedef typename std::deque<Property>::reverse_iterator reverse_iterator;
  typedef typename std::deque<Property>::const_reverse_iterator
      const_reverse_iterator;

  typedef Property value_type;
  typedef Property& reference;
  typedef size difference_type;
  typedef size size_type;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

  reverse_iterator rbegin();
  const_reverse_iterator rbegin() const;
  reverse_iterator rend();
  const_reverse_iterator rend() const;

  void push_back(const Property& prop) { append(prop); }

  void push_front(const Property& prop) { prepend(prop); }

  void insert(iterator pos, const Property& prop)
  {
    insert(pos - begin(), prop);
  }

  void insert(const_iterator pos, const Property& prop)
  {
    insert(pos - begin(), prop);
  }

  void pop_back() { erase(get_size() - 1); }

  void pop_front() { erase(0); }

  iterator erase(iterator pos)
  {
    int idx = pos - begin();
    erase(pos - begin());
    return begin() + idx;
  }

  iterator erase(const_iterator pos)
  {
    int idx = pos - begin();
    erase(pos - begin());
    return begin() + idx;
  }

 private:
  std::deque<Property> m_props;
};

/**
 * @brief Create a property list from a file.
 * @param name Name of the input file.
 * @param position Index of the data set to read.
 *
 * The function reads the properties of the data set with index @em position
 * from the file @em name.
 *
 * Currently only the FITS file format is supported. The property list is
 * created by reading the FITS keywords from extension @em position. The
 * numbering of the data sections starts from 0.
 * When creating the property list from a FITS header, any keyword without
 * a value such as undefined keywords, are not transformed into
 * a property. In the case of float or double (complex) keywords, there is no
 * way to identify the type returned by CFITSIO, therefore this function will
 * always load them as double (complex).
 *
 * @return The function returns the newly created property list or @c NULL if an
 *     error occurred.
 */
PropertyList
load_propertylist(const std::filesystem::path& name, size position);

/**
 * @brief Create a filtered property list from a file.
 * @param name Name of the input file.
 * @param position Index of the data set to read.
 * @param regexp Regular expression used to filter properties.
 * @param invert Flag inverting the sense of matching property names.
 *
 * The function reads all properties of the data set with index @em position
 * with matching names from the file @em name. If the flag @em invert is zero,
 * all properties whose names match the regular expression @em regexp are
 * read. If @em invert is set to a non-zero value, all properties with
 * names not matching @em regexp are read rather. The function expects
 * POSIX 1003.2 compliant extended regular expressions.
 *
 * Currently only the FITS file format is supported. The property list is
 * created by reading the FITS keywords from extension @em position.
 * The numbering of the data sections starts from 0.
 *
 * When creating the property list from a FITS header, any keyword without
 * a value such as undefined keywords, are not transformed into
 * a property. In the case of float or double (complex) keywords, there is no
 * way to identify the type returned by CFITSIO, therefore this function will
 * always load them as double (complex).
 *
 * FITS format specific keyword prefixes (e.g. @c HIERARCH) must
 * not be part of the given pattern string @em regexp, but only the actual
 * FITS keyword name may be given.
 *
 * @return The function returns the newly created property list or @c NULL if an
 *     error occurred.
 */
PropertyList
load_propertylist_regexp(const std::filesystem::path& name, size position,
                         const std::string& regexp, bool invert);

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_PROPERTYLIST_HPP