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

#include "cpldrs/photom_bindings.hpp"

#include "cpldrs/photom.hpp"

namespace py = pybind11;

void
bind_photom(py::module& m)
{
  py::module photom = m.def_submodule(
      "photom", "High-level functions that are photometry related");
  // cpl_unit enum is only seemingly used for cpl_photom so it should be fine to
  // put it here
  py::enum_<cpl_unit> unit(photom, "Unit");

  unit.value("PHOTONRADIANCE", CPL_UNIT_PHOTONRADIANCE)
      .value("ENERGYRADIANCE", CPL_UNIT_ENERGYRADIANCE)
      .value("LESS", CPL_UNIT_LESS)
      .value("LENGTH", CPL_UNIT_LENGTH)
      .value("FREQUENCY", CPL_UNIT_FREQUENCY);

  // Photom only has one function
  photom.def("fill_blackbody", &cpl::drs::photom::fill_blackbody,
             py::arg("out_unit"), py::arg("evalpoints"), py::arg("in_unit"),
             py::arg("temp"),
             R"pydoc(
    The Planck radiance from a black-body

    Parameters
    ----------
    out_unit: cpl.drs.photom.Unit
        cpl.drs.photom.Unit.PHOTONRADIANCE, cpl.drs.photom.Unit.ENERGYRADIANCE or cpl.drs.photom.Unit.LESS
    evalpoints: cpl.core.Vector
        The evaluation points (wavelengths or frequencies)
    in_unit: cpl.drs.photom.Unit
        cpl.drs.photom.Unit.LENGTH or cpl.drs.photom.Unit.FREQUENCY
    temp: float
        The black body temperature [K]

    Return
    ------
    cpl.core.Vector
        The computed radiance

    Raises
    ------
    cpl.core.IncompatibleInputError
        if the size of evalpoints is different from the size of spectrum
    cpl.core.UnsupportedModeError
        if in_unit and out_unit are not as requested
    cpl.core.IllegalInputError
        if temp or a wavelength is non-positive

    Notes
    -----
    The Planck black-body radiance can be computed in 5 different ways:
    As a radiance of either energy [J*radian/s/m^3] or photons [radian/s/m^3],
    and in terms of either wavelength [m] or frequency [1/s]. The fifth way is
    as a unit-less radiance in terms of wavelength, in which case the area under
    the planck curve is 1.
    The dimension of the spectrum (energy or photons or unit-less, cpl.drs.photom.Unit.LESS)
    is controlled by out_unit, and the dimension of the input (length or
    frequency) is controlled by in_unit.

    evalpoints and spectrum must be of equal, positive length.

    The input wavelengths/frequencies and the temperature must be positive.

    The four different radiance formulas are:
    
    .. math::
        Rph1(\lambda,T) = 2 \pi \frac{c}{\lambda^4} (\exp(hc/kT\lambda)-1)^{-1}

    .. math::
        Rph2(\nu,T) = 2 \pi \frac{\nu^2}{c^4} (\exp(h\nu/kT)-1)^{-1}

    .. math::
        Re1(\lambda,T) = 2 \pi \frac{hc^2}{\lambda^5} (\exp(hc/kT\lambda)-1)^{-1} =
        \frac{hc}{\lambda} Rph1(\lambda,T)

    .. math::
        Re2(\nu,T) = 2 \pi \frac{h\nu^3}{c^2} (\exp(h\nu/kT)-1)^{-1} = h\nu Rph2(\nu,T)

    .. math::
        R1(\lambda,T) = \frac{15h^5c^5}{\pi^4k^5\lambda^5T^5}
        (\exp(hc/kT\lambda)-1)^{-1} = \frac{h^4c^3}{2\pi^5k^5T^5} Rph1(\lambda,T)

    where :math:`\lambda` is the wavelength, :math:`\nu` is the frequency,
    :math:`T` is the temperature, h is the Planck constant, k is the Boltzmann
    constant and c is the speed of light in vacuum.

    When the radiance is computed in terms of wavelength, the radiance peaks
    at :math:`\lambda_{max} = 2.897771955\times 10^{-3}/T` [m]. When the radiance
    is unit-less this maximum, :math:`R1(\lambda_{max},T)`, is approximately 3.2648.
    :math:`R1(\lambda,T)` integrated over l from 0 to infinity is 1.
)pydoc");
}
