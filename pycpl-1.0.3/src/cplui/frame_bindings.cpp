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

#include "cplui/frame_bindings.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <cpl_frame.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "cpldfs/dfs.hpp"
#include "cplui/frame.hpp"
#include "cplui/frameset.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

// There is no simple, general method to convert arbitrary data
// types to a string in C++17, however this is straightforward to
// do using Python's string formatting functionality. To take
// advantage of this flexibility we implement __repr__() and
// __str__() methods by creating a Python string object and use its
// format() method to insert string representations of the required
// attributes.

static py::str
frame_to_repr(cpl::ui::Frame& frame)
{
  py::str representation =
      "cpl.ui.Frame({!r}, {!r}, cpl.ui.Frame.{}, cpl.ui.Frame.{}, "
      "cpl.ui.Frame.{})";
  return representation.format(frame.get_filename(), frame.get_tag(),
                               frame.get_group(), frame.get_level(),
                               frame.get_type());
}

static py::str
frame_to_str(cpl::ui::Frame& frame)
{
  py::str representation = "(file={}, tag={}, group={}, level={}, type={})";
  return representation.format(frame.get_filename(), frame.get_tag(),
                               frame.get_group(), frame.get_level(),
                               frame.get_type());
}

// @brief
//   Create the official string representation of a FrameSet
// @param sof             The FrameSet to convert to its textual representation
// @param frame_formatter Delegate to convert a single frame to text
// @param frame_limit     The maximum number of frames
// @param max_frames      The first and the last number of frames
// @param opening         A prefix to be prepended in front of the output
// @param closing         A suffix to be appended at the end of the output
// @return
//   A @c py::str object containing the string representation of @em sof
//
// The function converts the @c cpl::ui::FrameSet object @em sof to a
// textual representation, where each of the cpl::ui::Frame object it
// contains is converted to its textual representation by calling the
// delegate @em frame_formatter. The strings @em opening and @em closing
// may be used to add a prefix and a suffix to the concatenated textual
// representation of the @em sof content.
//
// The argument @em frame_limit is the maximum number of frames up to
// which the frame representations of all frames is shown. If the @em sof
// contains more than @em frame_limit frames, the created textual
// representation willbe truncated and contain information only for the
// first and the last @em max_frames frames in @m sof. All other frames
// are skipped, which is indicated by three dots.
static py::str
frameset_formatter(cpl::ui::FrameSet& sof,
                   std::function<py::str(cpl::ui::Frame&)> frame_formatter,
                   int frame_limit, int max_frames,
                   const std::string& opening = "[",
                   const std::string& closing = "]")
{
  std::string indent(opening.size(), ' ');
  std::ostringstream os;
  int nframe = sof.size();
  frame_limit = std::max(2 * max_frames, frame_limit);
  if (nframe == 0) {
    os << opening << closing;
  } else if (nframe == 1) {
    os << opening << frame_formatter(*sof.get_at((0))) << closing;
  } else if (nframe <= frame_limit) {
    for (int iframe = 0; iframe < nframe; ++iframe) {
      if (iframe == 0) {
        os << opening << frame_formatter(*sof.get_at(iframe)) << ",\n";
      } else if (iframe == nframe - 1) {
        os << indent << frame_formatter(*sof.get_at(iframe)) << closing;
      } else {
        os << indent << frame_formatter(*sof.get_at(iframe)) << ",\n";
      }
    }
  } else {
    for (int iframe = 0; iframe < 3; ++iframe) {
      if (iframe == 0) {
        os << opening << frame_formatter(*sof.get_at(iframe)) << ",\n";
      } else {
        os << indent << frame_formatter(*sof.get_at(iframe)) << ",\n";
      }
    }
    os << indent << "...\n";
    for (int iframe = nframe - 3; iframe < nframe; ++iframe) {
      if (iframe == nframe - 1) {
        os << indent << frame_formatter(*sof.get_at(iframe)) << closing;
      } else {
        os << indent << frame_formatter(*sof.get_at(iframe)) << ",\n";
      }
    }
  }
  return os.str();
}

