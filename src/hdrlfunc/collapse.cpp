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

#include "hdrlfunc/collapse.hpp"

#include <memory>
#include <string>

#include <cpl_image_io.h>
#include <cpl_type.h>
#include <hdrl_collapse.h>
#include <hdrl_image.h>
#include <hdrl_imagelist_basic.h>
#include <hdrl_parameter.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/image.hpp"
#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{
using hdrl::core::Error;
using hdrl::core::Image;
using hdrl::core::InvalidTypeError;
using hdrl::core::pycpl_image;

Collapse::Collapse(std::string m)
{
  // do we need a constructor here?
  mode = m;
  if (mode == "mean") {
    m_interface = Error::throw_errors_with(hdrl_collapse_mean_parameter_create);
  } else if (mode == "wmean") {
    m_interface =
        Error::throw_errors_with(hdrl_collapse_weighted_mean_parameter_create);
  } else if (mode == "median") {
    m_interface =
        Error::throw_errors_with(hdrl_collapse_median_parameter_create);
  }
}

// MinMax
Collapse::Collapse(double nlow, double nhigh)
{
  mode = "minmax";
  m_interface = Error::throw_errors_with(hdrl_collapse_minmax_parameter_create,
                                         nlow, nhigh);
}

// Mode
Collapse::Collapse(double histo_min, double histo_max, double bin_size,
                   hdrl_mode_type mode_method, cpl_size error_niter)
{
  mode = "mode";
  m_interface =
      Error::throw_errors_with(hdrl_collapse_mode_parameter_create, histo_min,
                               histo_max, bin_size, mode_method, error_niter);
}

// Sigclip
Collapse::Collapse(double kappa_low, double kappa_high, int niter)
{
  mode = "sigclip";
  m_interface = Error::throw_errors_with(hdrl_collapse_sigclip_parameter_create,
                                         kappa_low, kappa_high, niter);
}

hdrl_parameter*
Collapse::ptr()
{
  return m_interface;
}

std::string
Collapse::get_mode()
{
  return mode;
}

py::object
Collapse::compute(std::shared_ptr<ImageList> himlist)
{
  hdrl_image* out;
  cpl_image* contrib;
  hdrl_parameter* params;

  Error::throw_errors_with(hdrl_imagelist_collapse, himlist.get()->ptr(),
                           m_interface, &out, &contrib);
  py::module_ col = py::module_::import("collections");
  py::object ntup = col.attr("namedtuple")("CollapseResult", "out contrib");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib));
  return result;
}

/*py::object
Collapse::collapse(ImageList himlist, collapse_type collapse){

    hdrl_image* out;
    cpl_image* contrib;
    hdrl_parameter* params;
    if (CollapseMean* c = std::get_if<CollapseMean>(&collapse)){ params =
c->ptr(); } else if (CollapseMedian* c =
std::get_if<CollapseMedian>(&collapse)){ params = c->ptr(); } else if
(CollapseWeightedMean* c = std::get_if<CollapseWeightedMean>(&collapse)){ params
= c->ptr(); } else if (CollapseSigclip* c =
std::get_if<CollapseSigclip>(&collapse)){ params = c->ptr(); } else if
(CollapseMinMax* c = std::get_if<CollapseMinMax>(&collapse)){ params = c->ptr();
} else if (CollapseMode* c = std::get_if<CollapseMode>(&collapse)){ params =
c->ptr(); } else { throw std::logic_error("collapse was given unaccounted-for
variant");
    }

    Error::throw_errors_with(hdrl_imagelist_collapse,
himlist.ptr(),params,&out,&contrib); py::module_ col =
py::module_::import("collections"); py::object ntup =
col.attr("namedtuple")("CollapseResult","out contrib"); py::object result =
ntup(Image(out),pycpl_image(contrib)); return result;
}*/

py::object
Collapse::collapse_mean(std::shared_ptr<ImageList> himlist)
{
  hdrl_image* out;
  cpl_image* contrib;
  Error::throw_errors_with(hdrl_imagelist_collapse_mean, himlist.get()->ptr(),
                           &out, &contrib);
  py::module_ col = py::module_::import("collections");
  py::object ntup = col.attr("namedtuple")("CollapseMeanResult", "out contrib");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib));
  return result;
}

py::object
Collapse::collapse_weighted_mean(std::shared_ptr<ImageList> himlist)
{
  hdrl_image* out;
  cpl_image* contrib;
  Error::throw_errors_with(hdrl_imagelist_collapse_weighted_mean,
                           himlist.get()->ptr(), &out, &contrib);
  py::module_ col = py::module_::import("collections");
  py::object ntup =
      col.attr("namedtuple")("CollapseWeightedMeanResult", "out contrib");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib));
  return result;
}

py::object
Collapse::collapse_median(std::shared_ptr<ImageList> himlist)
{
  hdrl_image* out;
  cpl_image* contrib;
  Error::throw_errors_with(hdrl_imagelist_collapse_median, himlist.get()->ptr(),
                           &out, &contrib);
  py::module_ col = py::module_::import("collections");
  py::object ntup =
      col.attr("namedtuple")("CollapseMedianResult", "out contrib");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib));
  return result;
}

