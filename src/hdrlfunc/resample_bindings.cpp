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

#include "hdrlfunc/resample_bindings.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_propertylist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_table.hpp"         // IWYU pragma: keep
#include "hdrlcore/pycpl_wcs.hpp"           // IWYU pragma: keep
#include "hdrlfunc/resample.hpp"


namespace py = pybind11;

void
bind_resample(py::module& m)
{
  // necessary to declare ahead of time to get around docstring bug see
  // https://pybind11.readthedocs.io/en/stable/advanced/misc.html#avoiding-c-types-in-docstrings
  auto resample_class =
      py::class_<hdrl::func::Resample, std::shared_ptr<hdrl::func::Resample>>(
          m, "Resample", py::buffer_protocol());
  auto resampleresult_class =
      py::class_<hdrl::func::ResampleResult,
                 std::shared_ptr<hdrl::func::ResampleResult>>(
          m, "ResampleResult", py::buffer_protocol());
  auto resampleoutgrid_class =
      py::class_<hdrl::func::ResampleOutgrid,
                 std::shared_ptr<hdrl::func::ResampleOutgrid>>(
          m, "ResampleOutgrid", py::buffer_protocol());
  auto resamplemethod_class =
      py::class_<hdrl::func::ResampleMethod,
                 std::shared_ptr<hdrl::func::ResampleMethod>>(
          m, "ResampleMethod", py::buffer_protocol());

  // Resample
  resample_class.doc() = R"docstring(
      A hdrl.func.Resample class provides an interface to static functions 
      required to resample images and cubes. 
      )docstring";

  resample_class
      .def_static(
          "compute",
          [](const hdrl::core::pycpl_table& restable,
             const hdrl::func::ResampleMethod& method,
             const hdrl::func::ResampleOutgrid& outputgrid,
             const hdrl::core::pycpl_wcs& wcs) {
            return hdrl::func::Resample::compute(restable, method, outputgrid,
                                                 wcs);
          },
          py::arg("restable"), py::arg("method"), py::arg("outputgrid"),
          py::arg("wcs"),
          R"docstring(
      This routine does not work directly on an image or cube but on a table (`restable`). 
      
      For 2D images, the table is created by the function hdrl.func.Resample.image_to_table(), 
      whereas for a 3D data cube the function hdrl.func.Resample.imagelist_to_table() is used. 

      In the case that many images or cubes have to be combined into a single mosaic, the two functions
      can be called multiple times and the returned tables should be merged into a single table using
      cpl.core.Table.insert().

      Parameters
      ----------
      restable : cpl.core.Table
        A PyCPL table with information on the data to be resampled. 
        
        It can be created either via hdrl.func.Resample.image_to_table() for 
        a 2D image or via hdrl.func.Resample.imagelist_to_table() for a 3D data cube.
        These functions require either an hdrl.core.Image (2D image) or hdrl.core.ImageList (3D data cube), 
        plus a valid cpl.drs.WCS object that encodes the world coordinate system of the given image or cube. 

        Alternatively, in case the above mentioned functions can not be used to create the table, 
        e.g. the pixel to sky mapping is very complex and can not be encoded by the cpl.drs.WCS object, 
        the pipeline developer has to create and fill the table. A template of this table can be
        generated using the function hdrl.func.Resample.restable_template().

      method : hdrl.func.ResampleMethod
        The interpolation `method` to apply as specified via an instance of the
        hdrl.func.ResampleMethod class, which can be instantiated via one of 
        the following constructors: 
        - hdrl.func.ResampleMethod.Nearest: Nearest neighbour resampling.
        - hdrl.func.ResampleMethod.Linear: Weighted resampling using an inverse distance weighting function.
        - hdrl.func.ResampleMethod.Quadratic: Weighted resampling using a quadratic inverse distance weighting function.
        - hdrl.func.ResampleMethod.Renka: Weighted resampling using a Renka weighting function.
        - hdrl.func.ResampleMethod.Drizzle: Weighted resampling using a drizzle-like weighting scheme.
        - hdrl.func.ResampleMethod.Lanczos: Weighted resampling using a Lanczos-like restricted sinc as weighting function.

      outputgrid : hdrl.func.ResampleOutgrid
        Defines basic properties of the resampled image or cube. Depending on the input
        data (image or cube), `outputgrid` should be an instance of the 
        hdrl.func.ResampleOutgrid class, which can be instantiated via one of
        the following constructors:
        - hdrl.func.ResampleOutgrid.User2D : User specified function for 2D images.
        - hdrl.func.ResampleOutgrid.User3D : User specified function for 3D data cubes.
        - hdrl.func.ResampleOutgrid.Auto2D : Convenience function for 2D images.
        - hdrl.func.ResampleOutgrid.Auto3D : Convenience function for 3D data cubes.
        In the case of the Auto2D and Auto3D constructors, only the step sizes in 
        right ascension, declination and wavelength of the output image or cube are required. 
        All the rest are automatically derived from the data by hdrl.func.Resample.compute().
      
      wcs : cpl.drs.WCS
        The World Coordinate System representative of the images to be resampled. 
        The resampling functions use the `wcs` input (CD matrix) mostly to determine the 
        scales between the input and output grid. 
        Please note, that in case the user would like to combine images or cubes with 
        substantially different pixel sizes into a single output image or cube, the 
        single tables have to be properly scaled to the same scales before merging them into the final table.

      Returns
      -------
      res : hdrl.func.ResampleResult
        An object containing the results consisting of a cpl.core.PropertyList, representing the image or cube header, 
        and an hdrl.core.ImageList. These can be accessed via the hdr and imlist attributes of the object.

      Example
      -------
      .. code-block:: python

        # plist is a cpl.core.PropertyList containing the WCS keywords from the header
        wcs = cpl.drs.WCS(plist)
        # himg is a hdrl.core.Image to be resampled
        table = hdrl.func.Resample.image_to_table(himg, wcs)
        rmethod = hdrl.func.ResampleMethod.Lanczos(1,False,2)
        outgrid = hdrl.func.ResampleOutgrid.Auto2D(0.01,0.01)
        result = hdrl.func.Resample.compute(table,rmethod,outgrid,wcs)
        first_img = result.imlist[0].image
        first_err = result.imlist[0].error
        hdr = result.hdr

      )docstring")
      .def_static(
          "image_to_table",
          [](std::shared_ptr<hdrl::core::Image> hima,
             hdrl::core::pycpl_wcs wcs) {
            return hdrl::func::Resample::image_to_table(hima, wcs);
          },
          py::arg("hima"), py::arg("wcs"),
          R"docstring(
      Creates the `restable` input table needed by hdrl.func.Resample.compute() for 2D images.

      Parameters
      ----------
      himg : hdrl.core.Image
        The image to be resampled. 
      wcs : cpl.drs.WCS
        The World Coordinate System representative of the images to be resampled. 

      Returns
      -------
      restable : cpl.core.Table
        The table that is used by hdrl.func.Resample.compute()

      See Also
      --------
      hdrl.func.Resample.imagelist_to_table : Creates the `restable` input table needed by hdrl.func.Resample.compute() for 3D data cubes.
      hdrl.func.Resample.restable_template : Creates a table template as per Sect. 4.14.3 of the HDRL manual. 
      )docstring")
      .def_static(
          "imagelist_to_table",
          [](std::shared_ptr<hdrl::core::ImageList> himlist,
             hdrl::core::pycpl_wcs wcs) {
            return hdrl::func::Resample::imagelist_to_table(himlist, wcs);
          },
          py::arg("himlist"), py::arg("wcs"),
          R"docstring(
      Creates the `restable` input table needed by hdrl.func.Resample.compute() for 3D data cubes.

      Parameters
      ----------
      himlist : hdrl.core.ImageList
        The imagelist containing the images, i.e. the data cube, to be resampled. 
      wcs : cpl.drs.WCS
        The World Coordinate System representative of the images to be resampled. 

      Returns
      -------
      restable: cpl.core.Table
        The table that is used by hdrl.func.Resample.compute()

      See Also
      --------
      hdrl.func.Resample.image_to_table : Creates the `restable` input table needed by hdrl.func.Resample.compute() for 2D images.
      hdrl.func.Resample.restable_template : Creates a table template as per Sect. 4.14.3 of the HDRL manual. 
      )docstring")
      .def_static(
          "restable_template",
          [](int nrows) {
            return hdrl::func::Resample::restable_template(nrows);
          },
          py::arg("nrows"),
          R"docstring(
      Creates a table template as per Sect. 4.14.3 of the HDRL manual. 
      Useful for instances where the pixel to sky mapping is very complex 
      and can not be encoded in a cpl.drs.WCS object. This template can then 
      be filled by the pipeline developer as required. 

      Parameters
      ----------
      nrows : int 
        The number of rows to create.

      Returns
      -------
      restable : cpl.core.Table
        The table template with `nrows` rows. 

      See Also
      -------
      hdrl.func.Resample.image_to_table : Creates the `restable` input table needed by hdrl.func.Resample.compute() for 2D images.
      hdrl.func.Resample.imagelist_to_table : Creates the `restable` input table needed by hdrl.func.Resample.compute() for 3D data cubes.
      )docstring");

  // ResampleResult


  resampleresult_class.doc() = R"docstring(
      A hdrl.func.ResampleResult class is a container for the results of hdrl.func.Resample.compute().
      The results consist of a cpl.core.PropertyList, representing the image or cube header, 
      and an hdrl.core.ImageList. 

      These can be accessed via the hdr and imlist attributes of the object.

      Example
      -------
      .. code-block:: python

        result = hdrl.func.Resample.compute(table,rmethod,outgrid,wcs)
        hdr = result.hdr
        himlist = result.imlist
        first_img = himlist[0].image
        first_err = himlist[0].error
      )docstring";

  resampleresult_class
      .def_readonly("hdr", &hdrl::func::ResampleResult::hdr,
                    "cpl.core.PropertyList : image or cube header")
      .def_readonly(
          "imlist", &hdrl::func::ResampleResult::imlist,
          "hdrl.core.ImageList : imagelist containing the resampled images");

  // ResampleMethod

  resamplemethod_class.doc() = R"docstring(
      A hdrl.func.ResampleMethod class represents an interpolation algorithm 
      to be applied using hdrl.func.Resample.compute(). 

      The implemented interpolation algorithms are based on the MUSE pipeline and work for 2D images and 3D cubes. 
      The 2D and 3D interpolation is done in 2-dimensional and 3-dimensional spaces, respectively. 
      
      Currently there are six different interpolation methods implemented:
      - Nearest: Nearest neighbour resampling
      - Linear: Weighted resampling using an inverse distance weighting function
      - Quadratic: Weighted resampling using a quadratic inverse distance weighting function
      - Renka: Weighted resampling using a Renka weighting function
      - Drizzle: Weighted resampling using a drizzle-like weighting scheme
      - Lanczos: Weighted resampling using a Lanczos-like restricted sinc as weighting function

      Each method has its own separate constructor (e.g. hdrl.func.ResampleMethod.Nearest(), hdrl.func.ResampleMethod.Linear()).
      )docstring";

  resamplemethod_class
      // Nearest
      .def_static(
          "Nearest", []() { return new hdrl::func::ResampleMethod(); },
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      This method performs nearest neighbour resampling. 

      Returns
      -------
      hdrl.func.ResampleMethod
        Instance of hdrl.func.ResampleMethod for Nearest method

      Example
      -------
      .. code-block:: python

        rmethod = hdrl.func.ResampleMethod.Nearest()
      
      Notes
      -----
      The algorithm does not use any weighting function, but simply uses the value of the nearest neighbour inside an output voxel [1]_ centre as the final output value. If there is no nearest neighbour inside the voxel (but e.g. only outside), the voxel is marked as bad. This speeds up the algorithm considerably. There are no control parameters for this method.

      .. [1] In 3D computer graphics, a voxel represents a value on a regular grid in three-dimensional space. See `http://https://en.wikipedia.org/wiki/Voxel <http://https://en.wikipedia.org/wiki/Voxel>`_ for more information.

      See Also
      --------
      hdrl.func.ResampleMethod.Linear : Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      hdrl.func.ResampleMethod.Quadratic : Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      hdrl.func.ResampleMethod.Renka : Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      hdrl.func.ResampleMethod.Drizzle : Creates an instance of hdrl.func.ResampleMethod for the Drizzle method. 
      hdrl.func.ResampleMethod.Lanczos : Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.  
      )docstring")
      // Linear
      .def_static(
          "Linear",
          [](int loop_distance, bool use_errorweights) {
            return new hdrl::func::ResampleMethod("linear", loop_distance,
                                                  use_errorweights);
          },
          py::arg("loop_distance"), py::arg("use_errorweights"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      This method performs weighted resampling using an inverse distance weighting function.

      Parameters
      ----------
      loop_distance : int
        Controls the number of surrounding pixels that are taken into account on the final grid.
      use_errorweights : bool
        Apply an additional weight of 1/variance.

      Returns
      -------
      hdrl.func.ResampleMethod
       Instance of hdrl.func.ResampleMethod for the Linear method.

      Example
      -------
      .. code-block:: python

        loop_distance = 2
        use_errorweights = True
        rmethod = hdrl.func.ResampleMethod.Linear(loop_distance, use_errorweights)
      
      Notes
      -----
      The algorithm uses a linear inverse distance weighting function :math:`(1/r)` for the interpolation. 
      The parameter `loop_distance` controls the number of surrounding pixels that are taken into account on the final grid, 
      e.g. a `loop_distance` of 1 uses 3 pixels :math:`(x - 1, x, x + 1)` in each dimension, i.e. 9 in total for a 2D image 
      and 27 in total for a 3D cube. 
      Moreover, if the parameter `use_errorweights` is set to True, an additional weight, defined as 1/variance, is taken into account.
      This additional weight is only applied if the variance of a pixel is greater than 0.

      See Also
      --------
      hdrl.func.ResampleMethod.Nearest : Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      hdrl.func.ResampleMethod.Quadratic : Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      hdrl.func.ResampleMethod.Renka : Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      hdrl.func.ResampleMethod.Drizzle : Creates an instance of hdrl.func.ResampleMethod for the Drizzle method. 
      hdrl.func.ResampleMethod.Lanczos : Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.  
      )docstring")
      // Quadratic
      .def_static(
          "Quadratic",
          [](int loop_distance, bool use_errorweights) {
            return new hdrl::func::ResampleMethod("quadratic", loop_distance,
                                                  use_errorweights);
          },
          py::arg("loop_distance"), py::arg("use_errorweights"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      This method performs weighted resampling using a quadratic inverse distance weighting function. 

      Parameters
      ----------
      loop_distance : int
        Controls the number of surrounding pixels that are taken into account on the final grid.
      use_errorweights : bool
        Apply an additional weight of 1/variance.

      Returns
      -------
      hdrl.func.ResampleMethod
        Instance of hdrl.func.ResampleMethod for the Quadratic method.

      Example
      -------
      .. code-block:: python

        loop_distance = 2
        use_errorweights = True
        rmethod = hdrl.func.ResampleMethod.Quadratic(loop_distance, use_errorweights)
      
      Notes
      -----
      The algorithm uses a quadratic inverse distance weighting function :math:`(1/r^2)` for the interpolation. 
      The parameter `loop_distance` controls the number of surrounding pixels that are taken into account on the final grid, 
      e.g. a `loop_distance` of 1 uses 3 pixels :math:`(x - 1, x, x + 1)` in each dimension, i.e. 9 in total for a 2D image 
      and 27 in total for a 3D cube. 
      Moreover, if the parameter `use_errorweights` is set to True, an additional weight, defined as 1/variance, is taken into account.
      This additional weight is only applied if the variance of a pixel is greater than 0.

      See Also
      --------
      hdrl.func.ResampleMethod.Nearest : Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      hdrl.func.ResampleMethod.Linear : Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      hdrl.func.ResampleMethod.Renka : Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      hdrl.func.ResampleMethod.Drizzle : Creates an instance of hdrl.func.ResampleMethod for the Drizzle method. 
      hdrl.func.ResampleMethod.Lanczos : Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.  
      )docstring")
      // Renka
      .def_static(
          "Renka",
          [](int loop_distance, bool use_errorweights, double critical_radius) {
            return new hdrl::func::ResampleMethod(
                loop_distance, use_errorweights, critical_radius);
          },
          py::arg("loop_distance"), py::arg("use_errorweights"),
          py::arg("critical_radius"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      This method performs weighted resampling using a Renka weighting function.

      Parameters
      ----------
      loop_distance : int
        Controls the number of surrounding pixels that are taken into account on the final grid.
      use_errorweights : bool
        Apply an additional weight of 1/variance.
      critical_radius : float
        The distance beyond which the weights are set to 0.

      Returns
      -------
      hdrl.func.ResampleMethod
        Instance of hdrl.func.ResampleMethod for the Renka method.

      Example
      -------
      .. code-block:: python

        loop_distance = 2
        use_errorweights = True
        critical_radius = 3
        rmethod = hdrl.func.ResampleMethod.Renka(loop_distance, use_errorweights, critical_radius)
      
      Notes
      -----
      The algorithm uses a modified Shepard-like distance weighting function following Renka for the interpolation. 
      The parameter `critical_radius` defines the distance beyond which the weights are set to 0 and 
      the pixels are therefore not taken into account.
      The parameter `loop_distance` controls the number of surrounding pixels that are taken into account on the final grid, 
      e.g. a `loop_distance` of 1 uses 3 pixels :math:`(x - 1, x, x + 1)` in each dimension, i.e. 9 in total for a 2D image 
      and 27 in total for a 3D cube. 
      Moreover, if the parameter `use_errorweights` is set to True, an additional weight, defined as 1/variance, is taken into account.
      This additional weight is only applied if the variance of a pixel is greater than 0.

      See Also
      --------
      hdrl.func.ResampleMethod.Nearest : Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      hdrl.func.ResampleMethod.Linear : Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      hdrl.func.ResampleMethod.Quadratic : Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      hdrl.func.ResampleMethod.Drizzle : Creates an instance of hdrl.func.ResampleMethod for the Drizzle method. 
      hdrl.func.ResampleMethod.Lanczos : Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.  
      )docstring")
      // Drizzle
      .def_static(
          "Drizzle",
          [](int loop_distance, bool use_errorweights, double pix_frac_x,
             double pix_frac_y, double pix_frac_lambda) {
            return new hdrl::func::ResampleMethod(loop_distance,
                                                  use_errorweights, pix_frac_x,
                                                  pix_frac_y, pix_frac_lambda);
          },
          py::arg("loop_distance"), py::arg("use_errorweights"),
          py::arg("pix_frac_x"), py::arg("pix_frac_y"),
          py::arg("pix_frac_lambda"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Drizzle method.
      This method performs weighted resampling using a drizzle-like weighting scheme.

      Parameters
      ----------
      loop_distance : int
        Controls the number of surrounding pixels that are taken into account on the final grid.
      use_errorweights : bool
        Apply an additional weight of 1/variance.
      pix_frac_x : float
        Fraction of flux of the original pixel/voxel that drizzles into target pixel/voxel in the x-direction.
      pix_frac_y : float
        Fraction of flux of the original pixel/voxel that drizzles into target pixel/voxel in the y-direction.
      pix_frac_lambda : float
        Fraction of flux of the original pixel/voxel that drizzles into target pixel/voxel in the lambda-direction.

      Returns
      -------
      hdrl.func.ResampleMethod
        hdrl.func.ResampleMethod for the Drizzle method.

      Example
      -------
      .. code-block:: python

        loop_distance = 2
        use_errorweights = True
        pix_frac_drizzle_x = 0.8
        pix_frac_drizzle_y = 0.8
        pix_frac_drizzle_lambda = 1
        rmethod = hdrl.func.ResampleMethod.Drizzle(loop_distance, use_errorweights, pix_frac_drizzle_x, pix_frac_drizzle_y, pix_frac_drizzle_lambda)
      
      Notes
      -----
      The algorithm uses a drizzle-like distance weighting function for the interpolation. 
      The down-scaling factors `pix_frac_x`, `pix_frac_y`, and `pix_frac_lambda`, for x, y, and wavelength direction control the 
      percentage of flux of the original pixel/voxel that drizzles into the target pixel/voxel.
      The parameter `loop_distance` controls the number of surrounding pixels that are taken into account on the final grid, 
      e.g. a `loop_distance` of 1 uses 3 pixels :math:`(x - 1, x, x + 1)` in each dimension, i.e. 9 in total for a 2D image 
      and 27 in total for a 3D cube. 
      Moreover, if the parameter `use_errorweights` is set to True, an additional weight, defined as 1/variance, is taken into account.
      This additional weight is only applied if the variance of a pixel is greater than 0.

      See Also
      --------
      hdrl.func.ResampleMethod.Nearest : Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      hdrl.func.ResampleMethod.Linear : Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      hdrl.func.ResampleMethod.Quadratic : Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      hdrl.func.ResampleMethod.Renka : Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      hdrl.func.ResampleMethod.Lanczos : Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.  
      )docstring")
      // Lanczos
      .def_static(
          "Lanczos",
          [](int loop_distance, bool use_errorweights, int kernel_size) {
            return new hdrl::func::ResampleMethod(
                loop_distance, use_errorweights, kernel_size);
          },
          py::arg("loop_distance"), py::arg("use_errorweights"),
          py::arg("kernel_size"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleMethod for the Lanczos method.
      This method performs weighted resampling using a Lanczos-like restricted sinc as weighting function.

      Parameters
      ----------
      loop_distance : int
        Controls the number of surrounding pixels that are taken into account on the final grid.
      use_errorweights : bool
        Apply an additional weight of 1/variance.
      kernel_size : int
        The kernel size in pixel units for the sinc distance weighting function.

      Returns
      -------
      hdrl.func.ResampleMethod
        hdrl.func.ResampleMethod for the Lanczos method.

      Example
      -------
      .. code-block:: python

        loop_distance = 2
        use_errorweights = True
        kernel_size = 2
        rmethod = hdrl.func.ResampleMethod.Lanczos(loop_distance, use_errorweights, kernel_size)
      
      Notes
      -----
      The algorithm uses a restricted sinc distance weighting function sinc(r)/sinc(r/kernel_size), 
      with the kernel size given by the parameter `kernel_size` for the interpolation.
      The parameter `loop_distance` controls the number of surrounding pixels that are taken into account on the final grid, 
      e.g. a `loop_distance` of 1 uses 3 pixels :math:`(x - 1, x, x + 1)` in each dimension, i.e. 9 in total for a 2D image 
      and 27 in total for a 3D cube. 
      Moreover, if the parameter `use_errorweights` is set to True, an additional weight, defined as 1/variance, is taken into account.
      This additional weight is only applied if the variance of a pixel is greater than 0.
      
      See Also
      --------
      hdrl.func.ResampleMethod.Nearest : Creates an instance of hdrl.func.ResampleMethod for the Nearest method.
      hdrl.func.ResampleMethod.Linear : Creates an instance of hdrl.func.ResampleMethod for the Linear method.
      hdrl.func.ResampleMethod.Quadratic : Creates an instance of hdrl.func.ResampleMethod for the Quadratic method.
      hdrl.func.ResampleMethod.Renka : Creates an instance of hdrl.func.ResampleMethod for the Renka method.
      hdrl.func.ResampleMethod.Drizzle : Creates an instance of hdrl.func.ResampleMethod for the Drizzle method. 
      )docstring");

  // ResampleOutgrid

  resampleoutgrid_class.doc() = R"docstring(
      A hdrl.func.ResampleOutgrid class defines the basic properties of the resampled image or cube that are to
      be considered by hdrl.func.Resample.compute(). 

      It can be instantiated via one of the following constructors:
      - hdrl.func.ResampleOutgrid.User2D : User specified function for 2D images.
      - hdrl.func.ResampleOutgrid.User3D : User specified function for 3D data cubes.
      - hdrl.func.ResampleOutgrid.Auto2D : Convenience function for 2D images.
      - hdrl.func.ResampleOutgrid.Auto3D : Convenience function for 3D data cubes.
      In the case of the Auto2D and Auto3D constructors, only the step sizes in 
      right ascension, declination and wavelength of the output image or cube are required. 
      All the rest are automatically derived from the data by hdrl.func.Resample.compute().
      )docstring";

  resampleoutgrid_class
      // User2D
      .def_static(
          "User2D",
          [](double delta_ra, double delta_dec, double ra_min, double ra_max,
             double dec_min, double dec_max, double fieldmargin) {
            return new hdrl::func::ResampleOutgrid(delta_ra, delta_dec, ra_min,
                                                   ra_max, dec_min, dec_max,
                                                   fieldmargin);
          },
          py::arg("delta_ra"), py::arg("delta_dec"), py::arg("ra_min"),
          py::arg("ra_max"), py::arg("dec_min"), py::arg("dec_max"),
          py::arg("fieldmargin"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleOutgrid for 2D images.

      Parameters
      ----------
      delta_ra : float
        Output grid step in right ascension
      delta_dec : float
        Output grid step in declination
      ra_min : float
        Minimum boundary of the image in right ascension
      ra_max : float
        Maximum boundary of the image in right ascension
      dec_min : float
        Minimum boundary of the image in declination
      dec_max : float
        Maximum boundary of the image in declination
      fieldmargin : float
        Percentage of how much margin to add to the output image in all spatial directions.
        A value of 0 adds no margin. 

      Returns
      -------
      hdrl.func.ResampleOutgrid
        instance of hdrl.func.ResampleOutgrid for 2D images.

      Example
      -------
      .. code-block:: python

        delta_ra = 0.1
        delta_dec = 0.2
        ra_min = 48.069416667
        ra_max = 48.0718125
        dec_min = -20.6229925
        dec_max = -20.620708611
        field_margin = 5
        outputgrid = hdrl.func.ResampleOutgrid.User2D(delta_ra, delta_dec, ra_min, ra_max, dec_min, dec_max, field_margin)

      See Also
      --------
      hdrl.func.ResampleOutgrid.Auto2D : Creates an instance of hdrl.func.ResampleOutgrid for 2D images. 
      )docstring")
      // User3D
      .def_static(
          "User3D",
          [](double delta_ra, double delta_dec, double delta_lambda,
             double ra_min, double ra_max, double dec_min, double dec_max,
             double lambda_min, double lambda_max, double fieldmargin) {
            return new hdrl::func::ResampleOutgrid(
                delta_ra, delta_dec, delta_lambda, ra_min, ra_max, dec_min,
                dec_max, lambda_min, lambda_max, fieldmargin);
          },
          py::arg("delta_ra"), py::arg("delta_dec"), py::arg("delta_lambda"),
          py::arg("ra_min"), py::arg("ra_max"), py::arg("dec_min"),
          py::arg("dec_max"), py::arg("lambda_min"), py::arg("lambda_max"),
          py::arg("fieldmargin"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleOutgrid for 3D data cubes.

      Parameters
      ----------
      delta_ra : float
        Output grid step in right ascension
      delta_dec : float
        Output grid step in declination
      delta_lambda: float
        Output grid step in wavelength
      ra_min : float
        Minimum boundary of the image in right ascension
      ra_max : float
        Maximum boundary of the image in right ascension
      dec_min : float
        Minimum boundary of the image in declination
      dec_max : float
        Maximum boundary of the image in declination
      lambda_min : float
        Minimum boundary of the image in wavelength
      lambda_max : float
        Maximum boundary of the image in wavelength 
      fieldmargin : float
        Percentage of how much margin to add to the output image in all spatial directions.
        A value of 0 adds no margin. 

      Returns
      -------
      hdrl.func.ResampleOutgrid
        instance of hdrl.func.ResampleOutgrid for 3D data cubes.

      Example
      -------
      .. code-block:: python

        delta_ra = 0.1
        delta_dec = 0.2
        delta_lambda = 0.001
        ra_min = 48.069416667
        ra_max = 48.0718125
        dec_min = -20.6229925
        dec_max = -20.620708611
        lambda_min = 1.9283e-06
        lambda_max = 2.47146e-06
        field_margin = 5
        outputgrid = hdrl.func.ResampleOutgrid.User3D(delta_ra, delta_dec, delta_lambda, ra_min, ra_max, dec_min, dec_max, lambda_min, lambda_max, field_margin)

      See Also
      --------
      hdrl.func.ResampleOutgrid.Auto3D : Creates an instance of hdrl.func.ResampleOutgrid for 3D data cubes.
      )docstring")
      // Auto2D
      .def_static(
          "Auto2D",
          [](double delta_ra, double delta_dec) {
            return new hdrl::func::ResampleOutgrid(delta_ra, delta_dec);
          },
          py::arg("delta_ra"), py::arg("delta_dec"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleOutgrid for 2D images.

      Parameters
      ----------
      delta_ra : float
        Output grid step in right ascension
      delta_dec : float
        Output grid step in declination

      Returns
      -------
      hdrl.func.ResampleOutgrid
        Instance of hdrl.func.ResampleOutgrid for 2D images.

      Example
      -------
      .. code-block:: python

        outputgrid = hdrl.func.ResampleOutgrid.Auto2D(0.1,0.2)

      See Also
      --------
      hdrl.func.ResampleOutgrid.User2D : Creates an instance of hdrl.func.ResampleOutgrid for 2D images. 
      )docstring")
      // Auto3D
      .def_static(
          "Auto3D",
          [](double delta_ra, double delta_dec, double delta_lambda) {
            return new hdrl::func::ResampleOutgrid(delta_ra, delta_dec,
                                                   delta_lambda);
          },
          py::arg("delta_ra"), py::arg("delta_dec"), py::arg("delta_lambda"),
          R"docstring(
      Creates an instance of hdrl.func.ResampleOutgrid for 3D data cubes.

      Parameters
      ----------
      delta_ra : float
        Output grid step in right ascension
      delta_dec : float
        Output grid step in declination
      delta_lambda: float
        Output grid step in wavelength

      Returns
      -------
      hdrl.func.ResampleOutgrid
        Instance of hdrl.func.ResampleOutgrid for 3D data cubes.

      Example
      -------
      .. code-block:: python

        outputgrid = hdrl.func.ResampleOutgrid.Auto3D(0.1,0.2,0.001)

      See Also
      --------
      hdrl.func.ResampleOutgrid.User3D : Creates an instance of hdrl.func.ResampleOutgrid for 3D data cubes.
      )docstring");
}