// See frame_bindings.hpp for doc comment
void
bind_frames(py::module& m)
{
  // Exceptions
  // These exception(s) are registered in the bind_frames because they
  // are defined by frame.hpp, and only used by frame related code.
  // py::register_exception<cpl::ui::FileNotFoundException>(
  //     m, "FileNotFoundException");

  py::class_<cpl::ui::Frame, std::shared_ptr<cpl::ui::Frame>> frame(m, "Frame");

  frame.doc() = R"pydoc(
     A frame is a container for descriptive attributes related to a data file. The attributes are related to a data file through the 
     file name member of the frame type. Among the attributes which may be assigned to a data file is an attribute identifying the 
     type of the data stored in the file (image or table data), a classification tag indicating the kind of data the file contains 
     and an attribute denoting to which group the data file belongs (raw, processed or calibration file). For processed data a 
     processing level indicates whether the product is an temporary, intermediate or final product.
  )pydoc";

  // Enums
  py::enum_<cpl_frame_group>(frame, "FrameGroup")
      .value("NONE",
             CPL_FRAME_GROUP_NONE)  // Currently returning a none enum: if
                                    // possible replace with python type none.
                                    // Last time I tried it doesn't work as the
                                    // parameter expects char *
      .value("CALIB", CPL_FRAME_GROUP_CALIB)
      .value("PRODUCT", CPL_FRAME_GROUP_PRODUCT)
      .value("RAW", CPL_FRAME_GROUP_RAW);

  py::enum_<cpl_frame_level>(frame, "FrameLevel")
      .value("NONE", CPL_FRAME_LEVEL_NONE)
      .value("TEMPORARY", CPL_FRAME_LEVEL_TEMPORARY)
      .value("INTERMEDIATE", CPL_FRAME_LEVEL_INTERMEDIATE)
      .value("FINAL", CPL_FRAME_LEVEL_FINAL);

  py::enum_<cpl_frame_type>(frame, "FrameType")
      .value("NONE", CPL_FRAME_TYPE_NONE)
      .value("MATRIX", CPL_FRAME_TYPE_MATRIX)
      .value("IMAGE", CPL_FRAME_TYPE_IMAGE)
      .value("PAF", CPL_FRAME_TYPE_PAF)
      .value("TABLE", CPL_FRAME_TYPE_TABLE)
      .value("ANY", CPL_FRAME_TYPE_ANY);

  // Methods
  frame
      .def(py::init<std::filesystem::path, std::string, cpl_frame_group,
                    cpl_frame_level, cpl_frame_type>(),
           py::arg("file"), py::arg("tag") = "",
           py::arg("group") = CPL_FRAME_GROUP_NONE,
           py::arg("level") = CPL_FRAME_LEVEL_NONE,
           py::arg("frameType") = CPL_FRAME_TYPE_NONE,
           R"pydoc(
        Container for descriptive attributes related to a data file. The attributes are related to a data file through the
        file name member of the frame type.

        Parameters
        ----------
        file : str
          path of the data file
        group : cpl.ui.FrameGroup
          The frame group data type
        level : cpl.ui.FrameLevel
          The frame processing level
        type : cpl.ui.FrameType
          The frame type data type

        Raises
        ------
        cpl.core.FileNotFoundError
          if `file` cannot be found )pydoc")
      .def_property("file", &cpl::ui::Frame::get_filename,
                    &cpl::ui::Frame::set_filename, "str: filename of the frame")
      .def_property("tag", &cpl::ui::Frame::get_tag, &cpl::ui::Frame::set_tag,
                    "str: Category tag for the frame")
      .def_property("group", &cpl::ui::Frame::get_group,
                    &cpl::ui::Frame::set_group,
                    "cpl.ui.FrameGroup : The frame group data type.")
      .def_property("level", &cpl::ui::Frame::get_level,
                    &cpl::ui::Frame::set_level,
                    "cpl.ui.FrameLevel : The frame processing level")
      .def_property("type", &cpl::ui::Frame::get_type,
                    &cpl::ui::Frame::set_type,
                    "cpl.ui.FrameType : The frame type data type.")
      .def(
          "as_ccddata",
          [](cpl::ui::Frame& a, py::kwargs kwargs) -> py::object {
            // pybind11 equivalent of `from astropy.nddata import CCDData`
            py::object CCDData =
                py::module::import("astropy.nddata").attr("CCDData");
            // pybind11 equivalent of `ccddata = CCDData.read(self.filename,
            // **kwargs)`
            py::object ccddata =
                CCDData.attr("read")(a.get_filename(), **kwargs);
            return ccddata;
          },
          "Wrapper function of astropy's astropy.nddata.read constructor to "
          "convert the frame to astropy CCDData object. Refer to the "
          "documentation of astropy.nddata.read for more details ")
      .def(
          "as_hdulist",
          [](cpl::ui::Frame& a, py::kwargs kwargs) -> py::object {
            // pybind11 equivalent of `import astropy.io.fits as fits`
            py::object fits = py::module::import("astropy.io.fits");
            // pybind11 equivalent of `hdulist = fits.open(self.filename,
            // **kwargs)`
            py::object hdulist = fits.attr("open")(a.get_filename(), **kwargs);
            return hdulist;
          },
          "Convenience function to convert the frame to astropy HDUList "
          "object. Any kwargs passed "
          "to this function is passed down to astropy.io.fits.open. "
          "Refer to the "
          "documentation of astropy.io.fits.open for more details ")
      .def("__repr__", &frame_to_repr)
      .def("__str__", &frame_to_str)
      .def(
          "dump",
          [](cpl::ui::Frame& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump the Frame contents to a file, stdout or a string.
          
        This function is mainly intended for debug purposes.

        Parameters
        ----------
        filename : str, optional
            file path to dump frame contents to
        mode : str, optional
            File mode to save the file, default 'w' overwrites contents.
        show : bool, optional
            Send frame contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the frame contents.
        )pydoc")
      .def("__eq__",
           [](cpl::ui::Frame& self, py::object eq_arg) -> bool {
             /*
               If eq_arg were to be a cpl::ui::Frame (avoiding complication
               here), then running Frame == NotAFrame would raise a type error
               in Python, So instead, it mustb e cast manually, here, to catch
               said type error.
             */
             try {
               cpl::ui::Frame* casted_frame = eq_arg.cast<cpl::ui::Frame*>();
               return self == *casted_frame;
             }
             catch (const py::cast_error& wrong_type) {
               return false;
             }
           })
      .def(py::pickle(
          [](cpl::ui::Frame& f) -> py::tuple {
            return py::make_tuple(f.get_filename(), f.get_tag(), f.get_group(),
                                  f.get_level(), f.get_type());
          },
          [](py::tuple t) -> std::shared_ptr<cpl::ui::Frame> {
            return std::make_shared<cpl::ui::Frame>(
                t[0].cast<std::string>(), t[1].cast<std::string>(),
                t[2].cast<cpl_frame_group>(), t[3].cast<cpl_frame_level>(),
                t[4].cast<cpl_frame_type>());
          }

          ));


  py::class_<cpl::ui::FrameSet, std::shared_ptr<cpl::ui::FrameSet>> frameset(
      m, "FrameSet");

  frameset.doc() = R"pydoc(
    Frames can be stored in a frame set and retrieved by index and sequential access. Frame sets can be created, filled and saved to a ‘set of frames’ file or loaded from such a file.

    Framesets are intended to be used for passing fits file information to and from recipes. 

    Frames are accessed by index or iteration.
    )pydoc";

  frameset.def(py::init<>(), "create an empty frameset")
      .def(py::init<std::filesystem::path>(), py::arg("sof_filename"), R"pydoc(
        Generate a frameset from a given sof file. 

        A sof file contains a list of the input data. This data is specified in an sof file (which is just a text file), where each 
        input file is specified with its associated classification and category. The format of each line in the sof file is as follows:

        full-path-to-file  classification

        An example file, for the mythological "ZIMOS" instrument, might look like this:

        /home/user/data/mos/ZIMOS.2003-12-26T01:05:06.233.fits  MOS_SCIENCE
        /home/user/data/mos/ZIMOS.2003-12-26T01:26:00.251.fits  MOS_SCIENCE
        /home/user/data/mos/ZIMOS.2003-12-26T01:47:04.050.fits  MOS_SCIENCE
        /home/user/data/cal/master_bias4.fits                   MASTER_BIAS
        /home/user/data/cal/grs_LR_red.3.tfits                  GRISM_TABLE
        /home/user/gasgano/extract_table2.fits                  EXTRACT_TABLE
        /home/user/data/cal/badpixel.3.tfits                    CCD_TABLE

        For an concrete example for a specific instrument, check the documentation for that instrument.

        Optionally, a third column may be provided. Permitted values are either RAW or CALIB. This is for when a recipe does not identify 
        the type of input file, but as all ESO recipes are required to do so, this column is typically not needed. 
        
        Parameters
        ----------
        sof_filename : str
          filename of the sof file
        
        Raises
        ------
        cpl.core.FileIOError
          if the sof file cannot be found
      )pydoc")
      .def(py::init([](py::iterable frames) -> cpl::ui::FrameSet {
             cpl::ui::FrameSet self;
             for (py::handle it : frames) {
               try {
                 std::shared_ptr<cpl::ui::Frame> toInsert =
                     py::cast<std::shared_ptr<cpl::ui::Frame>>(it);
                 self.append(toInsert);
               }
               catch (const py::cast_error& wrong_type) {
                 throw py::type_error(
                     std::string("expected iterable of cpl.ui.Frame, not ") +
                     it.get_type().attr("__name__").cast<std::string>());
               }
             }
             return self;
           }),
           py::arg("frames"), R"pydoc(
        Generate a frameset object with an iterable of the cpl.ui.Frame objects

        Parameters
        ----------
        frames : iterable
          iterable container with the frames to store in the frameset
      )pydoc")
      .def("append", &cpl::ui::FrameSet::append, py::arg("frame"), R"pydoc(
        Insert a frame into the given frame set.

        The function adds the frame frame to the frame set using the
        frame's tag as key.

        Parameters
        ----------
        frame : cpl.ui.Frame
            The frame to insert.
        )pydoc")
      .def("__repr__",
           [](cpl::ui::FrameSet& a) -> py::str {
             return frameset_formatter(a, frame_to_repr, 15, 3,
                                       "cpl.ui.FrameSet([", "])");
           })
      .def("__str__",
           [](cpl::ui::FrameSet& a) -> py::str {
             return frameset_formatter(a, frame_to_str, 15, 3);
           })
      .def(
          "dump",
          [](cpl::ui::FrameSet& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump the FrameSet contents to a file, stdout or a string.
          
        This function is mainly intended for debug purposes.

        Parameters
        ----------
        filename : str, optional
            file path to dump frameset contents to
        mode : str, optional
            File mode to save the file, default 'w' overwrites contents.
        show : bool, optional
            Send frameset contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the frameset contents.
        )pydoc")
      .def("__len__", &cpl::ui::FrameSet::size)
      .def("__getitem__",
           [](cpl::ui::FrameSet& a,
              signed int index) -> std::shared_ptr<cpl::ui::Frame> {
             if (index < 0) {
               return a.get_at(index + a.size());
             } else {
               return a.get_at(index);
             }
           })
      .def("__eq__",
           [](cpl::ui::FrameSet& self, py::object eq_arg) -> bool {
             /* See Frame.__eq__() above for an explanation of the below casting
              */
             try {
               cpl::ui::FrameSet* casted_frameset =
                   eq_arg.cast<cpl::ui::FrameSet*>();
               return self == *casted_frameset;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           })
      .def(
          "sign_products",
          [](std::shared_ptr<cpl::ui::FrameSet> self, bool md5, bool checksum)
              -> void { cpl::dfs::sign_products(self, md5, checksum); },
          py::arg("compute_md5") = true, py::arg("compute_checksum") = true,
          R"mydelim(
    Update DFS and DICB required header information of frames in the frameset

    Parameters
    ----------
    compute_md5 : bool, optional
      Boolean Flag to compute the ``DATAMD5`` hash and add to the product header
    compute_checksum : bool, optional
      Flag to compute the standard FITS checksums

    Return
    ------
    None

    Notes
    -----
    The function takes all frames marked as products from the input frameset.
    )mydelim")
      .def(
          "update_product_header",
          [](std::shared_ptr<cpl::ui::FrameSet> self) -> void {
            cpl::dfs::update_product_header(self);
          },
          R"mydelim(
    Perform any DFS-compliancy required actions (``DATAMD5``/``PIPEFILE`` update) on the
    frames in the framest

    Returns
    -------
    None

    Raises
    ------
    cpl.core.DataNotFoundError
      If the input framelist contains a frame of type
      product with a missing filename.
    cpl.core.BadFileFormatError
      If the input framelist contains a frame of type
      product without a FITS card with key ``DATAMD5`` could not be updated.

    Notes
    -----
    Each product frame must correspond to a FITS file created with a CPL
    FITS saving function.
    )mydelim")
      .def(py::pickle(
          [](cpl::ui::FrameSet& self) -> py::tuple {
            std::vector<std::shared_ptr<cpl::ui::Frame>> vec;
            for (int i = 0; i < self.size(); i++) {
              vec.push_back(self.get_at(i));
            }
            return py::make_tuple(vec);
          },
          [](py::tuple t) -> std::shared_ptr<cpl::ui::FrameSet> {
            std::shared_ptr<cpl::ui::FrameSet> fs =
                std::make_shared<cpl::ui::FrameSet>();
            std::vector<std::shared_ptr<cpl::ui::Frame>> vec =
                t[0].cast<std::vector<std::shared_ptr<cpl::ui::Frame>>>();
            for (std::shared_ptr<cpl::ui::Frame> f : vec) {
              fs->append(f);
            }
            return fs;
          }));
}
