# This file is part of the PyHDRL Python language bindings
# Copyright (C) 2023-2026 European Southern Observatory
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import math

import numpy as np
import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestBPM:
    # Tests fo BPM-2D
    def test_2d_getters(self):
        # filtersmooth
        fs = hdrlfunc.BPM2D.Filter(4.0, 5.0, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, 9)
        # legendresmooth
        ls = hdrlfunc.BPM2D.Legendre(4, 5, 6, 20, 21, 11, 12, 2, 10)

        assert fs.kappa_low == pytest.approx(4.0)
        assert fs.kappa_high == pytest.approx(5.0)
        assert fs.maxiter == 6
        assert fs.border == cplcore.Border.NOP
        assert fs.filter == cplcore.Filter.MEDIAN
        assert fs.smooth_x == 7
        assert fs.smooth_y == 9
        assert fs.method == hdrlfunc.BPM2D.Method.Filter

        assert ls.kappa_low == pytest.approx(4.0)
        assert ls.kappa_high == pytest.approx(5.0)
        assert ls.maxiter == 6
        assert ls.steps_x == 20
        assert ls.steps_y == 21
        assert ls.filter_size_x == 11
        assert ls.filter_size_y == 12
        assert ls.order_x == 2
        assert ls.order_y == 10
        assert ls.method == hdrlfunc.BPM2D.Method.Legendre

    def test_2d_input(self):
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(-1.0, 5, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, -1.0, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, -1, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, 6, cplcore.Filter.STDEV, cplcore.Border.NOP, 7, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, -1, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, -1)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 0, 9)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Filter(4, 5, 6, cplcore.Filter.MEDIAN, cplcore.Border.NOP, 7, 0)

        # legendresmooth
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Legendre(-1.0, 5, 6, 20, 21, 11, 12, 2, 10)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Legendre(4, -1.0, 6, 20, 21, 11, 12, 2, 10)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Legendre(4, 5, -1, 20, 21, 11, 12, 2, 10)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Legendre(4, 5, 6, 20, 21, 11, 12, -1, 10)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM2D.Legendre(4, 5, 6, 20, 21, 11, 12, 2, -1)

    def test_2d_compute(self):
        # Create BPM parameters
        fs = hdrlfunc.BPM2D.Filter(3.0, 3.0, 2, cplcore.Filter.MEDIAN, cplcore.Border.FILTER, 3, 3)
        # Test sigmaclipped mean gauss mean 100 sigma 3.5 and 2 outliers
        values = [
            92,
            93,
            94,
            94,
            95,
            95,
            96,
            96,
            96,
            97,
            97,
            97,
            97,
            98,
            98,
            98,
            98,
            99,
            99,
            99,
            99,
            100,
            100,
            100,
            100,
            100,
            101,
            101,
            101,
            101,
            102,
            102,
            102,
            102,
            103,
            103,
            103,
            103,
            104,
            104,
            104,
            105,
            105,
            106,
            106,
            107,
            108,
            500,
            600,
        ]
        data = cplcore.Image(values, width=7, dtype=cplcore.Type.DOUBLE)
        errors = cplcore.Image.zeros(7, 7, cplcore.Type.DOUBLE)
        errors.add_scalar(1)

        errors[6][6] = 100000.0
        errors[6][5] = 10000.0

        sigimage = hdrlcore.Image(data, errors)

        with pytest.raises(hdrlcore.InvalidTypeError):
            mask_out = fs.compute(None)

        # check sigimg mask values

        ls = hdrlfunc.BPM2D.Legendre(3.0, 3.0, 2, 20, 20, 11, 11, 3, 3)
        mask_out = ls.compute(sigimage)

        data_bpm = cplcore.Mask(200, 300)
        data = cplcore.Image.create_noise_uniform(200, 300, cplcore.Type.FLOAT, 90, 110)
        data[49][49] = 300.0
        data[99][99] = 300.0
        data[149][149] = 300.0
        data[259][109] = 300.0

        data_bpm[119][119] = True
        data_bpm[120][119] = True
        data_bpm[121][119] = True
        data_bpm[119][120] = True
        data_bpm[120][120] = True
        data_bpm[121][120] = True
        data_bpm[119][121] = True
        data_bpm[120][121] = True
        data_bpm[121][121] = True
        # set one outlier on a bad pixel
        data[121][121] = 300.0

        data.reject_from_mask(data_bpm)
        error = data.duplicate()
        error.power(0.5)

        image = hdrlcore.Image(data, error)

        fs = hdrlfunc.BPM2D.Filter(3.0, 3.0, 5, cplcore.Filter.MEDIAN, cplcore.Border.FILTER, 3, 3)
        mask_out = fs.compute(image)
        assert mask_out[49][49]  # True
        assert mask_out[99][99]  # True
        assert mask_out[149][149]  # True
        assert mask_out[259][109]  # True
        assert not mask_out[121][121]  # False

        ls = hdrlfunc.BPM2D.Legendre(3.0, 3.0, 5, 20, 20, 11, 11, 3, 3)
        mask_out = ls.compute(image)
        assert mask_out[49][49]  # True
        assert mask_out[99][99]  # True
        assert mask_out[149][149]  # True
        assert mask_out[259][109]  # True
        assert not mask_out[121][121]  # False

    # BPM 3D tests
    def test_3d_getters(self):
        bpm_3d = hdrlfunc.BPM3D(4, 5, hdrlfunc.BPM3D.Method.Absolute)
        assert bpm_3d.kappa_low == pytest.approx(4.0)
        assert bpm_3d.kappa_high == pytest.approx(5.0)
        assert bpm_3d.method == hdrlfunc.BPM3D.Method.Absolute

        bpm_3d = hdrlfunc.BPM3D(4, 5, hdrlfunc.BPM3D.Method.Relative)
        assert bpm_3d.kappa_low == pytest.approx(4.0)
        assert bpm_3d.kappa_high == pytest.approx(5.0)
        assert bpm_3d.method == hdrlfunc.BPM3D.Method.Relative

        bpm_3d = hdrlfunc.BPM3D(4, 5, hdrlfunc.BPM3D.Method.Error)
        assert bpm_3d.kappa_low == pytest.approx(4.0)
        assert bpm_3d.kappa_high == pytest.approx(5.0)
        assert bpm_3d.method == hdrlfunc.BPM3D.Method.Error

    def test_3d_input(self):
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM3D(5.1, 5.0, hdrlfunc.BPM3D.Method.Absolute)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM3D(-5.0, 5.0, hdrlfunc.BPM3D.Method.Relative)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM3D(5.0, -5.0, hdrlfunc.BPM3D.Method.Error)

    def test_bpm_3d_compute(self):
        imlist = hdrlcore.ImageList()
        for var in range(5):
            data_bpm = cplcore.Mask(200, 300)
            data = cplcore.Image.create_noise_uniform(200, 300, cplcore.Type.DOUBLE, 82, 118)
            if var == 0:
                # Negative outlier set and marked as bad
                data[9][9] = 20.0
                data_bpm[9][9] = True
                # Positive outlier set and marked as bad
                data[49][49] = 300.0
                data_bpm[49][49] = True

                # Positive outlier set
                data[59][59] = 300.0
                data[60][60] = 300.0
                data[61][61] = 300.0
                # Negative outliers
                data[69][69] = 20.0
                data[70][70] = 20.0
                data[71][71] = 20.0
                # some pixels marked as bad
                data_bpm[79][79] = True
                data_bpm[79][80] = True
                data_bpm[79][81] = True
            elif var == 3:
                data[149][149] = 300.0
                data[259][109] = 300.0
                data_bpm[69][69] = True
                data_bpm[79][79] = True

            data.reject_from_mask(data_bpm)
            error = data.duplicate()
            error.power(0.5)
            image = hdrlcore.Image(data, error)
            imlist.append(image)

        param3d = hdrlfunc.BPM3D(-50.0, 50.0, hdrlfunc.BPM3D.Method.Absolute)
        result_data = param3d.compute(imlist)
        img_tmp = result_data[0]

        assert not img_tmp[9][9]  # False
        assert not img_tmp[49][49]  # False
        assert img_tmp[59][59]  # True
        assert img_tmp[60][60]  # True
        assert img_tmp[61][61]  # True
        assert img_tmp[69][69]  # True
        assert img_tmp[70][70]  # True
        assert img_tmp[71][71]  # True
        assert not img_tmp[79][79]  # False
        assert not img_tmp[79][80]  # False
        assert not img_tmp[79][81]  # False

        param3d = hdrlfunc.BPM3D(5.0, 5.0, hdrlfunc.BPM3D.Method.Relative)
        result_data = param3d.compute(imlist)
        img_tmp = result_data[0]

        assert not img_tmp[9][9]  # False
        assert not img_tmp[49][49]  # False
        assert img_tmp[59][59]  # True
        assert img_tmp[60][60]  # True
        assert img_tmp[61][61]  # True
        assert img_tmp[69][69]  # True
        assert img_tmp[70][70]  # True
        assert img_tmp[71][71]  # True
        assert not img_tmp[79][79]  # False
        assert not img_tmp[79][80]  # False
        assert not img_tmp[79][81]  # False

        param3d = hdrlfunc.BPM3D(5.0, 5.0, hdrlfunc.BPM3D.Method.Error)
        result_data = param3d.compute(imlist)
        img_tmp = result_data[0]

        assert not img_tmp[9][9]  # False
        assert not img_tmp[49][49]  # False
        assert img_tmp[59][59]  # True
        assert img_tmp[60][60]  # True
        assert img_tmp[61][61]  # True
        assert img_tmp[69][69]  # True
        assert img_tmp[70][70]  # True
        assert img_tmp[71][71]  # True
        assert not img_tmp[79][79]  # False
        assert not img_tmp[79][80]  # False
        assert not img_tmp[79][81]  # False

        param3 = hdrlfunc.BPM3D(500.0, 5.0, hdrlfunc.BPM3D.Method.Error)
        result_data = param3.compute(imlist)
        img_tmpp = result_data[0]

        assert not img_tmpp[9][9]  # False
        assert not img_tmpp[49][49]  # False
        assert img_tmpp[59][59]  # True
        assert img_tmpp[60][60]  # True
        assert img_tmpp[61][61]  # True
        assert not img_tmpp[69][69]  # False
        assert not img_tmpp[70][70]  # False
        assert not img_tmpp[71][71]  # False
        assert not img_tmpp[79][79]  # False
        assert not img_tmpp[79][80]  # False
        assert not img_tmpp[79][81]  # False

        param3 = hdrlfunc.BPM3D(500.0, 5.0, hdrlfunc.BPM3D.Method.Error)
        with pytest.raises(hdrlcore.InvalidTypeError):
            result_data = param3.compute(None)

    # Tests for BPM Fit
    def test_fit_getters(self):
        fit1 = hdrlfunc.BPMFit.PVal(1, 10.0)
        assert fit1.degree == 1
        assert fit1.pval == pytest.approx(10.0)
        fit2 = hdrlfunc.BPMFit.PVal(22, 5.5)
        assert fit2.degree == 22
        assert fit2.pval == pytest.approx(5.5)
        fit3 = hdrlfunc.BPMFit.RelCoef(1, 0.5, 0.4)
        assert fit3.coef_low == pytest.approx(0.5)
        assert fit3.coef_high == pytest.approx(0.4)
        fit4 = hdrlfunc.BPMFit.RelChi(1, 0.5, 0.4)
        assert fit4.chi_low == pytest.approx(0.5)
        assert fit4.chi_high == pytest.approx(0.4)

    def test_fit_inputs(self):
        # invalid degree
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.PVal(-1, 0.1)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.RelChi(-1, 1.0, 1.0)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.RelCoef(-1, 1.0, 1.0)
        # invalid threshold
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.PVal(1, -0.1)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.PVal(1, 100.1)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.RelChi(1, -1.0, 1.0)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPMFit.RelCoef(1, -1.0, -1.0)

    def test_bpm_fit_compute(self):
        hl = hdrlcore.ImageList()
        sample = cplcore.Vector.zeros(10)
        for i in range(10):
            himg = hdrlcore.Image.zeros(13, 4)
            himg.add_scalar((i + 1, np.sqrt(i + 1)))
            himg.set_pixel(0, 0, (1.01, 1.0))
            hl.append(himg)
            sample[i] = i

        p = hdrlfunc.BPMFit.PVal(1, 0.1)
        out_mask = p.compute(hl, sample)
        assert out_mask.get_flux() == pytest.approx(0.0, abs=1e-12)

        p = hdrlfunc.BPMFit.RelCoef(1, 1.0, 1.0)
        out_mask = p.compute(hl, sample)
        assert out_mask[0][0] == 3

        p = hdrlfunc.BPMFit.RelChi(1, 1.0, 1.0)
        hl[4].add_scalar((5.1, math.sqrt(5.1)))
        out_mask = p.compute(hl, sample)
        assert out_mask.get_flux() == pytest.approx(1.0)
        assert out_mask[0][0] == pytest.approx(1.0)

    def test_filter(self):
        img_mask = cplcore.Mask(200, 300)
        img_mask[49][49] = True
        img_mask[99][99] = True
        img_mask[149][149] = True
        img_mask[249][99] = True
        img_mask[251][99] = True
        img_mask[253][99] = True
        img_mask[255][99] = True
        img_mask[251][101] = True
        img_mask[253][101] = True
        img_mask[255][101] = True
        img_mask[251][197] = True
        img_mask[253][197] = True
        img_mask[255][197] = True
        img_mask[251][199] = True
        img_mask[253][199] = True
        img_mask[255][199] = True
        img_mask[299][198] = True
        img_mask[298][198] = True
        img_mask[297][198] = True
        img_mask[299][199] = True
        img_mask[298][199] = True
        img_mask[297][199] = True

        # test border behaviour
        img_mask[199][198] = True
        img_mask[197][198] = True

        filtered_mask = hdrlfunc.BPM.filter(img_mask, 3, 3, cplcore.Filter.CLOSING)
        assert filtered_mask[254][99]  # True
        assert filtered_mask[254][100]  # True
        assert filtered_mask[254][101]  # True
        assert not filtered_mask[254][102]  # False

        assert filtered_mask[250][99]  # True

        assert filtered_mask[254][197]  # True
        assert filtered_mask[254][198]  # True
        assert filtered_mask[254][199]  # True

        assert filtered_mask[253][197]  # True
        assert filtered_mask[253][198]  # True
        assert filtered_mask[253][199]  # True

        # test border behaviour
        assert not filtered_mask[198][199]  # False

    def test_filter_list(self):
        nx = 5
        ny = 5
        img1 = cplcore.Image.zeros(ny, nx, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(ny, nx, cplcore.Type.INT)
        img2.add_scalar(1)
        list = cplcore.ImageList([img1, img2])

        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.BPM.filter_list(list, 3, 3, cplcore.Filter.MEDIAN)
        with pytest.raises(hdrlcore.NullInputError):
            hdrlfunc.BPM.filter_list(None, 3, 3, cplcore.Filter.MEDIAN)

        # rest of the test in hdrl_utils_test.c not related to filter_test
        # Wrapper around hdrl_bpm_filter() to filter list of images
        hdrlfunc.BPM.filter_list(list, 3, 3, cplcore.Filter.CLOSING)
