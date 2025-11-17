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

#include "cpldrs/geom_img_bindings.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "cplcore/bivector.hpp"
#include "cplcore/error.hpp"
#include "cplcore/imagelist.hpp"
#include "cplcore/vector.hpp"
// FIXME: For now include cpl_geom_img.h only through including
//        geom_img.hpp! See there for an explanation of the issue.
#include "cpldrs/geom_img.hpp"

namespace py = pybind11;

using size = cpl::core::size;

void
bind_geom_img(py::module& m)
{
  py::module geom_img = m.def_submodule(
      "geometric_transforms",
      "Functions to compute the shift-and-add operation on an image list.");

  py::object NamedTuple = py::module::import("collections").attr("namedtuple");
  py::object geom_offset_combine_tuple = NamedTuple(
      "GeomOffsetCombineResult", py::cast(std::vector<std::string>(
                                     {"combined", "contribution", "pisigma"})));
  py::object geom_offset_saa_tuple =
      NamedTuple("GeomOffsetSaaResult",
                 py::cast(std::vector<std::string>(
                     {"combined", "contribution", "ppos_x", "ppos_y"})));

  // Bindings for the combine enums
  py::enum_<cpl_geom_combine>(geom_img, "Combine",
                              "The CPL Geometry combination modes for the "
                              "various cpl.drs.geometric_transforms functions")
      .value("INTERSECT", CPL_GEOM_INTERSECT,
             "Combine using the intersection of the images.")
      .value("UNION", CPL_GEOM_UNION, "Combine using the union of the images.")
      .value("FIRST", CPL_GEOM_FIRST,
             "Combine using the first image to aggregate the other ones.")
      .export_values();

  geom_img
      .def("offset_fine", &cpl::drs::geom::img_offset_fine, py::arg("ilist"),
           py::arg("estimates"), py::arg("anchors"), py::arg("search_hx"),
           py::arg("search_hy"), py::arg("measure_hx"), py::arg("measure_hy"),
           R"pydoc(
        Get the offsets by correlating the images

        The images in the input list must only differ from a shift. In order
        from the correlation to work, they must have the same level (check the
        average values of your input images if the correlation does not work).

        The supported image types are cpl.core.Type.DOUBLE and cpl.core.Type.FLOAT.
        The bad pixel maps are ignored by this function.

        Parameters
        ----------
        ilist : cpl.core.ImageList
            Input image list
        estimates : cpl.core.Bivector
            First-guess estimation of the offsets
        anchors : cpl.core.Bivector
            List of cross-correlation points
        search_hx : int
            Half-width of search area
        search_hy : int
            Half-height of search area
        measure_hx : int
            Half-width of the measurement area
        measure_hy : int
            Half-height of the measurement area

        Return
        ------
        tuple(cpl.core.Bivector, cpl.core.Vector)
            Tuple of the List of offsets and the list of cross-correlation quality
            factors, in the format (`offsets`, `quality_factors`).

        Notes
        -----
        The matching is performed using a 2d cross-correlation, using a minimal
        squared differences criterion. One measurement is performed per input anchor
        point, and the median offset is returned together with a measure of
        similarity for each plane.

        The images in the input list must only differ from a shift. In order
        from the correlation to work, they must have the same level (check the
        average values of your input images if the correlation does not work).

        The ith offset (:code:`offsets.x`, :code:`offsets.y`) in the returned
        `offsets` is the one that have to be used to shift the ith image to align
        it on the reference image (the first one).

        Raises
        ------
        cpl.core.IllegalInputError
            if ilist is not valid
        )pydoc")
      .def(
          "offset_combine",
          [geom_offset_combine_tuple](
              const cpl::core::ImageList& ilist,
              const cpl::core::Bivector& offs, size min_rej, size max_rej,
              cpl_geom_combine union_flag, bool refine,
              std::optional<size> s_hx, std::optional<size> s_hy,
              std::optional<size> m_hx, std::optional<size> m_hy,
              std::optional<cpl::core::Bivector> anchors,
              std::optional<cpl::core::Vector> sigmas) -> py::object {
            // Pass args to function and store result in python tuple
            size s_hx_in, s_hy_in, m_hx_in, m_hy_in;
            if (refine && (!s_hx.has_value() || !s_hy.has_value() ||
                           !m_hx.has_value() || !m_hy.has_value())) {
              throw cpl::core::IllegalInputError(
                  PYCPL_ERROR_LOCATION,
                  "search_hx, search_hy, measure_hx and measure_hy must be "
                  "given for refine=True");
            } else if (!refine) {
              // Pass in 0 to everything. This won't impact functionality as
              // these values won't be used in the underlying CPL if refine is
              // false anyway, it just meets the C syntax rules
              s_hx_in = 0;
              s_hy_in = 0;
              m_hx_in = 0;
              m_hy_in = 0;
            } else {
              s_hx_in = s_hx.value();
              s_hy_in = s_hy.value();
              m_hx_in = m_hx.value();
              m_hy_in = m_hy.value();
            }

            py::tuple res = py::cast(cpl::drs::geom::img_offset_combine(
                ilist, offs, s_hx_in, s_hy_in, m_hx_in, m_hy_in, min_rej,
                max_rej, (cpl_geom_combine)union_flag, refine, anchors,
                sigmas));
            // Return in NamedTuple form
            return geom_offset_combine_tuple(*res);
          },
          py::arg("ilist"), py::arg("offs"), py::arg("min_rej"),
          py::arg("max_rej"), py::arg("union_flag"), py::arg("refine") = false,
          py::arg("search_hx").none(true) = py::none(),
          py::arg("search_hy").none(true) = py::none(),
          py::arg("measure_hx").none(true) = py::none(),
          py::arg("measure_hy").none(true) = py::none(),
          py::arg("anchors").none(true) = py::none(),
          py::arg("sigmas").none(true) = py::none(),
          R"pydoc(
        Images list recombination

        If offset refinement is enabled this function will detect sources in the
        first image (unless a list of positions has been provided by the user using
        the `anchors` parameter) then use cross correlation to refine the provided
        estimated image offsets from the `offs` parameters. If offset refinement is
        disabled the image offsets in `offs` are used as they are.

        Following the optional offset refinement each image is shifted by the
        corresponding offset before being added together to produce a combined image.

        The supported types are cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT.

        The number of provided offsets shall be equal to the number of input images.
        The ith offset (:code:`offs.x`, :code:`offs_y`) is the offset that has to be
        used to shift the ith image to align it on the first one.

        If offset refinement is enabled (`refine`=True), `anchors` or `sigmas` must
        be given, with `anchors` taking precidence.

        Parameters
        ----------
        ilist : cpl.core.ImageList
            Input image list
        offs : cpl.core.Bivector
            List of offsets in x and y. Applied directly if `refine` is False,
            otherwise it will be refined using cross-correlation.
        union_flag : cpl.drs.geometric_transforms.Combine
            Combination mode: cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.INTERSECT or
            cpl.drs.geometric_transforms.Combine.FIRST.
        search_hx : int
            Half-width of search area. This parameter must be set when `refine` is
            `True`, if `refine` is `False` it has no effect.
        search_hy : int
            Half-height of search area. This parameter must be set when `refine`
            is `True`, otherwise it has no effect.
        measure_hx : int
            Half-width of the measurement area. This parameter must be set when
            `refine` is `True`, otherwise it has no effect.
        measure_hy : int
            Half-height of the measurement area. This parameter must be set when
            `refine` is `True`, otherwise it has no effect.
        refine : bool, optional
            Set to True to enable offset refinement offsets
        anchors : cpl.core.Bivector, optional
            List of cross corelation points in the first image. Unused if `refine`
            is set to False
        sigmas : cpl.core.Vector, optional
            Positive, decreasing sigmas to apply for cross-correlation point
            detection. Unused if `refine` is set to False, or if `refine` is
            True but `anchors` is given.

        Return
        ------
        NamedTuple(cpl.core.Image, cpl.core.Image, int or None)
            NamedTuple in the format (combined, contribution, pisigma) where:

            - combined: the combined image
            - contribution: the contribution map
            - pisigma: Index of the sigma that was used. None if `sigmas` is not given

        Raises
        ------
        cpl.core.NullInputError
            if `sigmas` is not given when either refine set to True and anchors is
            also not given
        cpl.core.IllegalInputError
            if ilist is not uniform, or if `search_hx`, `search_hy`, `measure_hx`
            and `measure_hy` have not been set when `refine` is set to `True`.
        cpl.core.IncompatibleInputError
            if ilist and offs have different sizes
        cpl.core.DataNotFoundError
            if the shift and add of the images fails

        See Also
        --------
        cpl.drs.geometric_transformations.offset_fine : used to refine the offsets if refine is `True`
        cpl.drs.geometric_transformations.offset_saa : used for image recombination using the default kernel
        )pydoc")
      .def(
          "offset_saa",
          [geom_offset_saa_tuple](const cpl::core::ImageList& ilist,
                                  const cpl::core::Bivector& offs,
                                  cpl_kernel kernel, size rejmin, size rejmax,
                                  cpl_geom_combine union_flag) -> py::object {
            // Pass args to function and store result in python tuple
            py::tuple res = py::cast(cpl::drs::geom::img_offset_saa(
                ilist, offs, kernel, rejmin, rejmax,
                (cpl_geom_combine)union_flag));
            // Return in NamedTuple form
            return geom_offset_saa_tuple(*res);
          },
          py::arg("ilist"), py::arg("offs"), py::arg("kernel"),
          py::arg("rejmin"), py::arg("rejmax"), py::arg("union_flag"),
          R"pydoc(
        Shift and add an images list to a single image

        The supported types are cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT.

        The number of provided offsets shall be equal to the number of input images.
        The ith offset (offs_x, offs_y) is the offset that has to be used to shift
        the ith image to align it on the first one.

        The following kernel types are supported when being passed to `kernel`:

            - cpl.core.Kernel.DEFAULT: default kernel, currently cpl.core.Kernel.TANH
            - cpl.core.Kernel.TANH: Hyperbolic tangent
            - cpl.core.Kernel.SINC: Sinus cardinal
            - cpl.core.Kernel.SINC2: Square sinus cardinal
            - cpl.core.Kernel.LANCZOS: Lanczos2 kernel
            - cpl.core.Kernel.HAMMING: Hamming kernel
            - cpl.core.Kernel.HANN: Hann kernel
            - cpl.core.Kernel.NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)

        If the number of input images is lower or equal to 3, the rejection
        parameters are ignored.
        If the number of input images is lower or equal to 2*(rejmin+rejmax), the
        rejection parameters are ignored.

        Pixels with a zero in the contribution map are flagged as bad in the
        combined image.

        The return values ppos_x and ppos_y follow the PyCPL standard, where the
        lower-leftmost pixel of the output image is at (0, 0). Note that this
        differs from the corresponding CPL function, where the lower-leftmost
        pixel of the output image is at (1, 1).

        Parameters
        ----------
        ilist : cpl.core.ImageList
            Input image list
        offs : cpl.core.Bivector
            List of offsets in x and y
        kernel : cpl.core.Kernel
            Interpolation kernel to use for resampling. See extended summary for
            supported kernel types
        rejmin : int
            Number of minimum value pixels to reject in stacking
        rejmax : int
            Number of maximum value pixels to reject in stacking
        union_flag : cpl.drs.geometric_transforms.Combine
            Combination mode: cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.INTERSECT or cpl.drs.geometric_transforms.Combine.FIRST

        Return
        ------
        NamedTuple(cpl.core.Image, cpl.core.Image, float, float)
            NamedTuple in the format (combined, contribution, ppos_x, ppos_y) where:

            - combined: the combined image
            - contribution: the contribution map
            - ppos_x: X-position of the first image in the combined image
            - ppos_y: Y-position of the first image in the combined image

            `ppos_x` and `ppos_y` represent the pixel coordinate in
            the created output image-pair `combined` and `contribution` where the
            lowermost-leftmost pixel of the first input image is located. So with
            cpl.drs.geometric_transforms.Combine.FIRST this will always be (0, 0).

        Raises
        ------
        cpl.core.IllegalInputError
            if ilist is not valid or rejmin or rejmax is negative
        cpl.core.IncompatibleInputError
            if ilist and offs have different sizes
        cpl.core.IllegalOutputError
            if cpl.drs.geometric_transforms.INTERSECT is used with non-overlapping images.
        cpl.core.InvalidTypeError
            if the passed image list type is not supported
        cpl.core.UnsupportedModeError
            if union_flag is not one of the supported modes.
        )pydoc");
}