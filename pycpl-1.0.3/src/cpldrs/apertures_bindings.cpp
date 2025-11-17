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

#include "cpldrs/apertures_bindings.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cpl_apertures.h>
#include <pybind11/stl.h>

#include "cplcore/coords.hpp"
#include "cplcore/image.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/vector.hpp"
#include "cplcore/window_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive
#include "cpldrs/apertures.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

struct ApertureAccessor
{
  /**
   * Quick access to the Apertures object
   */
  std::shared_ptr<cpl::drs::Apertures> Apertures;
  /**
   * The index of the aperture in Apertures
   */
  size idx;
};

void
bind_apertures(py::module& m)
{
  py::class_<ApertureAccessor> aperture_class(m, "Aperture");
  aperture_class.doc() = R"docstring(
        Returned from a Apertures' __getitem__ method or iterator. Used to access
        each Aperture record individually.

        Not instantiatable on its own.
    )docstring";

  py::class_<cpl::drs::Apertures, std::shared_ptr<cpl::drs::Apertures>>
      apertures(m, "Apertures");
  apertures.doc() = R"pydoc(
    Compute statistics on selected apertures.

    The aperture object contains a list of zones in an image. It is typically
    used to contain the results of an objects detection, or if one wants to work
    on a very specific zone in an image.

    Can be built either with the constructor with a reference and labellised image,
    or via the various static `extract_*` functions.

    Each individual Aperture statistic can be accessed either via the `get_*`
    methods (using 1 indexing) or by indexing the Apertures themselves (e.g.
    apt[0], 0 indexing), which will return an `Aperture` object, with the properties
    corresponding to the individual Aperture statistics.

    Parameters
    ----------
    reference : cpl.core.Image
        Reference image
    labelized : cpl.core.Image
        Labelized image (of type cpl.core.Type.INT). Must contain at least one pixel
        for each value from 1 to the maximum value in the image.

    Raises
    ------
    cpl.core.TypeMismatchError
        if labelized is not of cpl.core.Type.INT
    cpl.core.IllegalInputError
        if labelized has a negative value or zero maximum
    cpl.core.IncompatibleInputError
        if lab and inImage have different sizes.

    Notes
    -----
    For the centroiding computation of an aperture, if some pixels have
    values lower or equal to 0, all the values of the aperture are locally
    shifted such as the minimum value of the aperture has a value of
    epsilon. The centroid is then computed on these positive values. In
    principle, centroid should always be computed on positive values, this
    is done to avoid raising an error in case the caller of the function
    wants to use it on negative values images without caring about the
    centroid results. In such cases, the centroid result would be
    meaningful, but slightly depend on the hardcoded value chosen for
    epsilon (1e-10).

    See Also
    --------
    cpl.core.Image.labelise_create : Can be used for creating `labelized`.
)pydoc";

  py::object NamedTuple = py::module::import("collections").attr("namedtuple");
  py::object extract_tuple =
      NamedTuple("ExtractResult",
                 py::cast(std::vector<std::string>({"Apertures", "pisigma"})));

  apertures.def("__str__", &cpl::drs::Apertures::dump)
      .def(
          "dump",
          [](cpl::drs::Apertures& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump the Apertures contents to a file, stdout or a string.
          
        This function is mainly intended for debug purposes.

        Parameters
        ----------
        filename : str, optional
            file path to dump apertures contents to
        mode : str, optional
            File mode to save the file, default 'w' overwrites contents.
        show : bool, optional
            Send apertures contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the apertures contents.
        )pydoc")
      .def("__repr__",
           [](cpl::drs::Apertures& a) -> py::str {
             py::str representation = "<cpl.drs.Apertures, {} Apertures>";
             return representation.format(a.get_size());
           })
// FIXME: The dump() method above replaces the write functionality.
//        Review if this is obsolete and can be removed
#if 0
      .def(
          "write",
          [](cpl::drs::Apertures& self, std::string location,
             std::string mode) {
            if (mode != "w" && mode != "w+" && mode != "a" && mode != "a+") {
              throw cpl::core::IllegalInputError(
                  PYCPL_ERROR_LOCATION,
                  "Invalid file mode. Valid modes are w, w+, a, a+");
            }
            FILE* fp =
                fopen(location.c_str(),
                      mode.c_str());  // Use args to create a FILE pointer
            if (fp == NULL) {
              throw cpl::core::FileIOError(PYCPL_ERROR_LOCATION,
                                           "Failed to open file " + location);
            }
            self.dump(fp);  // Dump to file pointer
            fclose(fp);     // Close the stream
          },
          py::arg("file"), py::arg("mode"),
          R"pydoc(
          Write the aperture information to a text file.

          Parameters
          ----------
          file : str
              Path of the file to write to
          mode : str
              file mode to open. Supports "w" for write, and "a" for append.
          )pydoc")
#endif
      .def("__getitem__",
           [](std::shared_ptr<cpl::drs::Apertures> self,
              size index) -> ApertureAccessor {
             if (index < 0) {
               index = self->get_size() - index;
             }
             if (index >= self->get_size() || index < 0) {
               throw cpl::core::AccessOutOfRangeError(
                   PYCPL_ERROR_LOCATION,
                   "Index must be positive or less than the number of "
                   "apertures");
             }
             // While python collections are 0 indexed, apertures start from 1.
             // Just transform it for accessibility.
             return ApertureAccessor{self, index + 1};
           })
      .def("__next__",
           [](std::shared_ptr<cpl::drs::Apertures> self) -> ApertureAccessor {
             if (self->iter_idx >= self->get_size()) {
               self->iter_idx = 0;
               throw py::stop_iteration();
             }
             int index = self->iter_idx++;
             return ApertureAccessor{self, index + 1};
           })
      .def("__iter__", [](py::object self) -> py::object { return self; })
      .def(py::init<const cpl::core::ImageBase&, const cpl::core::ImageBase&>(),
           py::arg("reference"), py::arg("labelized"))
      .def("__len__", &cpl::drs::Apertures::get_size)
      .def("get_pos_x", &cpl::drs::Apertures::get_pos_x, py::arg("idx"),
           R"pydoc(
    Get the average X-position of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    float
        The average X-position of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_pos_y", &cpl::drs::Apertures::get_pos_y, py::arg("idx"),
           R"pydoc(
    Get the average Y-position of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    float
        The average Y-position of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_centroid_x", &cpl::drs::Apertures::get_centroid_x,
           py::arg("idx"), R"pydoc(
    Get the X-centroid of an aperture

    For a concave aperture the centroid may not belong to the aperture.

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    float
        The X-centroid of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_centroid_y", &cpl::drs::Apertures::get_centroid_y,
           py::arg("idx"), R"pydoc(
    Get the Y-centroid of an aperture

    For a concave aperture the centroid may not belong to the aperture.

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    float
        The Y-centroid of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_maxpos_x", &cpl::drs::Apertures::get_maxpos_x, py::arg("idx"),
           R"pydoc(
    Get the X-position of the aperture maximum value

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The X-position of the aperture maximum value

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_maxpos_y", &cpl::drs::Apertures::get_maxpos_y, py::arg("idx"),
           R"pydoc(
    Get the Y-position of the aperture maximum value

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The Y-position of the aperture maximum value

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_minpos_x", &cpl::drs::Apertures::get_minpos_x, py::arg("idx"),
           R"pydoc(
    Get the X-position of the aperture minimum value

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The X-position of the aperture minimum value

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_minpos_y", &cpl::drs::Apertures::get_minpos_y, py::arg("idx"),
           R"pydoc(
    Get the Y-position of the aperture minimum value

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The Y-position of the aperture minimum value

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_npix", &cpl::drs::Apertures::get_npix, py::arg("idx"), R"pydoc(
    Get the number of pixels of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The number of pixels of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_left", &cpl::drs::Apertures::get_left, py::arg("idx"), R"pydoc(
    Get the leftmost x position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the leftmost x position of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_left_y", &cpl::drs::Apertures::get_left_y, py::arg("idx"),
           R"pydoc(
    Get the y position of the leftmost x position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the y position of the leftmost x position

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_right", &cpl::drs::Apertures::get_right, py::arg("idx"),
           R"pydoc(
    Get the rightmost x position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the rightmost x position in an aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_right_y", &cpl::drs::Apertures::get_right_y, py::arg("idx"),
           R"pydoc(
    Get the y position of the rightmost x position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the y position of the rightmost x position

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_bottom_x", &cpl::drs::Apertures::get_bottom_x, py::arg("idx"),
           R"pydoc(
    Get the x position of the bottommost y position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the bottommost x position of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_bottom", &cpl::drs::Apertures::get_bottom, py::arg("idx"),
           R"pydoc(
    Get the bottommost y position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the bottommost y position in the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_top_x", &cpl::drs::Apertures::get_top_x, py::arg("idx"),
           R"pydoc(
    Get the x position of the topmost y position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the x position of the topmost y position or negative on error

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_top", &cpl::drs::Apertures::get_top, py::arg("idx"),
           R"pydoc(
    Get the topmost y position in an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        the topmost y position in the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_max", &cpl::drs::Apertures::get_max, py::arg("idx"),
           R"pydoc(
    Get the maximum value of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The maximum value of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_min", &cpl::drs::Apertures::get_min, py::arg("idx"),
           R"pydoc(
    Get the minimum value of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The minimum value of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_mean", &cpl::drs::Apertures::get_mean, py::arg("idx"),
           R"pydoc(
    Get the mean value of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The mean value of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_median", &cpl::drs::Apertures::get_median, py::arg("idx"),
           R"pydoc(
    Get the median value of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The median value of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("get_stdev", &cpl::drs::Apertures::get_stdev, py::arg("idx"),
           R"pydoc(
    Get the standard deviation value of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The standard deviation value of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
    cpl.core.DataNotFOundError
        if the aperture comprises of less than two pixels
)pydoc")
      .def("get_flux", &cpl::drs::Apertures::get_flux, py::arg("idx"),
           R"pydoc(
    Get the flux of an aperture

    Parameters
    ----------
    idx : int
        The aperture index (1 for the first one)

    Returns
    -------
    int
        The flux of the aperture

    Raises
    ------
    cpl.core.IllegalInputError
        if idx is non-positive
    cpl.core.AccessOutOfRangeError
        if idx is greater than the number of apertures
)pydoc")
      .def("sort_by_npix", &cpl::drs::Apertures::sort_by_npix,
           "Sort apertures by decreasing size (in pixels) and apply changes")
      // No parameters or return values or extra notes so no full docstring
      // needed
      .def("sort_by_max", &cpl::drs::Apertures::sort_by_max,
           "Sort apertures by decreasing peak value and apply changes")
      .def("sort_by_flux", &cpl::drs::Apertures::sort_by_flux,
           "Sort apertures by decreasing aperture flux and apply changes")
      .def_static(
          "extract",
          [extract_tuple](const cpl::core::ImageBase& inImage,
                          const cpl::core::Vector& sigmas) {
            py::tuple res =
                py::cast(cpl::drs::Apertures::extract(inImage, sigmas));
            return extract_tuple(*res);
          },
          py::arg("source_image"), py::arg("sigmas"),
          R"pydoc(
    Simple detection of apertures in an image

    Aperture detection on the image is performed using each value in `sigmas`
    until at least one is found.

    Parameters
    ----------
    source_image : cpl.core.Image
        The image to process
    sigmas : cpl.core.Vector
        Detection levels. Positive, decreasing sigmas to apply

    Returns
    -------
    cpl.drs.Apertures, int
        The detected apertures (cpl.drs.Apertures) and the index of the sigma that
        was used (int)

    Raises
    ------
    cpl.core.DataNotFoundError
        if the apertures could not be detected

    See Also
    --------
    cpl.drs.Apertures.extract_sigma :
        Used on the image for aperture detection. Also provides detailed explaination
        of individual sigmas.
)pydoc")
      .def_static(
          "extract_window",
          [extract_tuple](const cpl::core::ImageBase& self,
                          const cpl::core::Vector& sigmas,
                          cpl::core::Window area) {
            py::tuple res = py::cast(
                cpl::drs::Apertures::extract_window(self, sigmas, area));
            return extract_tuple(*res);
          },
          py::arg("source_image"), py::arg("sigmas"), py::arg("area"),
          R"pydoc(
    Simple detection of apertures in an image window

    Aperture detection on the window is performed using each value in `sigmas` until
    at least one is found.

    Parameters
    ----------
    source_image : cpl.core.Image
        The image to process
    sigmas : cpl.core.Vector
        Detection level. Positive, decreasing sigmas to apply
    area : tuple(int, int, int, int)
        Rectangle of the window in the format (llx, lly, urx, ury) where:
        
            - llx : Lower left x position
            - lly : Lower left y position
            - urx : Upper right x position
            - ury : Upper right y position

        Position indices are zero based.

    Returns
    -------
    cpl.drs.Apertures, int
        The detected apertures (cpl.drs.Apertures) and the index of the sigma that
        was used (int)

    Raises
    ------
    cpl.core.DataNotFoundError
        if the apertures could not be detected

    See Also
    --------
    cpl.drs.Apertures.extract_sigma :
        Used on the window for aperture detection. Also provides detailed
        explaination of individual sigmas.
)pydoc")
      .def_static(
          "extract_mask",
          [](const cpl::core::ImageBase& source_image,
             py::object selection) -> std::shared_ptr<cpl::drs::Apertures> {
            if (py::hasattr(selection, "_mask")) {
              return cpl::drs::Apertures::extract_mask(
                  source_image,
                  selection.attr("_mask").cast<cpl::core::Mask&>());
            } else {
              throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                 "selection must be a mask");
            }
          },
          py::arg("source_image"), py::arg("selection"),
          R"pydoc(
    Simple detection of apertures in an image from a user supplied selection mask

    The values selected for inclusion in the apertures must have the non-zero value
    in the selection mask, and must not be flagged as bad in the bad pixel map of
    the image.

    Parameters
    ----------
    source_image : cpl.core.Image
        The image to process. Can be of type cpl.core.Type.DOUBLE,
        cpl.core.Type.FLOAT, or cpl.core.Type.INT
    sigmas : cpl.core.Vector
        Detection levels. Positive, decreasing sigmas to apply

    Returns
    -------
    cpl.drs.Apertures
        The detected apertures

    Raises
    ------
    cpl.core.IncompatibleInputError
        if`source_image`and selection have different sizes
    cpl.core.TypeMistmatchError
        if`source_image`is of a complex type
    cpl.core.DataNotFoundError
        if the selection mask is empty
)pydoc")
      .def_static("extract_sigma", &cpl::drs::Apertures::extract_sigma,
                  py::arg("source_image"), py::arg("selection"),
                  R"pydoc(
    Simple detection of apertures in an image using a provided sigma

    Sigma is used to calculate the threshold for the aperture detection. This
    threshold is calculated using the median plus the average distance to the median
    times sigma.

    Parameters
    ----------
    source_image : cpl.core.Image
        The image to process
    sigma : float
        Detection level. Used as a variable to calculate the threshold for detection.

    Returns
    -------
    cpl.drs.Apertures
        The detected apertures

    Raises
    ------
    cpl.core.IllegalInputError
        if sigma is non-positive
    cpl.core.TypeMismatchError
        if`source_image`is of a complex type
    cpl.core.DataNotFoundError
        if the apertures could not be detected

    Notes
    -----
    In order to avoid (the potentially many) detections of small objects the mask
    of detected pixels is subjected to a 3x3 morphological opening filter.
)pydoc");

  aperture_class
      .def_property_readonly(
          "pos_x",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_pos_x(self.idx);
          },
          "average X-position of an aperture")
      .def_property_readonly(
          "pos_y",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_pos_y(self.idx);
          },
          "average Y-position of an aperture")
      .def_property_readonly(
          "centroid_x",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_centroid_x(self.idx);
          },
          "The X-centroid of an aperture")
      .def_property_readonly(
          "centroid_y",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_centroid_y(self.idx);
          },
          R"pydoc(
            The Y-centroid of an aperture. For a concave aperture the centroid may
            not belong to the aperture.
          )pydoc")
      .def_property_readonly(
          "maxpos_x",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_maxpos_x(self.idx);
          },
          "The X-position of the aperture maximum value")
      .def_property_readonly(
          "maxpos_y",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_maxpos_y(self.idx);
          },
          "The Y-position of the aperture maximum value")
      .def_property_readonly(
          "minpos_x",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_minpos_x(self.idx);
          },
          "The X-position of the aperture minimum value")
      .def_property_readonly(
          "minpos_y",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_minpos_y(self.idx);
          },
          "The Y-position of the aperture minimum value")
      .def_property_readonly(
          "npix",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_npix(self.idx);
          },
          "The number of pixels of an aperture")
      .def_property_readonly(
          "left",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_left(self.idx);
          },
          "The leftmost x position in an aperture")
      .def_property_readonly(
          "left_y",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_left_y(self.idx);
          },
          R"pydoc(
            The y position of the leftmost x position in an aperture. An aperture may
            have multiple leftmost y positions, in which case one of these is returned.
          )pydoc")
      .def_property_readonly(
          "right",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_right(self.idx);
          },
          "The rightmost x position in an aperture")
      .def_property_readonly(
          "right_y",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_right_y(self.idx);
          },
          R"pydoc(
            The y position of the rightmost x position in an aperture. An aperture may
            have multiple rightmost y positions, in which case one of these is returned.
          )pydoc")
      .def_property_readonly(
          "bottom_x",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_bottom_x(self.idx);
          },
          R"pydoc(
            The x position of the bottommost y position in an aperture. An aperture may
            have multiple bottommost x positions, in which case one of these is returned.
          )pydoc")
      .def_property_readonly(
          "bottom",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_bottom(self.idx);
          },
          "The bottommost y position in an aperture")
      .def_property_readonly(
          "top_x",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_top_x(self.idx);
          },
          R"pydoc(
            The x position of the topmost y position in an aperture. An aperture may
            have multiple topmost x positions, in which case one of these is returned.
          )pydoc")
      .def_property_readonly(
          "top",
          [](const ApertureAccessor& self) -> size {
            return self.Apertures->get_top(self.idx);
          },
          "The topmost y position in an aperture")
      .def_property_readonly(
          "max",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_max(self.idx);
          },
          "The maximum value of an aperture")
      .def_property_readonly(
          "min",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_min(self.idx);
          },
          "The minimum value of an aperture")
      .def_property_readonly(
          "mean",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_mean(self.idx);
          },
          "The mean value of an aperture")
      .def_property_readonly(
          "median",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_median(self.idx);
          },
          "The median value of an aperture")
      .def_property_readonly(
          "stdev",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_stdev(self.idx);
          },
          R"pydoc(
            The standard deviation value of an aperture

            Raises
            ------
            cpl.core.DataNotFoundError
                if the aperture comprises of less than two pixels
        )pydoc")
      .def_property_readonly(
          "flux",
          [](const ApertureAccessor& self) -> double {
            return self.Apertures->get_flux(self.idx);
          },
          "The flux of an aperture");
}
