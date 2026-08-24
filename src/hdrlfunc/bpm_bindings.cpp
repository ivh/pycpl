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

#include "hdrlfunc/bpm_bindings.hpp"

#include <memory>

#include <cpl_filter.h>
#include <cpl_type.h>
#include <hdrl_bpm_2d.h>
#include <hdrl_bpm_3d.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_image.hpp"      // IWYU pragma: keep
#include "hdrlcore/pycpl_imagelist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"       // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlcore/pycpl_vector.hpp"  // IWYU pragma: keep
#include "hdrlfunc/bpm.hpp"

namespace py = pybind11;

void
bind_bpm(py::module& m)
{
  // BPM
  py::class_<hdrl::func::BPM, std::shared_ptr<hdrl::func::BPM>> bpm_class(
      m, "BPM", py::buffer_protocol());

  bpm_class.doc() = R"docstring(
      The hdrl.func.BPM class provides an interface to various Bad Pixel Mask algorithms.
      This module contains static functions to detect bad pixels on single images, on a
      stack of identical images, and on a sequence of images.
      )docstring";

  bpm_class
      .def_static(
          "filter",
          [](hdrl::core::pycpl_mask input_mask, cpl_size kernel_nx,
             cpl_size kernel_ny, cpl_filter_mode filter) {
            return hdrl::func::BPM::filter(input_mask, kernel_nx, kernel_ny,
                                           filter);
          },
          py::arg("input_mask"), py::arg("kernel_nx"), py::arg("kernel_ny"),
          py::arg("filter"), R"docstring(
        Sets pixels to bad if the pixel is surrounded by other bad pixels.
        Allows the growing and shrinking of bad pixel masks. 
 
        The algorithm assumes that all pixels outside the mask are good, i.e. it
        enlarges the mask by the kernel size and marks this border as good. It
        applies on the enlarged mask during the operation and extracts the original-size
        mask at the very end.
        
        Parameters
        ----------
            input_mask : cpl.core.Mask
                Input mask
            kernel_nx : int
                Size in x-direction of the filtering kernel
            kernel_ny : int
                Size in y-direction of the filtering kernel
            filter : cpl.core.Filter mode
                Filter modes as defined in PyCPL.
                Supported modes: cpl.core.Filter.EROSION, cpl.core.Filter.DILATION, cpl.core.Filter.OPENING, cpl.core.Filter.CLOSING, cpl.core.Filter.LINEAR

        Returns
        -------
        cpl.core.Mask
            mask of a defined size.

        See Also
        ---------
        hdrl.func.BPM.filter_list : Wrapper around hdrl.func.BPM.filter() to filter list of images.
      )docstring")
      .def_static(
          "filter_list",
          [](hdrl::core::pycpl_imagelist inlist, cpl_size kernel_nx,
             cpl_size kernel_ny, cpl_filter_mode filter) {
            return hdrl::func::BPM::filter_list(inlist, kernel_nx, kernel_ny,
                                                filter);
          },
          py::arg("inlist"), py::arg("kernel_nx"), py::arg("kernel_ny"),
          py::arg("filter"), R"docstring(
        Wrapper around hdrl.func.BPM.filter() to filter list of images
        
        Parameters
        ----------
            inlist : cpl.core.ImageList
                Input image list
            kernel_nx : int
                Size in x-direction of the filtering kernel
            kernel_ny : int
                Size in y-direction of the filtering kernel
            filter : cpl.core.Filter mode
                Filter modes as defined in PyCPL.
                Supported modes: cpl.core.Filter.EROSION, cpl.core.Filter.DILATION, cpl.core.Filter.OPENING, cpl.core.Filter.CLOSING, cpl.core.Filter.LINEAR

        Returns
        -------
        cpl.core.ImageList
            the filtered image list.
        
        See Also
        --------
        hdrl.func.BPM.filter : Sets pixels to bad if the pixel is surrounded by other bad pixels.
      )docstring");
  // FIXME: Check if this is just dead code or if it is unfinished. Keep it
  //        disabled for now!
