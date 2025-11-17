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

#ifndef PYCPL_PHOTOM_HPP
#define PYCPL_PHOTOM_HPP

#include <cpl_photom.h>

#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{
namespace photom
{

/**
 * @brief The Planck radiance from a black-body
 * @param out_unit CPL_UNIT_PHOTONRADIANCE, CPL_UNIT_ENERGYRADIANCE or
 *     CPL_UNIT_LESS
 * @param evalpoints The evaluation points (wavelengths or frequencies)
 * @param in_unit CPL_UNIT_LENGTH or CPL_UNIT_FREQUENCY
 * @param temp The black body temperature [K]
 *
 * The Planck black-body radiance can be computed in 5 different ways:
 * As a radiance of either energy [J*radian/s/m^3] or photons [radian/s/m^3],
 * and in terms of either wavelength [m] or frequency [1/s]. The fifth way is
 * as a unit-less radiance in terms of wavelength, in which case the area under
 * the planck curve is 1.
 *
 * The dimension of the spectrum (energy or photons or unit-less, CPL_UNIT_LESS)
 * is controlled by out_unit, and the dimension of the input (length or
 * frequency) is controlled by in_unit.
 *
 * evalpoints and spectrum must be of equal, positive length.
 *
 * The input wavelengths/frequencies and the temperature must be positive.
 *
 * The four different radiance formulas are:
 * Rph1(l,T) = 2pi c/l^4/(exp(hc/klT)-1)
 * Rph2(f,T) = 2pi f^2/c^2/(exp(hf/kT)-1)
 * Re1(l,T) = 2pi hc^2/l^5/(exp(hc/klT)-1) = Rph1(l,T) * hc/l
 * Re2(f,T) = 2pi hf^3/c^2/(exp(hf/kT)-1)  = Rph2(f,T) * hf
 * R1(l,T)  = 15h^5c^5/(pi^4k^5l^5T^5/(exp(hc/klT)-1)
 * = Rph1(l,T) * h^4c^3/(2pi^5k^5T^5)
 *
 * where l is the wavelength, f is the frequency, T is the temperature,
 * h is the Planck constant, k is the Boltzmann constant and
 * c is the speed of light in vacuum.
 *
 * When the radiance is computed in terms of wavelength, the radiance peaks
 * at l_max = CPL_PHYS_Wien/temp. When the radiance is unit-less this maximum,
 * R1(l_max,T), is approximately 3.2648. R1(l,T) integrated over l from 0 to
 * infinity is 1.
 *
 * A unit-less black-body radiance in terms of frequency may be added later,
 * until then it is an error to combine CPL_UNIT_LESS and CPL_UNIT_FREQUENCY.
 *
 * @return the computed radiance
 * @throws IncompatibleInputError if the size of evalpoints is different from
 * the size of spectrum
 * @throws UnsupportedModeError if in_unit and out_unit are not as requested
 * @throws IllegalInputError if temp or a wavelength is non-positive
 */
cpl::core::Vector
fill_blackbody(cpl_unit out_unit, const cpl::core::Vector& evalpoints,
               cpl_unit in_unit, double temp);

}  // namespace photom
}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_PHOTOM_HPP