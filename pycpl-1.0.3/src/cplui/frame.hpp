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

#ifndef PYCPL_FRAME_HPP
#define PYCPL_FRAME_HPP

#include <filesystem>
#include <memory>
#include <string>

#include <cpl_frame.h>

namespace cpl
{
namespace ui
{
// FIXME: What is this supposed to be used for? Is this
//        an unfinished development or dead, and obsolete
//        code. Likely the former, but check!
//        See also frame.cpp!
#if 0 
class FileNotFoundException : public std::exception
{
  std::filesystem::path m_filename;

 public:
  FileNotFoundException(const std::filesystem::path filename);
  virtual const char* what() const throw();
};
#endif

class Frame
{
  // None values set as default
 public:
  Frame(std::filesystem::path filename, std::string tag = "",
        cpl_frame_group group = CPL_FRAME_GROUP_NONE,
        cpl_frame_level level = CPL_FRAME_LEVEL_NONE,
        cpl_frame_type type = CPL_FRAME_TYPE_NONE);

  Frame(cpl_frame* external);
  ~Frame();
  std::filesystem::path get_filename();
  void set_filename(std::filesystem::path filename);
  std::string get_tag();
  void set_tag(std::string tag);
  cpl_frame_group get_group();
  void set_group(cpl_frame_group group);
  cpl_frame_level get_level();
  void set_level(cpl_frame_level level);
  cpl_frame_type get_type();
  void set_type(cpl_frame_type type);
  std::string dump() const;
  // There is also cpl_frame_get_nextensions, number of extensions in the file:
  // not sure if we need it at the moment?

  bool operator==(Frame& other);
  bool operator!=(Frame& other);

  std::shared_ptr<Frame> duplicate();
  void leak();

  const cpl_frame* ptr() const;
  // What are the number of extensions in cpl frames?
 private:
  cpl_frame* m_interface;
  friend class FrameSet;
};

}  // namespace ui
}  // namespace cpl

#endif  // PYCPL_FRAME_HPP