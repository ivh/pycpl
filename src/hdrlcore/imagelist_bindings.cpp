// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

#include "hdrlcore/imagelist_bindings.hpp"

#include <filesystem>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "dump_handler.hpp"
#include "hdrlcore/image.hpp"
#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/pycpl_image.hpp"      // IWYU pragma: keep
#include "hdrlcore/pycpl_imagelist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"       // IWYU pragma: keep
#include "hdrlcore/pycpl_window.hpp"     // IWYU pragma: keep
#include "hdrlcore/value.hpp"            // IWYU pragma: keep
#include "hdrlfunc/collapse.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep


namespace py = pybind11;

void
bind_hdrl_imagelist(py::module& m)
{
  py::class_<hdrl::core::ImageList, std::shared_ptr<hdrl::core::ImageList>>
      imagelist_class(m, "ImageList", py::buffer_protocol());

  imagelist_class.doc() = R"docstring(
      A hdrl.core.ImageList is an HDRL imagelist containing a list of equally dimensioned HDRL Images. The API is similar
      to cpl.core.Imagelist and simple arithmetic and collapse operations propagate errors linearly. It follows 0-indexing
      and must have the same pixel type.

      Parameters
      ----------
      datalist : cpl.core.ImageList
          Data cpl.core.ImageList to store in `self` on init.
      errorlist : cpl.core.ImageList
          cpl.core.ImageList of corresponding errors to store in `self` on init.

      Notes
      -----
      A new empty hdrl.core.ImageList can be created using hdrl.core.ImageList()

      )docstring";

  imagelist_class.def(py::init<>())
      .def(py::init<hdrl::core::pycpl_imagelist, hdrl::core::pycpl_imagelist>())
      .def("__len__", &hdrl::core::ImageList::get_size,
           "int: size of the imagelist")
      .def_property_readonly("size_x", &hdrl::core::ImageList::get_size_x,
                             "int: number of columns of images in an imagelist")
      .def_property_readonly("size_y", &hdrl::core::ImageList::get_size_y,
                             "int: number of rows of images in an imagelist")
      .def(
          "collapse",
          [](std::shared_ptr<hdrl::core::ImageList>& self,
             hdrl::func::Collapse collapse) { return collapse.compute(self); },
          py::arg("collapse"), R"pydoc(
        Collapse an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts the
                number of pixels contributed to each pixel image.

        See Also
        ----------
        hdrl.func.Collapse : Interface for Collapse operations.
        hdrl.func.Collapse.compute : Perform Collapse operations on an HDRL Image or ImageList.
        )pydoc")
      .def(
          "collapse_mean",
          [](std::shared_ptr<hdrl::core::ImageList>& self) {
            //[](hdrl::core::ImageList& self) {
            // return
            // hdrl::func::Collapse::collapse_mean(std::make_shared<hdrl::core::ImageList>(self));
            return hdrl::func::Collapse::collapse_mean(self);
          },
          R"pydoc(
        Mean collapse of an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Returns
        -------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts the
                number of pixels contributed to each pixel image.

        See Also
        ----------
        hdrl.func.Collapse.Mean : Interface for performing Collapse operation on HDRL ImageList or Image with Mean parameters.
        hdrl.func.Collapse.compute : Perform Collapse operations on an HDRL Image or ImageList.
        )pydoc")
      .def(
          "collapse_weighted_mean",
          [](std::shared_ptr<hdrl::core::ImageList>& self) {
            return hdrl::func::Collapse::collapse_weighted_mean(self);
          },
          R"pydoc(
        The weighted mean collapse of an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        --------
        hdrl.func.Collapse.WeightedMean : Perform Weighted Mean collapse on an HDRL Image or ImageList.
        hdrl.func.Collapse.compute : Perform Collapse operations on an HDRL Image or ImageList.
        )pydoc")
      .def(
          "collapse_median",
          [](std::shared_ptr<hdrl::core::ImageList>& self) {
            return hdrl::func::Collapse::collapse_median(self);
          },
          R"pydoc(
        The median collapse of an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Returns
        --------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        ---------
        hdrl.func.Collapse.Median : Interface for Median Collapse on an HDRL Image or ImageList.
        hdrl.func.Collapse.compute : Perform Collapse operations on an HDRL Image or ImageList.
        )pydoc")
      .def(
          "collapse_sigclip",
          [](std::shared_ptr<hdrl::core::ImageList>& self, double kappa_low,
             double kappa_high, int niter) {
            return hdrl::func::Collapse::collapse_sigclip(self, kappa_low,
                                                          kappa_high, niter);
          },
          py::arg("kappa_low"), py::arg("kappa_high"), py::arg("niter"),
          R"pydoc(
        The sigma clipped collapse of an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Parameters
        ----------
        kappa_low : float
            low sigma bound
        kappa_high : float
            high sigma bound
        niter : int
            number of clipping iterators

        Returns
        --------
             namedtuple
                The namedtuple has four components- one hdrl.core.Image (out), containing the collapsed image,
                one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image, one cpl.core.Image (reject_low) containing low rejection threshold,
                one cpl.core.Image (reject_high) containing high rejection threshold.

        See Also
        --------
        hdrl.func.Collapse.Sigclip : Interface for Sigma Clipped Collapse on an HDRL Image or ImageList.
        hdrl.func.Collapse.compute : Perform Collapse function on an HDRL ImageList to create one HDRL Image.
        )pydoc")
      .def(
          "collapse_minmax",
          [](std::shared_ptr<hdrl::core::ImageList>& self, double nlow,
             double nhigh) {
            return hdrl::func::Collapse::collapse_minmax(self, nlow, nhigh);
          },
          py::arg("nlow"), py::arg("nhigh"), R"pydoc(
        The min-max clipped collapse of an imagelist to a single image.

        Error propagation is taken to account where the propagation formula is well defined.

        Parameters
        ----------
        nlow : float
            low number of pixels to reject
        nhigh : float
            high number of pixels to reject

        Returns
        --------
             namedtuple
                The namedtuple has four components- one hdrl.core.Image (out), containing the collapsed image,
                one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image, one cpl.core.Image (reject_low) containing low rejection threshold,
                one cpl.core.Image (reject_high) containing high rejection threshold.

        See Also
        ----------
        hdrl.func.Collapse.compute : Perform Collapse operation on an HDRL ImageList to create one HDRL Image.
        hdrl.func.Collapse.MinMax : Interface for Min-max Clipped Collapse on an HDRL Image or ImageList.
        )pydoc")
      .def(
          "collapse_mode",
          [](std::shared_ptr<hdrl::core::ImageList>& self, double histo_min,
             double histo_max, double bin_size, hdrl_mode_type mode_method,
             cpl_size error_niter) {
            return hdrl::func::Collapse::collapse_mode(
                self, histo_min, histo_max, bin_size, mode_method, error_niter);
          },
          py::arg("histo_min"), py::arg("histo_max"), py::arg("bin_size"),
          py::arg("mode_method"), py::arg("error_niter"),
          R"pydoc(
        The mode collapse of an imagelist to a single image.

        The error is calculated from the data, depending on the `error_niter`.

        Parameters
        ----------
        histo_min : float
            minimum value of low pixels to use
        histo_max : float
            maximum value of high pixels to be use
        bin_size : float
            size of the histogram bin
        mode_method : hdrl.func.Collapse.Mode
            mode_method to use for the mode computation
        error_niter : int
            size of the histogram bin

        Returns
        --------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        Notes
        ----------
        If the `error_niter` parameter is set to 0, it is doing an analytically error estimation. If the parameter is larger
        than 0, the error is calculated by a bootstrap Montecarlo simulation from the input data with the value of the parameter
        specifying the number of simulations. In this case the input data are perturbed with the bootstrap technique and
        the mode is calculated error_niter times. From this modes the standard deviation is calculated and returned as error.

        See Also
        ----------
        hdrl.func.Collapse.compute : Perform Collapse operation on an HDRL ImageList to create one HDRL Image.
        hdrl.func.Collapse.Mode : Perform Mode Collapse function on an HDRL ImageList or Image.
        )pydoc")
      .def("__getitem__",
           [](hdrl::core::ImageList& self, int index) {
             if (index > self.get_size() || index < 0) {
               throw py::index_error("ImageList index out of range");
             }
             return self.get_at(index);
           })
      .def(
          "__setitem__",
          [](hdrl::core::ImageList& self, int index,
             std::shared_ptr<hdrl::core::Image> himg) {
            if (index >= self.get_size() || index < 0) {
              throw py::index_error("ImageList index out of range");
            }
            self.set(himg, index);
          },
          py::arg("himg"), py::arg("index"))
      /*     .def(
               "insert",
               [](hdrl::core::ImageList& self, hdrl::core::Image himg, int index
                  ) {
                 if (index > self.get_size() || index< 0) {
                   throw py::index_error("ImageList index out of range");
                 }
                 else if ( index== self.get_size()) {  // Just append to the end
                    self.append(himg);
                 return;
                 }
                 else{
                     //TODO: Implement 'insert' functionality
                 }
               },
               py::arg("himg"), py::arg("index"), R"pydoc(
             Insert an image into the index given of `self`. This will increase
         the imagelist size by 1.

             Parameters
             ----------
             index : int
                 index to insert Image
             item : hdrl.core.Image
                 Image to insert

             See Also
             ----------
             hdrl.core.ImageList.append : Append a HDRL Image to the end of a
         similar-dimension HDRL ImageList. )pydoc")*/

      .def(
          "append",
          [](hdrl::core::ImageList& self,
             std::shared_ptr<hdrl::core::Image> himg) { self.append(himg); },
          py::arg("to_append"),
          R"pydoc(
        Append an HDRL image to the end of `self`. To insert an image into a specific position then set via index
        (e.g. self[i] = new_image). It is not allowed to insert images of different sizes or types into a list.

        Parameters
        ----------
        to_append : hdrl.core.Image
            The image to append

        )pydoc")
      .def(
          "pop",
          [](hdrl::core::ImageList& self, std::optional<cpl_size> index) {
            cpl_size pos = index.value_or(self.get_size() - 1);
            if (pos >= self.get_size() || pos < 0) {
              throw py::index_error("ImageList index out of range");
            }
            return self.pop(pos);
          },
          py::arg("index") = py::none(),
          R"pydoc(
        Remove and return the image at the `index`.

        Parameters
        ----------
        position : int, optional
            Index to pop image from the image list. Defaults to the last image.

        Returns
        ----------
        himg : hdrl.core.Image
                Image at `index`.

        Raises
        ----------
        IndexError
            If the `index` is out of range.
        )pydoc")
      .def("__delitem__",
           [](hdrl::core::ImageList& self, int index) {
             if (index >= self.get_size() || index < 0) {
               throw py::index_error("ImageList index out of range");
             }
             self.pop(index);
           })
      .def("empty", &hdrl::core::ImageList::empty, R"pydoc(
        Empty an imagelist and deallocate all its images. After the call the image list can be populated again.
       )pydoc")
      .def("duplicate", &hdrl::core::ImageList::duplicate,
           R"docstring(
        Copy the HDRL imagelist into a new HDRL imagelist. The pixels and errors are also copied.

        This method is also used when performing a deepcopy on an image.

        Returns
        ----------
        himlist : hdrl.core.ImageList
            New HDRL ImageList that is a copy of the original ImageList.

        )docstring")
      .def("__deepcopy__",
           [](hdrl::core::ImageList& self, py::dict /* unused */) {
             return self.duplicate();
           })
      .def("is_consistent", &hdrl::core::ImageList::is_consistent,
           R"docstring(
        Determine if an ImageList contains images of equal size and type.

        Returns
        ----------
        result : int
            0 if ok, positive if not consistent and negative on error. The function returns 1 if the list is empty.

        )docstring")
      .def("__str__",
           [](hdrl::core::ImageList& self) {
             return self.dump(hdrl::core::pycpl_window::All);
           })
      .def("__repr__", &hdrl::core::ImageList::dump_structure)
      .def(
          "dump",
          [](hdrl::core::ImageList& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<hdrl::core::pycpl_window> window,
             std::optional<bool> show) {
            return dump_handler(filename.value(), mode.value(),
                                self.dump(window), show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("window") = py::none(), py::arg("show") = true,
          R"pydoc(
            Dump the contents of each image in the ImageList to a file, stdout or a string.

            This function is intended just for debugging. It prints the contents of an image
            to the file path specified by `filename`.
            If a `filename` is not specified, output goes to stdout (unless `show` is False).
            In both cases the contents are also returned as a string.

            Parameters
            ----------
            filename : str, optional
                File to dump file image contents to
            mode : str, optional
                Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
                but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
                it if it does).
            window : tuple(int,int,int,int), optional
              Window to dump with `value` in the format (llx, lly, urx, ury) where:
              - `llx` Lower left X coordinate
              - `lly` Lower left Y coordinate
              - `urx` Upper right X coordinate
              - `ury` Upper right Y coordinate
            show : bool, optional
                Send image contents to stdout. Defaults to True.

            Returns
            --------
            str
                Multiline string containing the dump of the image contents in the ImageList.


          )pydoc")
      .def(
          "add_imagelist",
          [](hdrl::core::ImageList& self, hdrl::core::ImageList& himglist) {
            self.add_imagelist(himglist);
          },
          py::arg("himagelist"), R"pydoc(
        Add this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is added to the image number n in this list.

        Parameters
        ----------
        himglist : hdrl.core.ImageList
            ImageList to add

        See Also
        ---------
        hdrl.core.Image.add_image : Adds values from Image other to self. Modified in place.
      )pydoc")
      .def(
          "sub_imagelist",
          [](hdrl::core::ImageList& self, hdrl::core::ImageList& himglist) {
            self.sub_imagelist(himglist);
          },
          py::arg("himagelist"), R"pydoc(
        Elementwise subtract this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is subtracted from the image number n in this list.

        Parameters
        ----------
        himglist: hdrl.core.ImageList
            ImageList to subtract with

        See Also
        ---------
        hdrl.core.Image.sub_image : Subtracts other Image values from self Image values. Modified in place.
        )pydoc")
      .def(
          "mul_imagelist",
          [](hdrl::core::ImageList& self, hdrl::core::ImageList& himglist) {
            self.mul_imagelist(himglist);
          },
          py::arg("himagelist"), R"pydoc(
        Multiply this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is multiplied the image number n in this list.

        Parameters
        ----------
        himglist : hdrl.core.ImageList
            ImageList to multiply with

        See Also
        --------
        hdrl.core.Image.mul_image :  Multiplies self Image values by other Image values. Modified in place.
        )pydoc")
      .def(
          "div_imagelist",
          [](hdrl::core::ImageList& self, hdrl::core::ImageList& himglist) {
            self.div_imagelist(himglist);
          },
          py::arg("himagelist"), R"pydoc(
        Divide this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other divides the image number n in this list.

        Parameters
        ----------
        himglist : hdrl.core.ImageList
            ImageList to divide with

        See Also
        --------
        hdrl.core.Image.div_image : Divides self Image values by other Image values. Modified in place.
        )pydoc")
      .def(
          "add_image",
          [](hdrl::core::ImageList& self, hdrl::core::Image himg) {
            self.add_image(himg);
          },
          py::arg("himg"), R"pydoc(
        Add an Image to this ImageList. Modified in place.

        The input image must have the same size as those in this Imagelist, the input image is added elementwise to each image in this list.

        Parameters
        ----------
        himg : hdrl.core.Image
            Image to add

        See Also
        --------
        hdrl.core.Image.add_image : Adds values from Image other to self. Modified in place.
        )pydoc")
      .def(
          "sub_image",
          [](hdrl::core::ImageList& self, hdrl::core::Image himg) {
            self.sub_image(himg);
          },
          py::arg("himg"), R"pydoc(
        Subtract an Image from this ImageList. Modified in place.

        The input image must have the same size as those in this Imagelist, each image in this list is subtracted elementwise by the input image.

        Parameters
        ----------
        himglist : hdrl.core.Image
            Image to subtract with

        See Also
        --------
        hdrl.core.Image.sub_image : Subtracts other Image values from self Image values. Modified in place.
        )pydoc")
      .def(
          "mul_image",
          [](hdrl::core::ImageList& self, hdrl::core::Image himg) {
            self.mul_image(himg);
          },
          py::arg("himg"), R"pydoc(
        Multiply this ImageList by an Image. Modified in place.

        The input image must have the same size as those in this Imagelist, each image in this list is multiplied elementwise by the input image.

        Parameters
        ----------
        himglist : hdrl.core.Image
            Image to multiply with

        See Also
        --------
        hdrl.core.Image.mul_image :  Multiplies self Image values by other Image values. Modified in place.
        )pydoc")
      .def(
          "div_image",
          [](hdrl::core::ImageList& self, hdrl::core::Image himg) {
            self.div_image(himg);
          },
          py::arg("himg"), R"pydoc(
        Divide this ImageList by an Image. Modified in place.

        The input image must have the same size as those in this Imagelist, each image in this list is divided elementwise by the input image.

        Parameters
        ----------
        himglist : hdrl.core.Image
            Image to divide with

        See Also
        --------
        hdrl.core.Image.div_image : Divides self Image values by other Image values. Modified in place.
        )pydoc")
      .def(
          "add_scalar",
          [](hdrl::core::ImageList& self, hdrl::core::Value val) {
            self.add_scalar(val);
          },
          py::arg("val"), R"pydoc(
        Elementwise addition of a scalar to each image in the ImageList. Modified in place

        Parameters
        ----------
        value : tuple (float, float)
            Value to add. The first component is the scalar number to add, the second is the error value.

        See Also
        --------
        hdrl.core.Image.add_scalar : Elementwise addition of a scalar to an image. Modified in place.
        )pydoc")
      .def(
          "sub_scalar",
          [](hdrl::core::ImageList& self, hdrl::core::Value val) {
            self.sub_scalar(val);
          },
          py::arg("val"), R"pydoc(
        Elementwise subtraction of a scalar to each image in the ImageList. Modified in place.

        Parameters
        ----------
        value : tuple (float, float)
            Value to subtract. The first component is the scalar number to subtract, the second is the error value.

        See Also
        --------
        hdrl.core.Image.sub_scalar : Elementwise subtraction of a scalar from an image. Modified in place.

        )pydoc")
      .def(
          "mul_scalar",
          [](hdrl::core::ImageList& self, hdrl::core::Value val) {
            self.mul_scalar(val);
          },
          py::arg("val"), R"pydoc(
        Elementwise multiplication of a scalar to each image in the ImageList.

        Parameters
        ----------
        value : tuple (float, float)
            Value to multiply with. The first component is the multiplicator, the second is the error value.

        See Also
        --------
        hdrl.core.Image.mul_scalar : Elementwise multiplication of an image by a scalar. Modified in place.
        )pydoc")
      .def(
          "div_scalar",
          [](hdrl::core::ImageList& self, hdrl::core::Value val) {
            self.div_scalar(val);
          },
          py::arg("val"), R"pydoc(
        Elementwise division of each image in the ImageList with a scalar.

        Parameters
        ----------
        value : tuple (float, float)
            Non-zero number to divide with. The first component is the divisor, the second is the error value.

        See Also
        --------
        hdrl.core.Image.div_scalar : Elementwise division of an image with a scalar. Modified in place.
        )pydoc")
      .def(
          "pow_scalar",
          [](hdrl::core::ImageList& self, hdrl::core::Value val) {
            self.pow_scalar(val);
          },
          py::arg("val"), R"pydoc(
        Compute the elementwise exponential of each image in `self`. Modified in place.

        Parameters
        ----------
        base : tuple (float, float)
            Base of the exponential. The first component is the base, the second is the error value.

        See Also
        --------
        hdrl.core.Image.pow_scalar : Computes the power of an image by a scalar. Modified in place.
        )pydoc");

  /*.def_property_readonly("width",&hdrl::core::ImageList::get_width,
                         "int: width of the image")
  .def_property_readonly("height",&hdrl::core::ImageList::get_height,
                         "int: height of the image")*/
  /*.def_property_readonly("image",&hdrl::core::ImageList::get_image,
                         "cpl.core.Image: the hdrl primary image")
  .def_property_readonly("error",&hdrl::core::ImageList::get_error,
                         "cpl.core.Image: the hdrl error image") */

  /*.def("dump", &hdrl::core::Image::dump)
  .def_static("check", [](hdrl::core::pycpl_image i){
      hdrl::core::Image::check(i);
  }, py::arg("image"), R"docstring(
  "Check an image
  )docstring")
  .def_static("pass_through", [](hdrl::core::pycpl_image i){
      return hdrl::core::Image::pass_through(i);
  }, py::arg("image"), R"docstring(
  "Pass through an image
  )docstring")
*/
}