#if 0
      .def_static(
          "to_mask",
          [](hdrl::core::pycpl_image bpm, uint64_t selection) {
            return hdrl::func::BPM::to_mask(bpm, selection);
          },
          py::arg("bpm"), py::arg("selection"), R"docstring(
          Converts bad pixel information mask to a cpl.core.mask

          Parameters
          ----------
              bpm : hdrl.core.Image
                  integer image containing the bad pixel information
              selection : uint64_t
                  bit-mask selecting which values to set to bad

          Returns
          -------
          cpl.core.Mask

          Note
          -----
          As PyCpl only supports 32 bit integer images the top 32 bit of the
          selection mask must be zero

          See Also
          --------
          hdrl.func.BPM.filter : Sets pixels to bad if the pixel is
          surrounded by other bad pixels. hdrl.func.BPM.filter_list : Wrapper
          around hdrl.func.BPM.filter() to filter list of images
        )docstring");
#endif

  // BPM2D
  py::class_<hdrl::func::BPM2D, std::shared_ptr<hdrl::func::BPM2D>> bpm2d_class(
      m, "BPM2D", py::buffer_protocol());

  bpm2d_class.doc() = R"docstring(
      The hdrl.func.BPM2D class provides an interface to the bad pixel mask 2D algorithm.
      Bad pixels on single images, on a stack of identical images, and
      on a sequence of images can be detected.

      The algorithm first smoothes the image by applying different methods.
      Then it subtracts the smoothed image and derives bad pixels by thresholding
      the residual image, i.e. all pixels exceeding the threshold are considered bad.

      In order to create instances of the hdrl.func.BPM2D class, two methods are
      available depending on the smoothing algorithm used, namely hdrl.func.BPM2D.Method.Filter
      and hdrl.func.BPM2D.Method.Legendre.  
      )docstring";

  bpm2d_class
      .def_static(
          "Filter",
          [](double kappa_low, double kappa_high, int maxiter,
             cpl_filter_mode filter, cpl_border_mode border, int smooth_x,
             int smooth_y) {
            return new hdrl::func::BPM2D(kappa_low, kappa_high, maxiter, filter,
                                         border, smooth_x, smooth_y);
          },
          py::arg("kappa_low"), py::arg("kappa_high"), py::arg("maxiter"),
          py::arg("filter"), py::arg("border"), py::arg("smooth_x"),
          py::arg("smooth_y"), R"docstring(
        Creates an instance of hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Filter

       Parameters
       ----------
           kappa_low : float
              Low kappa factor for thresholding algorithm
           kappa_high : float
              High kappa factor for thresholding algorithm
           maxiter : int
              Maximum number of iterations
           filter : cpl.core.Filter mode
              Filter mode as defined in PyCPL.
              Supported modes: cpl.core.Filter.EROSION, cpl.core.Filter.DILATION, cpl.core.Filter.OPENING, cpl.core.Filter.CLOSING, cpl.core.Filter.LINEAR
           border : cpl.core.Border mode
              Border mode as defined in PyCPL.
              Supported modes: cpl.core.Border.FILTER. cpl.core.Border.ZERO, cpl.core.Border.CROP, cpl.core.Border.NOP, cpl.core.Border.COPY
           smooth_x : int
              Smoothing kernel size in the x-direction 
           smooth_y : int
              Smoothing kernel size in the y-direction
       
       Returns
       -------
       The hdrl.func.BPM2D
           An instance of the hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Filter.
   
       Example
       -------
       .. code-block:: python
   
           fs = hdrl.func.BPM2D.Filter(4.0, 5.0, 6, cpl.core.Filter.MEDIAN, cpl.core.Border.NOP, 7, 9)
   
       Notes
       -----
       Filter smooth algorithm: 
       This instance applies a filter like e.g. a median filter to the image. The filtering can be
       done by all modes currently supported by CPL and is controlled by the filter type, the border type
       and the kernel size in x and y.
       
       See Also
       --------
       hdrl.func.BPM2D.Legendre : Creates an instance of hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Legendre.
       hdrl.func.BPM2D.compute : Detect bad pixels on a single image with an iterative process.
      )docstring")
      .def_static(
          "Legendre",
          [](double kappa_low, double kappa_high, int maxiter, int steps_x,
             int steps_y, int filter_size_x, int filter_size_y, int order_x,
             int order_y) {
            return new hdrl::func::BPM2D(kappa_low, kappa_high, maxiter,
                                         steps_x, steps_y, filter_size_x,
                                         filter_size_y, order_x, order_y);
          },
          py::arg("kappa_low"), py::arg("kappa_high"), py::arg("maxiter"),
          py::arg("steps_x"), py::arg("steps_y"), py::arg("filter_size_x"),
          py::arg("filter_size_y"), py::arg("order_x"), py::arg("order_y"),
          R"docstring(
      Creates an instance of hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Legendre.

      Parameters
      ----------
      kappa_low : float
          Low kappa factor for the thresholding algorithm
      kappa_high : float
          High kappa factor for the thresholding algorithm
      maxiter : int
          Maximum number of iterations
      steps_x : int
          Number of sampling points in the x-direction
      steps_y : int
          Number of sampling points in the y-direction
      filter_size_x : int
          Size of the median box in the x-direction
      filter_size_y : int
          Size of the median box in the y-direction
      order_x : int
          Order of polynomial in the x-direction
      order_y : int
          Order of polynomial in the y-direction
  
      Returns
      -------
      The hdrl.func.BPM2D
          An instance of the hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Legendre.
  
      Example
      -------
      .. code-block:: python
  
          ls = hdrl.func.BPM2D.Legendre(4, 5, 6, 20, 21, 11, 12, 2, 10)
  
      Notes
      ----
      This instance of a hdrl.func.BPM2D class fits a Legendre polynomial to the image of order order_x,
      in x and order_y in y direction. This method allows you to define steps_x and steps_y
      sampling points (the latter are computed as the median within a box of filter_size_x
      and filter_size_y) where the polynomial is fitted. This substantially decreases the
      fitting time for the Legendre polynomial. 
      
      See Also
      --------
      hdrl.func.BPM2D.compute : Detect bad pixels on a single image with an iterative process.
      hdrl.func.BPM2D.Filter : Creates an instance of hdrl.func.BPM2D for the method hdrl.func.BPM2D.Method.Filter.
      )docstring")

      .def(
          "compute",
          [](hdrl::func::BPM2D& self,
             std::shared_ptr<hdrl::core::Image> img_in) {
            return self.compute(img_in);
          },
          py::arg("img_in"), R"docstring(
        Detects bad pixels on a single image with an iterative process

        Parameters
        ----------
        img_in : hdrl.core.Image
            Input image
  
        Returns
        -------
        cpl.core.Mask
            Bad pixel mask with the newly found bad pixels.

        Notes
        -----
        The algorithm first smoothes the image by applying the methods
        described under hdrl.func.BPM2D.Filter or hdrl.func.BPM2D.Legendre. 
        Then it subtracts the smoothed image and derives bad pixels by 
        thresholding the residual image, i.e. all pixels exceeding
        the threshold are considered bad. To compute the upper and lower
        thresholds, it measures a robust rms (a properly scaled Median Absolute
        Deviation), which is then scaled by the parameters  kappa_low and 
        kappa_high. Furthermore, the algorithm is applied iteratively
        controlled by  maxiter. During each iteration, the newly found bad
        pixels are ignored. Please note that the thresholding values are
        applied as median (residual-image) :math:`\pm` thresholds. This makes the
        algorithm more robust in the case that the methods are
        not able to completely remove the background level, e.g due to an
        exceeding number of bad pixels in the first iteration.
        
        See Also
        --------
        hdrl.func.BPM2D.Filter : Creates an instance of hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Filter.
        hdrl.func.BPM2D.Legendre : Creates an instance of hdrl.func.BPM2D class for the method hdrl.func.BPM2D.Method.Legendre.
      )docstring")
      .def_property_readonly("filter", &hdrl::func::BPM2D::get_filter,
                             "cpl.core.Filter : filter mode")
      .def_property_readonly("border", &hdrl::func::BPM2D::get_border,
                             "cpl.core.Border : border mode")
      .def_property_readonly(
          "kappa_low", &hdrl::func::BPM2D::get_kappa_low,
          "float : Low kappa factor for thresholding algorithm")
      .def_property_readonly(
          "kappa_high", &hdrl::func::BPM2D::get_kappa_high,
          "float : High kappa factor for thresholding algorithm")
      .def_property_readonly("maxiter", &hdrl::func::BPM2D::get_maxiter,
                             "int : Maximum number of iterations")
      .def_property_readonly(
          "steps_x", &hdrl::func::BPM2D::get_steps_x,
          "int : Number of sampling points in the x-direction")
      .def_property_readonly(
          "steps_y", &hdrl::func::BPM2D::get_steps_y,
          "int : Number of sampling points in the y-direction")
      .def_property_readonly("filter_size_x",
                             &hdrl::func::BPM2D::get_filter_size_x,
                             "int : Size of the median box in the x-direction")
      .def_property_readonly("filter_size_y",
                             &hdrl::func::BPM2D::get_filter_size_y,
                             "int : Size of the median box in the y-direction")
      .def_property_readonly("order_x", &hdrl::func::BPM2D::get_order_x,
                             "int : Order of polynomial in the x-direction")
      .def_property_readonly("order_y", &hdrl::func::BPM2D::get_order_y,
                             "int : Order of polynomial in the y-direction")
      .def_property_readonly("smooth_x", &hdrl::func::BPM2D::get_smooth_x,
                             "int : Smoothing kernel size in the x-direction")
      .def_property_readonly("smooth_y", &hdrl::func::BPM2D::get_smooth_y,
                             "int : Smoothing kernel size in the y-direction")
      .def_property_readonly(
          "method", &hdrl::func::BPM2D::get_method,
          "hdrl.func.BPM2D.Method : Selected algorithm method");

  py::enum_<hdrl_bpm_2d_method>(bpm2d_class, "Method",
                                "The method to be used when creating an "
                                "instance of the hdrl.func.BPM2D class.")
      .value("Legendre", HDRL_BPM_2D_LEGENDRESMOOTH, R"pydoc(
        This algorithm will use Legendre smoothing techniques to detect bad pixels. Fitting a Legendre
        polynomial to the image of order `order_x` in x and `order_y` in y direction. This method allows you to
        define `steps_x` :math:`\times` `steps_y` sampling points (the latter are computed as the median within a box of 
        `filter_size_x` and `filter_size_y`) where the polynomial is fitted. This substantially decreases the 
        fitting time for the Legendre polynomial.
      )pydoc")
      .value("Filter", HDRL_BPM_2D_FILTERSMOOTH, R"pydoc(
        This algorithm will use Filter smoothing techniques to detect bad pixels. Applying a filter
        like a median filter to the image. The filtering can be done by all modes currently supported
        by PyCPL and is controlled by the cpl.core.Filter type `filter`, the cpl.core.Border type `border`, and by the kernel size
        in x and y, i.e. `smooth_x`, and `smooth_y`.
      )pydoc");


  // BPM3D
  py::class_<hdrl::func::BPM3D, std::shared_ptr<hdrl::func::BPM3D>> bpm3d_class(
      m, "BPM3D", py::buffer_protocol());

  bpm3d_class.doc() = R"docstring(
      The hdrl.func.BPM3D class provides an interface to the bad pixel mask 3D algorithm.
      This algorithm detects bad pixels on a stack of identical images in an imagelist.
      Once the class is instantiated with the desired parameters, the compute function 
      may be called to run the algorithm.


      Parameters
      ----------
      kappa_low : float
          Low kappa factor for thresholding algorithm
      kappa_high : float
          High kappa factor for thresholding algorithm
      method : hdrl.func.BPM3D.Method
          Selected algorithm method
  
      Returns
      -------
      hdrl.func.BPM3D

      Notes
      -----
      There are three algorithm methods to be selected from: 

      - hdrl.func.BPM3D.Method.Absolute: It uses `kappa_low` and `kappa_high` as absolute threshold.
      - hdrl.func.BPM3D.Method.Relative: It scales the measured rms on the residual-image with
        `kappa_low` and `kappa_high` and uses it as threshold. For the rms a properly scaled Median
        Absolute Deviation (MAD) is used.
      - hdrl.func.BPM3D.Method.Error: It scales the propagated error of each individual
        pixel with `kappa_low` and `kappa_high` and uses it as threshold.
      
      Example
      -------
      .. code-block:: python
  
        bpm_3d = hdrl.func.BPM3D(4, 5, hdrl.func.BPM3D.Method.Absolute)
        bpm_3d = hdrl.func.BPM3D(4, 5, hdrl.func.BPM3D.Method.Relative)
        bpm_3d = hdrl.func.BPM3D(4, 5, hdrl.func.BPM3D.Method.Error)
  
      See Also
      --------
      hdrl.func.BPM3D.compute : Detect bad pixels on a stack of identical images.
      
      )docstring";

  bpm3d_class
      .def(py::init<double, double, hdrl_bpm_3d_method>(), py::arg("kappa_low"),
           py::arg("kappa_high"), py::arg("method"))
      .def(
          "compute",
          [](hdrl::func::BPM3D& self,
             std::shared_ptr<hdrl::core::ImageList> imglist_in) {
            return self.compute(imglist_in);
          },
          py::arg("imglist_in"), R"docstring(
        Detects bad pixels on a stack of identical images.
        
        Parameters
        ----------
        imglist : hdrl.core.ImageList
            input imagelist.

        Returns
        -------
        cpl.core.ImageList
            imagelist containing the newly found bad pixels for each input image with the same pixel
            coding as for a cpl.core.Mask bad pixel mask, i.e. 0 for good pixels and 1 for bad pixels.
            Please note that already known bad pixels given to the routine will not be included in
            the output mask.

        Notes
        -----
        The algorithm first collapses the stack of images by using the median in order
        to generate a master image. Then it subtracts the master image from each individual
        image and derives the bad pixels on the residual images by thresholding, i.e. all
        pixels exceeding the threshold are considered bad. 
        
        Please note that the algorithm assumes that the mean level of the different images
        is the same. If this is not the case, then the master image as described above 
        will be biased.

        See Also
        --------
        hdrl.func.BPM3D : Creates an instance of the hdrl.func.BPM3D class for the imagelist method.
      )docstring")
      .def_property_readonly(
          "kappa_low", &hdrl::func::BPM3D::get_kappa_low,
          "float : Low kappa factor for thresholding algorithm")
      .def_property_readonly(
          "kappa_high", &hdrl::func::BPM3D::get_kappa_high,
          "float : High kappa factor for thresholding algorithm")
      .def_property_readonly(
          "method", &hdrl::func::BPM3D::get_method,
          "hdrl.func.BPM3D.Method : Selected algorithm method");

  py::enum_<hdrl_bpm_3d_method>(bpm3d_class, "Method",
                                "The method to be used when creating an "
                                "instance of the hdrl.func.BPM3D class.")
      .value("Absolute", HDRL_BPM_3D_THRESHOLD_ABSOLUTE, R"pydoc(
        Uses kappa_low and kappa_high as absolute thresholds.
      )pydoc")
      .value("Relative", HDRL_BPM_3D_THRESHOLD_RELATIVE, R"pydoc(
        Scales the measured RMS on the residual image with
        kappa_low and kappa_high and uses it as thresholds. For the rms a properly
        scaled Median Absolute Deviation (MAD) is used.
      )pydoc")
      .value("Error", HDRL_BPM_3D_THRESHOLD_ERROR, R"pydoc(
        Scales the propagated error of each individual
        pixel with kappa_low and kappa_high and uses it as a threshold.
      )pydoc");

  // BPMFit
  py::class_<hdrl::func::BPMFit, std::shared_ptr<hdrl::func::BPMFit>>
      bpmfit_class(m, "BPMFit", py::buffer_protocol());

  bpmfit_class.doc() = R"docstring(
      The hdrl.func.BPMFit module provides an interface to various bad pixel mask fit algorithms
      to detect bad pixels on a sequence of images e.g. domeflats.

      The algorithm fits a polynomial to each pixel sequence and determines bad
      pixels based on this fit and various thresholding methods.

      Three thresholding methods are available to convert the information from the fit into a bad pixel map:

        - PVal: Pixels with p-values below the mentioned threshold are considered as bad pixels.
        - RelChi: Pixels with a chi value below the mentioned threshold are considered as bad pixels.
        - RelCoef: Pixels with a fit coefficient below the mentioned threshold are considered as bad pixels.

        In order to create an instance of the hdrl.func.BPMFit class, three constructors
        corresponding to the above methods are available. 
      )docstring";

  bpmfit_class
      .def_static(
          "PVal",
          [](int degree, double pval) {
            return new hdrl::func::BPMFit(degree, pval);
          },
          py::arg("degree"), py::arg("pval"), R"docstring(
        Creates an instance of hdrl.func.BPMFit class for the p-value method.

        Parameters
        ----------
            degree : int
                The degree of the fit
            pval : float
                The p-value bpm cutoff

        Notes
        -----
        Pixels with low p-value. When the errors of the pixels are correct the p-value can
        be interpreted as the probability with which the pixel response fits the chosen model.

        See Also
        --------
        hdrl.func.BPMFit.compute : Derives bad pixels on a sequence of images by fitting a polynomial along each pixel sequence of the images. 
        hdrl.func.BPMFit.RelChi : Creates an instance of hdrl.func.BPMFit class with relative chi bpm threshold.
        hdrl.func.BPMFit.RelCoef : Creates an instance of hdrl.func.BPMFit class with relative coefficient bpm threshold.
      )docstring")
      .def_static(
          "RelChi",
          [](int degree, double low, double high) {
            return new hdrl::func::BPMFit("chi", degree, low, high);
          },
          py::arg("degree"), py::arg("low"), py::arg("high"), R"docstring(
        Creates an instance of hdrl.func.BPMFit class with a relative chi bpm threshold.

        Parameters
        ----------
            degree : int
                The degree of the fit
            low : float
                Relative chi distribution bpm lower threshold
            high : float
                Relative chi distribution bpm upper threshold
        
        Returns
        -------
        hdrl.func.BPMFit

        Notes
        -----
        Relative cutoff on the chi distribution of all fits. Pixels with chi values which
        exceed mean :math:`\pm` cutoff :math:`\times` standard deviation are considered bad.

        See Also
        --------
        hdrl.func.BPMFit.compute : Derives bad pixels on a sequence of images by fitting a polynomial along each pixel sequence of the images. 
        hdrl.func.BPMFit.PVal : Creates an instance of hdrl.func.BPMFit class with p-value bpm threshold.
        hdrl.func.BPMFit.RelCoef : Creates an instance of hdrl.func.BPMFit class with relative coefficient bpm threshold.
      )docstring")
      .def_static(
          "RelCoef",
          [](int degree, double low, double high) {
            return new hdrl::func::BPMFit("coef", degree, low, high);
          },
          py::arg("degree"), py::arg("low"), py::arg("high"), R"docstring(
        Creates an instance of hdrl.func.BPMFit class with relative coefficient bpm threshold.

        Parameters
        ----------
            degree : int
                The degree of the fit
            low : float
                Relative fit coefficient distribution bpm lower threshold
            high : float
                Relative fit coefficient distribution bpm upper threshold
        
        Returns
        -------
        hdrl.func.BPMFit

        Notes
        -----
        Relative cutoff on the distribution of the fit coefficients. Pixels with fit coefficients which
        exceed mean :math:`\pm` cutoff :math:`\times` standard deviation are considered bad.

        See Also
        --------
        hdrl.func.BPMFit.compute : Derives bad pixels on a sequence of images by fitting a polynomial along each pixel sequence of the images. 
        hdrl.func.BPMFit.PVal : Creates an instance of hdrl.func.BPMFit class with p-value bpm threshold.
        hdrl.func.BPMFit.RelChi : Creates an instance of hdrl.func.BPMFit class with relative chi bpm threshold.
        
      )docstring")
      .def(
          "compute",
          [](hdrl::func::BPMFit& self,
             std::shared_ptr<hdrl::core::ImageList> imglist_in,
             hdrl::core::pycpl_vector sample_position) {
            return self.compute(imglist_in, sample_position);
          },
          py::arg("imglist_in"), py::arg("sample_position"), R"docstring(
      
        Derives bad pixels on a sequence of images by fitting a polynomial 
        along each pixel sequence of the images.
        
        Parameters
        ----------
            imglist_in : hdrl.core.ImageList
                imagelist to fit
            sample_position : cpl.core.Vector
                vector of sampling position of the images in data, e.g. exposure time
          
        Returns
        -------
        cpl.core.Image
            image with cpl.core.Type.INT pixels containing the 
            newly found bad pixels for each input image

        Notes
        -----
        When using hdrl.func.BPMFit.RelCoef the value of the returned image encodes 
        the coefficient that was outside the relative threshold as a power of two. 
        e.g. if coefficient 0 and 3 of the fit were not within the threshold for a pixel, 
        it will have the value :math:`2^0 + 2^3 = 9`. The other hdrl.func.BPMFit algorithms 
        return an image with non-zero values marking pixels outside the selection thresholds.

        Please note that already known bad pixels given to the routine will not be included in the output mask.

        See Also
        --------
        hdrl.func.BPMFit.PVal : Creates an instance of hdrl.func.BPMFit class with p-value bpm threshold.
        hdrl.func.BPMFit.RelChi : Creates an instance of hdrl.func.BPMFit class with relative chi bpm threshold.
        hdrl.func.BPMFit.RelCoef : Creates an instance of hdrl.func.BPMFit class with relative coefficient bpm threshold.
      )docstring")
      .def_property_readonly("degree", &hdrl::func::BPMFit::get_degree,
                             "int : The degree of the fit")
      .def_property_readonly("pval", &hdrl::func::BPMFit::get_pval,
                             "float: The p-value bpm cutoff")
      .def_property_readonly(
          "chi_low", &hdrl::func::BPMFit::get_rel_chi_low,
          "float : Relative chi distribution bpm lower threshold")
      .def_property_readonly(
          "chi_high", &hdrl::func::BPMFit::get_rel_chi_high,
          "float : Relative chi distribution bpm upper threshold")
      .def_property_readonly(
          "coef_low", &hdrl::func::BPMFit::get_rel_coef_low,
          "float : Relative fit coefficient distribution bpm lower threshold")
      .def_property_readonly(
          "coef_high", &hdrl::func::BPMFit::get_rel_coef_high,
          "float : Relative fit coefficient distribution bpm upper threshold");
}
