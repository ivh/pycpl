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


// Utility structs (not bindings) and functions for more C++-idiomatic usage
// of CPL's cpl_image, cpl_mask, and related functions.

#include "cplcore/coords.hpp"

namespace cpl
{
namespace core
{
std::pair<size, size>
cpl_coord(size x, size y)
{
  // WARNING: Modifications to this must be replicated in EXPAND_WINDOW
  return std::make_pair(x + 1, y + 1);
}

std::pair<size, size>
cpl_to_coord(size x, size y)
{
  return std::make_pair(x - 1, y - 1);
}

bool
Window::operator==(const Window& other) const noexcept
{
  return llx == other.llx && lly == other.lly && urx == other.urx &&
         ury == other.ury;
}

bool
Window::operator!=(const Window& other) const noexcept
{
  return llx != other.llx || lly != other.lly || urx != other.urx ||
         ury != other.ury;
}

std::ostream&
operator<<(std::ostream& os, const Window& other) noexcept
{
  os << "Window(" << other.llx << "," << other.lly << "," << other.urx << ","
     << other.ury << ")";
  return os;
}

const Window Window::All{
    // ll{x,y} is > ur{x,y} (closer to 0)
    // And I just pick arbitrary numbers so it's not close to a
    // value that might accidentally get in there
    //
    // HOWEVER keep in mind that pybind does wierd things where
    // passing a Window::All as a default arg doesn't work because
    // the python PyLong fails to cast to an int for this struct
    -1891,
    -1891,
    -9012,
    -9012,
};

}  // namespace core
}  // namespace cpl
