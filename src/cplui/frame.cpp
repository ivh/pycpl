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


// Aims to wrap cpl_frame structs as a C++ class.
// This wrapper will contain a pointer to the underlying CPL struct, using the
// appropriate CPL method for applying/retrieving information. Currently Missing
// nextensions. Still not certain what its for and wasn't needed to execute a
// recipe at this stage.

#include "cplui/frame.hpp"

#include <cstdio>

#include "cplcore/error.hpp"

namespace cpl
{
namespace ui
{
// FIXME: What is this supposed to be used for? Is this
//        an unfinished development or dead, and obsolete
//        code. Likely the former, but check!
//        See also frame.hpp!
#if 0 
FileNotFoundException::FileNotFoundException(
    const std::filesystem::path filename)
    : m_filename("File Not Found: " + std::string(filename)){};

const char*
FileNotFoundException::what() const throw()
{
  return m_filename.c_str();
};
#endif

Frame::Frame(cpl_frame* external) { m_interface = external; };

Frame::Frame(std::filesystem::path filename, std::string tag,
             cpl_frame_group group, cpl_frame_level level, cpl_frame_type type)
{
  m_interface = cpl_frame_new();

  if (!std::filesystem::exists(filename)) {
    throw cpl::core::FileIOError(PYCPL_ERROR_LOCATION,
                                 std::string(filename) + " could not be found");
  }
  cpl_frame_set_filename(m_interface, filename.c_str());
  cpl_frame_set_tag(m_interface, tag.c_str());
  cpl_frame_set_group(m_interface, group);
  cpl_frame_set_level(m_interface, level);
  cpl_frame_set_type(m_interface, type);
};

Frame::~Frame()
{
  if (m_interface != nullptr) {
    // If not leak()d already
    cpl_frame_delete(m_interface);
  }
};

std::filesystem::path
Frame::get_filename()
{
  return std::filesystem::path(cpl_frame_get_filename(m_interface));
}

void
Frame::set_filename(std::filesystem::path filename)
{
  cpl_frame_set_filename(m_interface, filename.c_str());
}

std::string
Frame::get_tag()
{
  return std::string(cpl_frame_get_tag(m_interface));
}

void
Frame::set_tag(std::string tag)
{
  cpl_frame_set_tag(m_interface, tag.c_str());
}

cpl_frame_group
Frame::get_group()
{
  cpl_frame_group group = cpl_frame_get_group(m_interface);
  return group;
}

void
Frame::set_group(cpl_frame_group group)
{
  cpl_frame_set_group(m_interface, group);
}

cpl_frame_level
Frame::get_level()
{
  cpl_frame_level level = cpl_frame_get_level(m_interface);
  return level;
}

void
Frame::set_level(cpl_frame_level level)
{
  cpl_frame_set_level(m_interface, level);
}

cpl_frame_type
Frame::get_type()
{
  cpl_frame_type type = cpl_frame_get_type(m_interface);
  return type;
}

void
Frame::set_type(cpl_frame_type type)
{
  cpl_frame_set_type(m_interface, type);
}

std::string
Frame::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl::core::Error::throw_errors_with(cpl_frame_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

bool
Frame::operator==(Frame& other)
{
  // Aspects that make a frame equivalent:
  // (From cpl_frame.h/cpl_frame.c)
  // are type, group, level and
  // file information (just filename)
  // and tag

  return get_filename() == other.get_filename() &&
         get_tag() == other.get_tag() && get_group() == other.get_group() &&
         get_level() == other.get_level() && get_type() == other.get_type();
}

const cpl_frame*
Frame::ptr() const
{
  return m_interface;
}

bool
Frame::operator!=(Frame& other)
{
  return !operator==(other);
}

std::shared_ptr<Frame>
Frame::duplicate()
{
  cpl_frame* dup = cpl_frame_duplicate(m_interface);
  return std::make_shared<Frame>(dup);
}

void
Frame::leak()
{
  m_interface = nullptr;
}

}  // namespace ui
}  // namespace cpl
