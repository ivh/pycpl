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

#include "cplui/frameset.hpp"

#include <wordexp.h>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "cplcore/error.hpp"

namespace cpl
{
namespace ui
{
FrameSet::FrameSet() {}  // No args will create a new empty frameset

// Generate our C++ frameset from a cpl_frameset
FrameSet::FrameSet(cpl_frameset* external)
{
  cpl_frameset_iterator* it = cpl_frameset_iterator_new(external);
  cpl_frame* frame = NULL;

  while ((frame = cpl_frameset_iterator_get(it)) != NULL) {
    append(std::make_shared<Frame>(frame));

    // cpl_frameset_iterator_advance will raise a CPL error
    // when the end of the list is encountered.
    // This will code ignores said error:
    cpl_errorstate status = cpl_errorstate_get();
    cpl_frameset_iterator_advance(it, 1);
    if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE) {
      cpl_errorstate_set(status);
    }
  }
  cpl_frameset_iterator_delete(it);
};

FrameSet::FrameSet(std::vector<std::shared_ptr<Frame>> vec) : m_frames(vec) {}

void
FrameSet::append(std::shared_ptr<Frame> frame)
{
  m_frames.push_back(frame);
}

FrameSet::FrameSet(std::filesystem::path sofPath)
{
  std::ifstream ifs(sofPath);
  std::string line;
  // If attempt to read sof fails, then it doesn't exist
  if (ifs.fail()) {
    throw(cpl::core::FileNotFoundError(PYCPL_ERROR_LOCATION, sofPath));
  }
  // For files in sof
  while (std::getline(ifs, line)) {
    int commentPos = line.find("#");
    line = line.substr(0, commentPos);
    // Ignore initial whitespace, blank lines & comments. Start from
    // the first non-whitespace character
    std::string::size_type start_pos = line.find_first_not_of(" \t,.;\n");
    // Skip the line if it is empty or a comment
    if (start_pos == std::string::npos) {
      continue;
    }

    // Split the line into the first 3 segments based on whitespace
    std::istringstream iss(line);
    std::string split[3];  // File, tag, group
    int entries = 0;
    for (std::string s; iss >> s && entries < 3; split[entries++] = s) {
    }
    int res;
    char* path;
    wordexp_t wresult;
    res = wordexp(split[0].c_str(), &wresult, 0);  // expand the $VAR/file
    if (res == 0) {
      path = *wresult.we_wordv;  // use 1st and only result word
      if (path == NULL)
        res = 1;
    }
    if (res != 0) {
      continue;  // If can't replace just don't add the frame
    }
    std::string fullPath;
    if (path[0] != '/') {  // Not an absolute path, convert to one
      fullPath = std::filesystem::canonical(sofPath).parent_path().u8string() +
                 "/" + std::string(path);
    } else {
      fullPath = std::string(path);
    }
    cpl_frame_group frameGroup;
    if (split[2] == "RAW") {
      frameGroup = CPL_FRAME_GROUP_RAW;
    } else if (split[2] == "CALIB") {
      frameGroup = CPL_FRAME_GROUP_CALIB;
    } else {
      frameGroup = CPL_FRAME_GROUP_NONE;
    }
    std::shared_ptr<Frame> frame(new Frame(fullPath, split[1], frameGroup));
    m_frames.push_back(frame);
    wordfree(&wresult);
  }
}

bool
FrameSet::operator==(FrameSet& other)
{
  if (size() != other.size()) {
    return false;
  }

  // Frame-wise comparison
  for (int i = 0; i < size(); ++i) {
    if (*get_at(i) != *other.get_at(i)) {
      return false;
    }
  }

  return true;
}

bool
FrameSet::operator!=(FrameSet& other)
{
  return !operator==(other);
}

FrameSet::iterator
FrameSet::begin()
{
  return m_frames.begin();
}

FrameSet::iterator
FrameSet::end()
{
  return m_frames.end();
}

FrameSet::const_iterator
FrameSet::begin() const
{
  return m_frames.begin();
}

FrameSet::const_iterator
FrameSet::end() const
{
  return m_frames.end();
}

std::shared_ptr<Frame>
FrameSet::get_at(int pos)
{
  return m_frames.at(pos);
}

int
FrameSet::size()
{
  return m_frames.size();
};

std::string
FrameSet::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl_frameset* fs = this->to_cpl();
  cpl::core::Error::throw_errors_with(cpl_frameset_dump, fs, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

bool
FrameSet::contains(cpl_frame* frame)
{
  for (std::shared_ptr<Frame> f : m_frames) {
    if (f->m_interface == frame) {
      return true;
    }
  }
  return false;
};

// Creates a copy of the frameset in cpl_frameset form. Used for passing input
// frames into recipes that are made to read cpl_frameset
cpl_frameset*
FrameSet::to_cpl() const
{
  cpl_frameset* set = cpl_frameset_new();
  for (std::shared_ptr<Frame> f : m_frames) {
    cpl_frameset_insert(
        set, cpl_frame_duplicate(f->m_interface));  // Insert duplicate of frame
                                                    // into our new cpl_frameset
  }
  return set;
}

}  // namespace ui
}  // namespace cpl
