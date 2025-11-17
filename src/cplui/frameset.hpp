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


// Container class FrameSet which is similar in functionality to cpl_frameset,
// However this class is NOT A wrapper around cpl_frameset.
//(It is convertable to/from a cpl_frameset)
// FrameSets are just vectors of Frames.
// They may theoretically contain multiple instances of the same Frame.

#ifndef PYCPL_FRAMESET_HPP
#define PYCPL_FRAMESET_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <cpl_frame.h>
#include <cpl_frameset.h>

#include "cplui/frame.hpp"

namespace cpl
{
namespace ui
{
class FrameSet
{
 public:
  using iterator = std::vector<std::shared_ptr<Frame>>::iterator;
  using const_iterator = std::vector<std::shared_ptr<Frame>>::const_iterator;

  FrameSet();
  /**
   * @brief Parses a .sof file into a FrameSet
   */
  FrameSet(std::filesystem::path sofPath);
  FrameSet(cpl_frameset* external);
  FrameSet(std::vector<std::shared_ptr<Frame>> vec);
  ~FrameSet() = default;  // Default call destructor of vector, which in turn
                          // calls destructor of the frames

  int size();

  void append(std::shared_ptr<Frame> frame);
  bool contains(cpl_frame* frame);
  std::string dump() const;

  bool operator==(FrameSet& other);
  bool operator!=(FrameSet& other);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  std::shared_ptr<Frame> get_at(int pos);
  cpl_frameset* to_cpl() const;

 private:
  std::vector<std::shared_ptr<Frame>> m_frames;
};

}  // namespace ui
}  // namespace cpl

#endif  // PYCPL_FRAMESET_HPP