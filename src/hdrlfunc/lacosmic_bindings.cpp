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

#include "hdrlfunc/lacosmic_bindings.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_mask.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/lacosmic.hpp"


namespace py = pybind11;

void
bind_lacosmic(py::module& m)
{
  py::class_<hdrl::func::LaCosmic, std::shared_ptr<hdrl::func::LaCosmic>>
      lacosmic_class(m, "LaCosmic", py::buffer_protocol());

  lacosmic_class.doc() = R"docstring(
        The hdrl.func.LaCosmic class provides an interface to the LA-Cosmic algorithm 
        described in van Dokkum et al. 2001, PASP, 113, 1420 to detect bad-pixels
        and cosmic-rays hits on a single image.
        
        After creating an instance of the class using hdrl.func.LaCosmic() with the desired
        parameters, the detection is executed using the member function edgedetect().

        Parameters
        ----------
        sigma_lim : float
            Limiting sigma for detection on the sampling image.
        f_lim : float
            Limiting f factor for detection on the modified Laplacian image.
        max_iter : int
            Maximum number of iterations.

        Returns
        -------
        hdrl.func.LaCosmic

        See Also
        --------
        hdrl.func.LaCosmic.edgedetect : Detect bad-pixels or cosmic-rays on a single image.

        Notes
        -----
        For the algorithm see the paper of `van Dokkum et al. 2001, PASP, 113, 1420 <https://ui.adsabs.harvard.edu/abs/2001PASP..113.1420V/abstract>`_.

      )docstring";

  lacosmic_class
      .def(py::init<double, double, int>(), py::arg("sigma_lim"),
           py::arg("f_lim"), py::arg("max_iter"))
      .def_property_readonly(
          "sigma_lim", &hdrl::func::LaCosmic::get_sigma_lim,
          "float : Limiting sigma for detection on the sampling image.")
      .def_property_readonly("f_lim", &hdrl::func::LaCosmic::get_f_lim,
                             "float : Limiting f factor for detection on the "
                             "modified Laplacian image.")
      .def_property_readonly("max_iter", &hdrl::func::LaCosmic::get_max_iter,
                             "int : Maximum number of iterations.")
      .def(
          "edgedetect",
          [](hdrl::func::LaCosmic& self,
             std::shared_ptr<hdrl::core::Image> img_in)
              -> hdrl::core::pycpl_mask { return self.edgedetect(img_in); },
          py::arg("img_in"), R"docstring(
        Detect bad-pixels or cosmic-rays on a single image.
        
        This routine determines bad-pixels on a single image via edge
        detection following the LA-Cosmic algorithm described in van Dokkum
        et al. 2001, PASP, 113, 1420. It was originally developed to detect cosmic
        ray hits, but it can also be used in the more general context to detect
        bad pixels. The HDRL implementation does not use the error model as
        described in the paper, but instead uses the error image passed to the
        function. Moreover, several iterations are performed until no new 
        bad pixels are found or the number of iterations reaches max_iter.
        In each iteration the detected cosmic ray hits are replaced by the median 
        of the surrounding 5x5 pixels taking into account the pixel quality
        information. The input parameters `sigma_lim` and `f_lim` refer to
        :math:`\sigma_{lim}` and :math:`f_{lim}` as described in the paper
        mentioned above.

        Parameters
        ----------
        ima_in : hdrl.core.Image
          The input image

        Returns
        -------
        cpl.core.Mask 
            A mask with all detected bad pixels or cosmics-rays marked as bad or None on error

        See Also
        --------
        hdrl.func.LaCosmic : Creates an LaCosmic instance.

        Notes
        -----
        Be aware that the implementation only detects positive bad
        pixels or cosmic ray hits, i.e. no "holes" in the image are detected,
        but in such a case the pixels surrounding the hole are marked as
        bad. Holes in the image can be introduced if e.g. one subtracts a not
        cosmic-ray-cleaned image from another image.
      )docstring");
}
