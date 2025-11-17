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

#include "cplcore/types.hpp"

#include <complex>

namespace cpl
{
namespace core
{
std::complex<float>
complexf_to_cpp(float _Complex value)
{
  /*
  According to the C specification, _Complex numbers are required to be
  stored as an array of [2], where first element is real, second is imaginary
  */
  float(&z)[2] = reinterpret_cast<float(&)[2]>(value);
  return std::complex<float>(z[0], z[1]);
}

std::complex<double>
complexd_to_cpp(double _Complex value)
{
  /*
  According to the C specification, _Complex numbers are required to be
  stored as an array of [2], where first element is real, second is imaginary
  */
  double(&z)[2] = reinterpret_cast<double(&)[2]>(value);
  return std::complex<double>(z[0], z[1]);
}

float _Complex complex_to_c(std::complex<float> value)
{
  float _Complex z;
  /*
  According to the C specification, _Complex numbers are required to be
  stored as an array of [2], where first element is real, second is imaginary
  */
  reinterpret_cast<float(&)[2]>(z)[0] = value.real();
  reinterpret_cast<float(&)[2]>(z)[1] = value.imag();
  return z;
}

double _Complex complex_to_c(std::complex<double> value)
{
  double _Complex z;
  /*
  According to the C specification, _Complex numbers are required to be
  stored as an array of [2], where first element is real, second is imaginary
  */
  reinterpret_cast<double(&)[2]>(z)[0] = value.real();
  reinterpret_cast<double(&)[2]>(z)[1] = value.imag();
  return z;
}

}  // namespace core
}  // namespace cpl