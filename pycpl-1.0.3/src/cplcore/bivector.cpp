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

#include "cplcore/bivector.hpp"

#include <cstdio>
#include <filesystem>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{

Bivector::Bivector(cpl_bivector* to_steal) noexcept
    : m_x(Error::throw_errors_with(cpl_bivector_get_x, to_steal)),
      m_y(Error::throw_errors_with(cpl_bivector_get_y, to_steal))
{
  Error::throw_errors_with(cpl_bivector_unwrap_vectors, to_steal);
}

Bivector&
Bivector::operator=(const Bivector& other)
{
  m_x = other.m_x;
  m_y = other.m_y;
  return *this;
}

Bivector::Bivector(size n) : m_x(n), m_y(n) {}

Bivector::Bivector(cpl::core::Vector&& x, cpl::core::Vector&& y)
    : m_x(x), m_y(y)
{
  if (m_x.get_size() != m_y.get_size()) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION, "Vector sizes mismatch");
  }
}

Bivector::Bivector(const Bivector& in) : m_x(in.get_x()), m_y(in.get_y()) {}

// FIXME: This is not used. Verify the status. Is this (part of) an unfinished
//        feature, or just dead code. In the latter case consider removing it!
#if 0
static std::pair<cpl::core::Vector, cpl::core::Vector>
unwrap_vectors(Bivector&& self)
{
  return std::make_pair(self.get_x(), self.get_y());
}
#endif

std::string
Bivector::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  auto interface = ptr();
  Error::throw_errors_with(cpl_bivector_dump, interface.get(), stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

Bivector
Bivector::read(const std::filesystem::path& filename)
{
  return Bivector(
      Error::throw_errors_with(cpl_bivector_read, filename.c_str()));
}

void
Bivector::copy(const Bivector& other)
{
  m_x.copy(other.get_x());
  m_y.copy(other.get_y());
}

size
Bivector::get_size() const
{
  size s_x = m_x.get_size();
  size s_y = m_y.get_size();

  if (s_x != s_y) {
    throw cpl::core::IllegalInputError(
        PYCPL_ERROR_LOCATION, "Bivector is made of different sized vectors");
  }

  return s_x;
}

cpl::core::Vector&
Bivector::get_x()
{
  return m_x;
}

cpl::core::Vector&
Bivector::get_y()
{
  return m_y;
}

cpl::core::Vector
Bivector::set_x(cpl::core::Vector&& new_x)
{
  cpl::core::Vector old_vec = std::move(m_x);
  m_x = new_x;
  return old_vec;
}

cpl::core::Vector
Bivector::set_y(cpl::core::Vector&& new_y)
{
  cpl::core::Vector old_vec = std::move(m_y);
  m_y = new_y;
  return old_vec;
}

const cpl::core::Vector&
Bivector::get_x() const
{
  return m_x;
}

const cpl::core::Vector&
Bivector::get_y() const
{
  return m_y;
}

double*
Bivector::get_x_data()
{
  return m_x.data();
}

double*
Bivector::get_y_data()
{
  return m_y.data();
}

const double*
Bivector::get_x_data() const
{
  return m_x.data();
}

const double*
Bivector::get_y_data() const
{
  return m_y.data();
}

void
Bivector::interpolate_linear(const Bivector& fref)
{
  auto this_interface = ptr();
  auto fref_interface = fref.ptr();
  Error::throw_errors_with(cpl_bivector_interpolate_linear,
                           this_interface.get(), fref_interface.get());
}

void
Bivector::sort(const Bivector& other, cpl_sort_direction dir,
               cpl_sort_mode mode)
{
  auto interface = ptr();
  auto other_interface = other.ptr();
  Error::throw_errors_with(cpl_bivector_sort, interface.get(),
                           other_interface.get(), dir, mode);
}

bool
Bivector::operator==(const Bivector& other) const
{
  return get_x() == other.get_x() && get_y() == other.get_y();
}

bool
Bivector::operator!=(const Bivector& other) const
{
  return !operator==(other);
}

std::unique_ptr<struct _cpl_bivector_, void (*)(cpl_bivector*)>
Bivector::ptr()
{
  cpl_bivector* ptr =
      Error::throw_errors_with(cpl_bivector_wrap_vectors, m_x.ptr(), m_y.ptr());

  return std::unique_ptr<struct _cpl_bivector_, void (*)(cpl_bivector*)>(
      ptr, cpl_bivector_unwrap_vectors);
}

namespace
{
void
cpl_bivector_unwrap_vectors_const(const cpl_bivector* m)
{
  cpl_bivector_unwrap_vectors(const_cast<cpl_bivector*>(m));
}
}  // namespace

std::unique_ptr<const struct _cpl_bivector_, void (*)(const cpl_bivector*)>
Bivector::ptr() const
{
  /*
  There is no cpl_bivector_wrap_const_vectors, because then you'd be given
  a const cpl_bivector*, which can't be deleted with
  cpl_bivector_delete/cpl_bivector_unwrap_vectors.

  However, in this C++ context we replace
  cpl_bivector_delete/cpl_bivector_unwrap_vectors with the unique_ptr and our
  above const casting cpl_bivector_unwrap_vectors_const

  This only works because unique_ptr ensures that there's only one copy (so
  deleting something while in-use doesn't happen), and so the constness of the
  pointer can be used only to signify that the vectors making up the bivector
  are not modifiable
  */
  const cpl_bivector* ptr = Error::throw_errors_with(
      cpl_bivector_wrap_vectors, const_cast<cpl_vector*>(m_x.ptr()),
      const_cast<cpl_vector*>(m_y.ptr()));

  return std::unique_ptr<const struct _cpl_bivector_,
                         void (*)(const cpl_bivector*)>(
      ptr, &cpl_bivector_unwrap_vectors_const);
}

}  // namespace core
}  // namespace cpl
