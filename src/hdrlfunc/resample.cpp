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

#include "hdrlfunc/resample.hpp"

#include <cpl_table.h>
#include <cpl_type.h>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Error;
using hdrl::core::IllegalInputError;

Resample::Resample() {}

hdrl_parameter*
Resample::ptr()
{
  return m_interface;
}

ResampleResult
Resample::compute(pycpl_table restable, ResampleMethod method,
                  ResampleOutgrid outputgrid, pycpl_wcs wcs)
{
  hdrl_resample_result* res = Error::throw_errors_with(
      hdrl_resample_compute, restable.t, method.ptr(), outputgrid.ptr(), wcs.w);
  return ResampleResult(res);
}

// restable related functions
pycpl_table
Resample::image_to_table(std::shared_ptr<Image> hima, pycpl_wcs wcs)
{
  cpl_table* restable = Error::throw_errors_with(hdrl_resample_image_to_table,
                                                 hima.get()->ptr(), wcs.w);
  return pycpl_table(restable);
}

pycpl_table
Resample::imagelist_to_table(std::shared_ptr<ImageList> himlist, pycpl_wcs wcs)
{
  cpl_table* restable = Error::throw_errors_with(
      hdrl_resample_imagelist_to_table, himlist.get()->ptr(), wcs.w);
  return pycpl_table(restable);
}

pycpl_table
Resample::restable_template(int nrows)
{
  cpl_table* tab = cpl_table_new(nrows);
  /* create the table columns */
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_RA,
                       HDRL_RESAMPLE_TABLE_RA_TYPE);
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_DEC,
                       HDRL_RESAMPLE_TABLE_DEC_TYPE);
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_LAMBDA,
                       HDRL_RESAMPLE_TABLE_LAMBDA_TYPE);
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_DATA,
                       HDRL_RESAMPLE_TABLE_DATA_TYPE);
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_BPM,
                       HDRL_RESAMPLE_TABLE_BPM_TYPE);
  cpl_table_new_column(tab, HDRL_RESAMPLE_TABLE_ERRORS,
                       HDRL_RESAMPLE_TABLE_ERRORS_TYPE);
  /* init column values */
  cpl_table_fill_column_window_double(tab, HDRL_RESAMPLE_TABLE_RA, 0, nrows,
                                      0.);
  cpl_table_fill_column_window_double(tab, HDRL_RESAMPLE_TABLE_DEC, 0, nrows,
                                      0.);
  cpl_table_fill_column_window_double(tab, HDRL_RESAMPLE_TABLE_LAMBDA, 0, nrows,
                                      0.);
  cpl_table_fill_column_window_double(tab, HDRL_RESAMPLE_TABLE_DATA, 0, nrows,
                                      0.);
  cpl_table_fill_column_window_int(tab, HDRL_RESAMPLE_TABLE_BPM, 0, nrows, 0);
  cpl_table_fill_column_window_double(tab, HDRL_RESAMPLE_TABLE_ERRORS, 0, nrows,
                                      0.);
  return pycpl_table(tab);
}

// ResampleMethod

// Nearest
ResampleMethod::ResampleMethod()
{
  m_interface =
      Error::throw_errors_with(hdrl_resample_parameter_create_nearest);
}

// Linear or Quadratic
ResampleMethod::ResampleMethod(std::string mode, int loop_distance,
                               bool use_errorweights)
{
  if (mode == "linear") {
    m_interface = Error::throw_errors_with(
        hdrl_resample_parameter_create_linear, loop_distance,
        static_cast<cpl_boolean>(use_errorweights));
  } else if (mode == "quadratic") {
    m_interface = Error::throw_errors_with(
        hdrl_resample_parameter_create_quadratic, loop_distance,
        static_cast<cpl_boolean>(use_errorweights));
  } else {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Invalid mode type for hdrl.func.ResampleMethod. "
                            "Expected linear or quadratic.");
  }
}

// Renka
ResampleMethod::ResampleMethod(int loop_distance, bool use_errorweights,
                               double critical_radius)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_renka, loop_distance,
      static_cast<cpl_boolean>(use_errorweights), critical_radius);
}

// Drizzle
ResampleMethod::ResampleMethod(int loop_distance, bool use_errorweights,
                               double pix_frac_x, double pix_frac_y,
                               double pix_frac_lambda)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_drizzle, loop_distance,
      static_cast<cpl_boolean>(use_errorweights), pix_frac_x, pix_frac_y,
      pix_frac_lambda);
}

// Lanczos
ResampleMethod::ResampleMethod(int loop_distance, bool use_errorweights,
                               int kernel_size)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_lanczos, loop_distance,
      static_cast<cpl_boolean>(use_errorweights), kernel_size);
}

// ResampleOutgrid

// userdef 2D
ResampleOutgrid::ResampleOutgrid(double delta_ra, double delta_dec,
                                 double ra_min, double ra_max, double dec_min,
                                 double dec_max, double fieldmargin)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_outgrid2D_userdef, delta_ra, delta_dec,
      ra_min, ra_max, dec_min, dec_max, fieldmargin);
}

// userdef 3D
ResampleOutgrid::ResampleOutgrid(double delta_ra, double delta_dec,
                                 double delta_lambda, double ra_min,
                                 double ra_max, double dec_min, double dec_max,
                                 double lambda_min, double lambda_max,
                                 double fieldmargin)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_outgrid3D_userdef, delta_ra, delta_dec,
      delta_lambda, ra_min, ra_max, dec_min, dec_max, lambda_min, lambda_max,
      fieldmargin);
}

// helper 2D
ResampleOutgrid::ResampleOutgrid(double delta_ra, double delta_dec)
{
  m_interface = Error::throw_errors_with(
      hdrl_resample_parameter_create_outgrid2D, delta_ra, delta_dec);
}

// helper 3D
ResampleOutgrid::ResampleOutgrid(double delta_ra, double delta_dec,
                                 double delta_lambda)
{
  m_interface =
      Error::throw_errors_with(hdrl_resample_parameter_create_outgrid3D,
                               delta_ra, delta_dec, delta_lambda);
}

ResampleResult::ResampleResult()
{
  // Default constructors handle nullptr initialization
}

ResampleResult::ResampleResult(hdrl_resample_result* res)
{
  // Create wrapped objects from C pointers (like catalogue module)
  if (res->header) {
    hdr = hdrl::core::pycpl_propertylist(res->header);
  }

  if (res->himlist) {
    imlist = std::make_shared<ImageList>(res->himlist);
  }
}

}  // namespace func
}  // namespace hdrl