py::object
Collapse::collapse_sigclip(std::shared_ptr<ImageList> himlist, double kappa_low,
                           double kappa_high, int niter)
{
  hdrl_image* out;
  cpl_image* contrib;
  cpl_image* reject_low;
  cpl_image* reject_high;
  Error::throw_errors_with(hdrl_imagelist_collapse_sigclip,
                           himlist.get()->ptr(), kappa_low, kappa_high, niter,
                           &out, &contrib, &reject_low, &reject_high);
  py::module_ col = py::module_::import("collections");
  py::object ntup = col.attr("namedtuple")(
      "CollapseSigclipResult", "out contrib reject_low reject_high");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib),
           pycpl_image(reject_low), pycpl_image(reject_high));
  return result;
}

py::object
Collapse::collapse_minmax(std::shared_ptr<ImageList> himlist, double nlow,
                          double nhigh)
{
  hdrl_image* out;
  cpl_image* contrib;
  cpl_image* reject_low;
  cpl_image* reject_high;
  Error::throw_errors_with(hdrl_imagelist_collapse_minmax, himlist.get()->ptr(),
                           nlow, nhigh, &out, &contrib, &reject_low,
                           &reject_high);
  py::module_ col = py::module_::import("collections");
  py::object ntup = col.attr("namedtuple")(
      "CollapseMinMaxResult", "out contrib reject_low reject_high");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib),
           pycpl_image(reject_low), pycpl_image(reject_high));
  return result;
}

py::object
Collapse::collapse_mode(std::shared_ptr<ImageList> himlist, double histo_min,
                        double histo_max, double bin_size,
                        hdrl_mode_type mode_method, cpl_size error_niter)
{
  hdrl_image* out;
  cpl_image* contrib;
  Error::throw_errors_with(hdrl_imagelist_collapse_mode, himlist.get()->ptr(),
                           histo_min, histo_max, bin_size, mode_method,
                           error_niter, &out, &contrib);
  py::module_ col = py::module_::import("collections");
  py::object ntup = col.attr("namedtuple")("CollapseModeResult", "out contrib");
  py::object result =
      ntup(std::make_shared<Image>(Image(out)), pycpl_image(contrib));
  return result;
}

double
Collapse::get_nhigh()
{
  if (mode == "minmax") {
    return Error::throw_errors_with(hdrl_collapse_minmax_parameter_get_nhigh,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "nhigh attribute only defined for hdrl.func.Collapse.MinMax");
  }
}

double
Collapse::get_nlow()
{
  if (mode == "minmax") {
    return Error::throw_errors_with(hdrl_collapse_minmax_parameter_get_nlow,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "nlow attribute only defined for hdrl.func.Collapse.MinMax");
  }
}

double
Collapse::get_histo_min()
{
  if (mode == "mode") {
    return Error::throw_errors_with(hdrl_collapse_mode_parameter_get_histo_min,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "histo_min attribute only defined for hdrl.func.Collapse.Mode");
  }
}

double
Collapse::get_histo_max()
{
  if (mode == "mode") {
    return Error::throw_errors_with(hdrl_collapse_mode_parameter_get_histo_max,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "histo_max attribute only defined for hdrl.func.Collapse.Mode");
  }
}

double
Collapse::get_bin_size()
{
  if (mode == "mode") {
    return Error::throw_errors_with(hdrl_collapse_mode_parameter_get_bin_size,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "bin_size attribute only defined for hdrl.func.Collapse.Mode");
  }
}

hdrl_mode_type
Collapse::get_method()
{
  if (mode == "mode") {
    return Error::throw_errors_with(hdrl_collapse_mode_parameter_get_method,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "method attribute only defined for hdrl.func.Collapse.Mode");
  }
}

cpl_size
Collapse::get_error_niter()
{
  if (mode == "mode") {
    return Error::throw_errors_with(
        hdrl_collapse_mode_parameter_get_error_niter, m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "error_niter attribute only defined for hdrl.func.Collapse.Mode");
  }
}

double
Collapse::get_kappa_high()
{
  if (mode == "sigclip") {
    return Error::throw_errors_with(
        hdrl_collapse_sigclip_parameter_get_kappa_high, m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "kappa_high attribute only defined for hdrl.func.Collapse.Sigclip");
  }
}

double
Collapse::get_kappa_low()
{
  if (mode == "sigclip") {
    return Error::throw_errors_with(
        hdrl_collapse_sigclip_parameter_get_kappa_low, m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "kappa_low attribute only defined for hdrl.func.Collapse.Sigclip");
  }
}

int
Collapse::get_niter()
{
  if (mode == "sigclip") {
    return Error::throw_errors_with(hdrl_collapse_sigclip_parameter_get_niter,
                                    m_interface);
  } else {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "niter attribute only defined for hdrl.func.Collapse.Sigclip");
  }
}

}  // namespace func
}  // namespace hdrl
