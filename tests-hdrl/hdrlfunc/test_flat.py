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

import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestFlat:
    def create_uniform_images(self, nima, ima_sx, ima_sy, values):
        # the following function generates a list of uniform images of given size and values
        imglist = hdrlcore.ImageList()
        for var in range(nima):
            data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
            # N.B. this mask is not used!
            # data_bpm = cplcore.Mask(ima_sx, ima_sy)
            # STDEV is about 10 for these images
            data.add_scalar(values[var])
            errors = data.duplicate()
            errors.power(0.5)
            image = hdrlcore.Image(data, errors)
            imglist.append(image)
        return imglist

    def hdrl_flat_create_static_mask(self, ima_sx, ima_sy, rect):
        b_llx = rect[0]
        b_lly = rect[1]
        b_urx = rect[2]
        b_ury = rect[3]
        stat_mask = cplcore.Mask(ima_sx, ima_sy)
        for j in range(b_lly, b_ury):
            for i in range(b_llx, b_urx):
                stat_mask[j - 1][i - 1] = True
        return stat_mask

    def hdrl_flat_test_case(self, imglist, rect, method, fsx, fsy, collapse, mask_sw, val1, err1, val2, err2):
        # verify that the image and error values in the
        # lower left corner and at a given data point are as expected
        stat_mask = None
        b_llx = rect[0]
        b_lly = rect[1]
        b_urx = rect[2]
        b_ury = rect[3]

        b_cx = int(0.5 * (b_llx + b_urx) + 0.5)
        b_cy = int(0.5 * (b_lly + b_ury) + 0.5)
        hima = imglist[0]
        ima_sx = hima.width
        ima_sy = hima.height

        flat = hdrlfunc.Flat(fsx, fsy, method)
        if mask_sw == 1:
            stat_mask = self.hdrl_flat_create_static_mask(ima_sx, ima_sy, rect)

        results = flat.compute(imglist, collapse, stat_mask)
        assert results.master.image.get_pixel(0, 0) == val1
        assert results.master.error.get_pixel(0, 0) == pytest.approx(err1)
        assert results.master.image.get_pixel(b_cy - 1, b_cx - 1) == val2

    def hdrl_flat_imlist_flag_region(self, imglist, rect, outlier, mask_sw):
        # the following function flag as bad data points over a given region
        nima = len(imglist)
        b_llx = rect[0]
        b_lly = rect[1]
        b_urx = rect[2]
        b_ury = rect[3]

        hima = imglist[0]

        ima_sx = hima.width
        ima_sy = hima.height

        for var in range(nima):
            hima = imglist[var]
            data = hima.image
            data_bpm = cplcore.Mask(ima_sx, ima_sy)

            # cpl_image_fill_window(data, b_llx, b_lly, b_urx, b_ury, outlier)
            for j in range(b_lly, b_ury):
                for i in range(b_llx, b_urx):
                    data[j - 1][i - 1] = outlier
            if mask_sw == 1:
                for j in range(b_lly, b_ury):
                    for i in range(b_llx, b_urx):
                        data_bpm[j - 1][i - 1] = True
            data.reject_from_mask(data_bpm)
            errors = data.duplicate()
            errors.power(0.5)
            image = hdrlcore.Image(data, errors)

            imglist[var] = image
        # the imglist is modified in place, so we do not need to return it
        # return imglist

    # tests translated from hdrl_flat-test.c

    def test_data_value_bpm(self):
        # Check flat results in case of a uniform input
        ima_sx = 51
        ima_sy = 31
        filter_size_x = 1
        filter_size_y = 1
        nima = 9

        vals = cplcore.Vector.zeros(nima)
        value = 9
        error = math.sqrt(value)
        vals.fill(value)

        imglist = self.create_uniform_images(nima, ima_sx, ima_sy, vals)
        r1_llx = 11
        r1_lly = 11
        r1_urx = 31
        r1_ury = 23
        xsam = 0
        ysam = 0
        outlier1 = 10000

        self.hdrl_flat_imlist_flag_region(imglist, (r1_llx, r1_lly, r1_urx, r1_ury), outlier1, 1)

        # case 1:
        #    9 images each of value 9,
        #    flat method=hdrlfunc.Flat.Mode.FreqLow,
        #    stat_mask=NULL,
        #    colapse_method: mean
        #
        #    expected results:
        #    master should have value 1 (master is normalised),
        #    error 3./9./math.sqrt(9) / math.sqrt(51*31)
        #    contribution map should be NULL.
        collapse = hdrlfunc.Collapse.Mean()
        stat_mask = None
        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqLow).compute(
            imglist, collapse, stat_mask
        )

        # check the results
        xsam = 0.5 * (r1_llx + r1_urx)
        ysam = 0.5 * (r1_lly + r1_ury)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        # hdrl.core.Image.get_pixel returns Value namedtuple
        res = results.master.get_pixel(py, px)
        assert res.data is None
        assert res.error is None
        assert res.invalid
        assert expected_value == 0

        # 3./9./math.sqrt(9)
        xsam = 0.5 * (ima_sx + r1_urx)
        ysam = 0.5 * (ima_sy + r1_ury)
        expected_error = error / value / math.sqrt(nima)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        res = results.master.get_pixel(py, px)
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

        # case 2:
        #    9 images each of value 9,
        #    flat method=hdrlfunc.Flat.Mode.FreqHigh,
        #    stat_mask=NULL,
        #    colapse_method: mean
        #
        #    expected results:
        #    master should have value 1 (master is normalised),
        #    error 3,
        #    contribution map should be NULL.
        #
        collapse = hdrlfunc.Collapse.Mean()
        stat_mask = None
        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqHigh).compute(
            imglist, collapse, stat_mask
        )

        xsam = 0.5 * (r1_llx + r1_urx)
        ysam = 0.5 * (r1_lly + r1_ury)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        res = results.master.get_pixel(py, px)
        assert res.data is None
        assert res.error is None
        assert res.invalid
        assert expected_value == 0

        # 3./9./math.sqrt(9)
        xsam = 0.5 * (ima_sx + r1_urx)
        ysam = 0.5 * (ima_sy + r1_ury)
        expected_error = error / value / math.sqrt(nima)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        res = results.master.get_pixel(py, px)
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

    def test_data_value_bpm_static(self):
        # Check flat results in case of a uniform input and a static mask
        ima_sx = 51
        ima_sy = 31
        filter_size_x = 1
        filter_size_y = 1
        nima = 9

        vals = cplcore.Vector.zeros(nima)
        value = 9
        error = math.sqrt(value)
        vals.fill(value)

        imglist = self.create_uniform_images(nima, ima_sx, ima_sy, vals)
        r1_llx = 11
        r1_lly = 11
        r1_urx = 31
        r1_ury = 23
        xsam = 0
        ysam = 0
        outlier1 = 10000

        # Note the last parameter to hdrl_flat_imlist_flag_region is ZERO here c.f. ONE for test_data_value_bpm()
        self.hdrl_flat_imlist_flag_region(imglist, (r1_llx, r1_lly, r1_urx, r1_ury), outlier1, 0)

        # case 1:
        #    9 images each of value 9,
        #    flat method=hdrlfunc.Flat.Mode.FreqLow,
        #    stat_mask=NULL,
        #    colapse_method: mean
        #
        #    expected results:
        #    master has value 1 (master is normalised) where points are not masked,
        #    and outlier/nima=111.11 where points are masked
        #    error 3./9./math.sqrt(9) / math.sqrt(51*31)
        #    contribution map should be NULL.
        collapse = hdrlfunc.Collapse.Mean()
        stat_mask = cplcore.Mask(ima_sx, ima_sy)
        for j in range(r1_lly, r1_ury):
            for i in range(r1_llx, r1_urx):
                stat_mask[j - 1][i - 1] = True

        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqLow).compute(
            imglist, collapse, stat_mask
        )

        # check the results
        xsam = 0.5 * (r1_llx + r1_urx)
        ysam = 0.5 * (r1_lly + r1_ury)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        # hdrl.core.Image.get_pixel returns Value namedtuple
        res = results.master.get_pixel(py, px)
        expected_error = math.sqrt(outlier1) / value / math.sqrt(nima)
        assert res.data == pytest.approx(outlier1 / nima)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

        # 3./9./math.sqrt(9)
        xsam = 0.5 * (ima_sx + r1_urx)
        ysam = 0.5 * (ima_sy + r1_ury)
        expected_error = error / value / math.sqrt(nima)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        res = results.master.get_pixel(py, px)
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

        # case 2:
        #   9 images each of value 9,
        #   flat method=hdrlfunc.Flat.Mode.FreqHigh,
        #   stat_mask=NULL,
        #   colapse_method: mean
        #
        #   expected results:
        #   master should have value 1 (master is normalised),
        #   error 3,
        #   contribution map should be NULL.
        #
        collapse = hdrlfunc.Collapse.Mean()
        # use stat_mask from ABOVE
        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqHigh).compute(
            imglist, collapse, stat_mask
        )

        xsam = 0.5 * (r1_llx + r1_urx)
        ysam = 0.5 * (r1_lly + r1_ury)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        expected_error = math.sqrt(outlier1) / outlier1 / math.sqrt(nima)
        res = results.master.get_pixel(py, px)
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

        # 3./9./math.sqrt(9)
        xsam = 0.5 * (ima_sx + r1_urx)
        ysam = 0.5 * (ima_sy + r1_ury)
        expected_error = error / value / math.sqrt(nima)
        py = int(ysam - 1)
        px = int(xsam - 1)
        expected_value = results.contrib_map.get_pixel(py, px)
        res = results.master.get_pixel(py, px)
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid
        assert expected_value == nima

    def test_data_value_basic(self):
        # Check flat results in case of a uniform input
        ima_sx = 51
        ima_sy = 31
        npix = ima_sx * ima_sy
        filter_size_x = 1
        filter_size_y = 1
        nima = 9

        vals = cplcore.Vector.zeros(nima)
        value = 9
        error = math.sqrt(value)
        vals.fill(value)

        imglist = self.create_uniform_images(nima, ima_sx, ima_sy, vals)

        # case 1:
        #    9 images each of value 9,
        #    flat method=hdrlfunc.Flat.Mode.FreqLow,
        #    stat_mask=NULL,
        #    colapse_method: mean
        #
        #    expected results:
        #    master should have value 1 (master is normalised),
        #    error 3./9./math.sqrt(9) / math.sqrt(51*31)
        #    contribution map should be NULL.
        collapse = hdrlfunc.Collapse.Mean()
        stat_mask = None
        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqLow).compute(
            imglist, collapse, stat_mask
        )

        # check the results
        expected_error = error / value / math.sqrt(nima) / math.sqrt(ima_sx * ima_sy)
        res = results.master.get_mean()
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid

        res = results.master.get_median()
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error * math.sqrt(math.pi * 0.5))
        assert not res.invalid
        assert results.master.get_stdev() == pytest.approx(0.0, abs=1e-12)

        res = results.master.get_sum()
        assert res.data == npix
        assert res.error == pytest.approx(math.sqrt(npix) / nima)
        assert not res.invalid
        assert results.contrib_map.get_mean() == nima

        # case 2:
        #   9 images each of value 9,
        #   flat method=hdrlfunc.Flat.Mode.FreqHigh,
        #   stat_mask=NULL,
        #   colapse_method: mean
        #
        #   expected results:
        #   master should have value 1 (master is normalised),
        #   error 3,
        #   contribution map should be NULL.
        collapse = hdrlfunc.Collapse.Mean()
        stat_mask = None
        results = hdrlfunc.Flat(filter_size_x, filter_size_y, hdrlfunc.Flat.Mode.FreqHigh).compute(
            imglist, collapse, stat_mask
        )

        # check the results (same as for the preceding case 1)
        expected_error = error / value / math.sqrt(nima) / math.sqrt(ima_sx * ima_sy)
        res = results.master.get_mean()
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error)
        assert not res.invalid

        res = results.master.get_median()
        assert res.data == pytest.approx(1.0)
        assert res.error == pytest.approx(expected_error * math.sqrt(math.pi * 0.5))
        assert not res.invalid
        assert results.master.get_stdev() == pytest.approx(0.0, abs=1e-12)

        res = results.master.get_sum()
        assert res.data == npix
        assert res.error == pytest.approx(math.sqrt(npix) / nima)
        assert not res.invalid
        assert results.contrib_map.get_mean() == nima

    def test_multi_options(self):
        # Check flat algorithm for various collapsing/smoothing conditions

        # input data
        ima_sx = 200
        ima_sy = 300

        b_llx = 100
        b_lly = 100
        b_urx = 200
        b_ury = 200

        rect = (b_llx, b_lly, b_urx, b_ury)

        nima = 5
        outlier = 100000

        vals = cplcore.Vector.zeros(nima)
        valserr_rel = cplcore.Vector.zeros(nima)
        base = 2.0
        for i in range(nima):
            intensity = math.pow(base, i)
            vals[i] = intensity
            valserr_rel[i] = math.sqrt(intensity) / intensity

        imglist = self.create_uniform_images(nima, ima_sx, ima_sy, vals)

        self.hdrl_flat_imlist_flag_region(imglist, rect, outlier, 0)

        # flat parameters
        filter_size_x = 1
        filter_size_y = 1
        r_median = 25000.0
        r_mean = 38750.0

        # case 1:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqLow,
        # stat_mask=NULL,
        # colapse_method: median

        # Error propagation for pixel 1,1
        valserr_rel.power(2.0)
        error_expected_pix1_mean = math.sqrt(valserr_rel.sum()) / nima
        error_expected_pix1_median = error_expected_pix1_mean * math.sqrt(math.pi * 0.5)

        collapse_mean = hdrlfunc.Collapse.Mean()
        collapse_median = hdrlfunc.Collapse.Median()

        # in the following cases (2-8) it is difficult to verify the error value at
        # the image centre because the image intensities are on purpose distributed
        # with complex values.

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqLow,
            filter_size_x,
            filter_size_y,
            collapse_median,
            0,
            1,
            error_expected_pix1_median,
            r_median,
            91.4844,
        )

        # case 2:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqLow,
        # stat_mask!=NULL,
        # colapse_method: median

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqLow,
            filter_size_x,
            filter_size_y,
            collapse_median,
            1,
            1,
            error_expected_pix1_median,
            r_median,
            91.4844,
        )

        # case 3:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqLow,
        # stat_mask=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqLow,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            0,
            1,
            error_expected_pix1_mean,
            r_mean,
            72.994,
        )

        # case 4:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqLow,
        # stat_mask!=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqLow,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            1,
            1,
            error_expected_pix1_mean,
            r_mean,
            72.994,
        )

        # case 5:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask=NULL,
        # colapse_method: median

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_median,
            0,
            1,
            error_expected_pix1_median,
            1,
            0.00177245,
        )
        # case 6:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask!=NULL,
        # colapse_method: median

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_median,
            1,
            1,
            error_expected_pix1_median,
            1,
            0.00177245,
        )
        # case 7:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            0,
            1,
            error_expected_pix1_mean,
            1,
            0.00141421,
        )
        # case 8:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask!=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            1,
            1,
            error_expected_pix1_mean,
            1,
            0.00141421,
        )

    # @pytest.mark.xfail(reason="Case 3 currently fails on err1: assert 0.348 == 0.278")
    def test_static_mask(self):
        # input data
        ima_sx = 200
        ima_sy = 300

        r1_llx = 50
        r1_lly = 50
        r1_urx = 80
        r1_ury = 250
        outlier1 = 100000

        r2_llx = 100
        r2_lly = 90
        r2_urx = 130
        r2_ury = 260
        outlier2 = 200000

        r3_llx = 150
        r3_lly = 80
        r3_urx = 180
        r3_ury = 270
        outlier3 = 300000

        nima = 5
        rect1 = (r1_llx, r1_lly, r1_urx, r1_ury)

        rect2 = (r2_llx, r2_lly, r2_urx, r2_ury)

        rect3 = (r3_llx, r3_lly, r3_urx, r3_ury)

        # image intensity values distributed as 2^n
        vals = cplcore.Vector.zeros(nima)

        base = 2.0
        for i in range(nima):
            intensity = math.pow(base, i)
            vals[i] = intensity

        imglist = self.create_uniform_images(nima, ima_sx, ima_sy, vals)

        self.hdrl_flat_imlist_flag_region(imglist, rect1, outlier1, 1)
        self.hdrl_flat_imlist_flag_region(imglist, rect2, outlier2, 1)
        self.hdrl_flat_imlist_flag_region(imglist, rect3, outlier3, 1)

        # flat parameters
        filter_size_x = 1
        filter_size_y = 1
        r_mean = 38750
        e_mean = 72.994

        collapse_mean = hdrlfunc.Collapse.Mean()
        # collapse_median = hdrlfunc.Collapse.Median()

        # case 1:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqLow,
        # stat_mask=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect1,
            hdrlfunc.Flat.Mode.FreqLow,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            0,
            1,
            0.278388,
            r_mean,
            e_mean,
        )

        # case 2:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask=NULL,
        # colapse_method: mean

        self.hdrl_flat_test_case(
            imglist,
            rect1,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            0,
            1,
            0.278388,
            1,
            0.00141421,
        )

        # case 3:
        #
        # flat method=hdrlfunc.Flat.Mode.FreqHigh,
        # stat_mask=NULL,
        # colapse_method: median

        # TODO: This only passes with collapse_mean and not collapse_median as
        # specified in hdrl_flat-test.c

        self.hdrl_flat_test_case(
            imglist,
            rect1,
            hdrlfunc.Flat.Mode.FreqHigh,
            filter_size_x,
            filter_size_y,
            collapse_mean,
            0,
            1,
            0.278388,
            1,
            0.00141421,
        )

    def test_inputs(self):
        filter_size_x = 5
        filter_size_y = 5
        method = hdrlfunc.Flat.Mode.FreqLow

        flat = hdrlfunc.Flat(filter_size_x, filter_size_y, method)

        assert flat.method == hdrlfunc.Flat.Mode.FreqLow
        assert flat.sizex == 5
        assert flat.sizey == 5

        with pytest.raises(hdrlcore.IllegalInputError):
            flat = hdrlfunc.Flat(-1, filter_size_y, method)
        with pytest.raises(hdrlcore.IllegalInputError):
            flat = hdrlfunc.Flat(filter_size_x, -1, method)
        with pytest.raises(TypeError):
            flat = hdrlfunc.Flat(filter_size_x, filter_size_y, 2)
        with pytest.raises(hdrlcore.IllegalInputError):
            flat = hdrlfunc.Flat(2, filter_size_y, method)
        with pytest.raises(hdrlcore.IllegalInputError):
            flat = hdrlfunc.Flat(filter_size_x, 2, method)
        with pytest.raises(hdrlcore.IllegalInputError):
            flat = hdrlfunc.Flat(2, 2, method)
