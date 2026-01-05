// This file is part of the PyHDRL Python language bindings
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

#include "hdrlfunc/lacosmic.hpp"

#include <cpl_error.h>
#include <cpl_mask.h>
#include <hdrl_lacosmics.h>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{
using hdrl::core::Error;
using hdrl::core::pycpl_mask;

LaCosmic::LaCosmic(double sigma_lim, double f_lim, int max_iter)
{
  m_interface = hdrl::core::Error::throw_errors_with(
      hdrl_lacosmic_parameter_create, sigma_lim, f_lim, max_iter);
  // verify that the parameters make sense
  cpl_error_code err =
      Error::throw_errors_with(hdrl_lacosmic_parameter_verify, m_interface);
}

pycpl_mask
LaCosmic::edgedetect(std::shared_ptr<hdrl::core::Image> img_in)
{
  cpl_mask* mask = Error::throw_errors_with(hdrl_lacosmic_edgedetect,
                                            img_in.get()->ptr(), m_interface);
  return pycpl_mask(mask);
}

double
LaCosmic::get_sigma_lim()
{
  return Error::throw_errors_with(hdrl_lacosmic_parameter_get_sigma_lim,
                                  m_interface);
}

double
LaCosmic::get_f_lim()
{
  return Error::throw_errors_with(hdrl_lacosmic_parameter_get_f_lim,
                                  m_interface);
}

int
LaCosmic::get_max_iter()
{
  return Error::throw_errors_with(hdrl_lacosmic_parameter_get_max_iter,
                                  m_interface);
}

hdrl_parameter*
LaCosmic::ptr()
{
  return m_interface;
}

}  // namespace func
}  // namespace hdrl
