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

import numpy as np
import pytest
from cpl import core as cplcore
from cpl import drs as cpldrs
from hdrl import core as hdrlcore
from hdrl import debug as hdrldebug
from hdrl import func as hdrlfunc


class TestTypes:
    def test_2d_wcs(self, make_tmp_filename):
        # HDRL_SIZE_X
        sx = 50
        # HDRL_SIZE_Y
        sy = 50
        naxis = 2
        ra = 10.0
        dec = 20.0
        # HDRL_CD11 etc.
        cd11 = -3.47222e-05
        cd22 = 3.47222e-05
        cd12 = 0.0
        cd21 = 0.0
        crpix1 = 33.5
        crpix2 = 33.5
        crval1 = 48.0706
        crval2 = -20.6219
        cdelt1 = abs(cd11)
        cdelt2 = abs(cd22)
        cunit1 = ""
        cunit2 = ""
        ctype1 = "pix"
        ctype2 = "pix"

        plist = self.hdrl_resample_util_crea_header_image(
            make_tmp_filename(),
            naxis,
            sx,
            sy,
            ra,
            dec,
            cd11,
            cd12,
            cd21,
            cd22,
            crpix1,
            crpix2,
            crval1,
            crval2,
            cdelt1,
            cdelt2,
            ctype1,
            ctype2,
            cunit1,
            cunit2,
        )
        w1 = cpldrs.WCS(plist)
        assert isinstance(w1, cpldrs.WCS)
        w2 = hdrldebug.Types.wcs(w1)
        assert isinstance(w2, cpldrs.WCS)
        assert w1.image_dims == w2.image_dims
        for i in range(0, 2):
            assert w1.crval[i] == w2.crval[i]
            assert w1.crpix[i] == w2.crpix[i]
            assert w1.cunit[i] == w2.cunit[i]
            assert w1.ctype[i] == w2.ctype[i]
        for x in range(0, 2):
            for y in range(0, 2):
                assert w1.cd[x][y] == w2.cd[x][y]

    def test_3d_wcs(self, make_tmp_filename):
        naxis = 3
        sx = 67
        sy = 67
        # HDRL_FLUX_ADU
        value = 100
        ra = 10.0
        dec = 20.0
        cd11 = -3.47222e-05
        cd22 = 3.47222e-05
        cd12 = 0
        cd21 = 0
        crpix1 = 33.5
        crpix2 = 33.5
        crval1 = 48.0706
        crval2 = -20.6219
        cdelt1 = 0
        cdelt2 = 0
        cunit1 = "deg"
        cunit2 = "deg"
        ctype1 = "RA---TAN"
        ctype2 = "DEC--TAN"
        # 3D keywords
        sz = 2218
        cd13 = 0
        cd31 = 0
        cd23 = 0
        cd32 = 0
        cd33 = 2.45e-10
        crpix3 = 1
        crval3 = 1.9283e-06
        cdelt3 = 0.1
        cunit3 = "m"
        ctype3 = "WAVE"

        plist = self.hdrl_resample_crea_header_cube(
            make_tmp_filename(),
            naxis,
            sx,
            sy,
            sz,
            ra,
            dec,
            cd11,
            cd12,
            cd21,
            cd22,
            cd13,
            cd31,
            cd23,
            cd32,
            cd33,
            crpix1,
            crpix2,
            crpix3,
            crval1,
            crval2,
            crval3,
            cdelt1,
            cdelt2,
            cdelt3,
            ctype1,
            ctype2,
            ctype3,
            cunit1,
            cunit2,
            cunit3,
        )

        w1 = cpldrs.WCS(plist)
        assert isinstance(w1, cpldrs.WCS)
        w2 = hdrldebug.Types.wcs(w1)
        assert isinstance(w2, cpldrs.WCS)
        assert w1.image_dims == w2.image_dims
        for i in range(0, 3):
            assert w1.crval[i] == w2.crval[i]
            assert w1.crpix[i] == w2.crpix[i]
            assert w1.cunit[i] == w2.cunit[i]
            assert w1.ctype[i] == w2.ctype[i]
        for x in range(0, 3):
            for y in range(0, 3):
                assert w1.cd[x][y] == w2.cd[x][y]

    def hdrl_resample_crea_header_cube(
        self,
        filename,
        naxis,
        sx,
        sy,
        sz,
        ra,
        dec,
        cd11,
        cd12,
        cd21,
        cd22,
        cd13,
        cd31,
        cd23,
        cd32,
        cd33,
        crpix1,
        crpix2,
        crpix3,
        crval1,
        crval2,
        crval3,
        cdelt1,
        cdelt2,
        cdelt3,
        ctype1,
        ctype2,
        ctype3,
        cunit1,
        cunit2,
        cunit3,
    ):
        # create first the FITS header for a 2D example
        plist = self.hdrl_resample_util_crea_header_image(
            filename,
            naxis,
            sx,
            sy,
            ra,
            dec,
            cd11,
            cd12,
            cd21,
            cd22,
            crpix1,
            crpix2,
            crval1,
            crval2,
            cdelt1,
            cdelt2,
            ctype1,
            ctype2,
            cunit1,
            cunit2,
        )

        plist["NAXIS"].value = naxis
        plist.append(cplcore.Property("NAXIS3", cplcore.Type.INT, sz))
        plist.append(cplcore.Property("CRPIX3", cplcore.Type.DOUBLE, crpix3))
        plist.append(cplcore.Property("CRVAL3", cplcore.Type.DOUBLE, crval3))
        plist.append(cplcore.Property("CDELT3", cplcore.Type.DOUBLE, cdelt3))
        plist.append(cplcore.Property("CTYPE3", cplcore.Type.STRING, ctype3))
        plist.append(cplcore.Property("CUNIT3", cplcore.Type.STRING, cunit3))
        plist.append(cplcore.Property("CD1_3", cplcore.Type.DOUBLE, cd13))
        plist.append(cplcore.Property("CD3_1", cplcore.Type.DOUBLE, cd31))
        plist.append(cplcore.Property("CD2_3", cplcore.Type.DOUBLE, cd23))
        plist.append(cplcore.Property("CD3_2", cplcore.Type.DOUBLE, cd32))
        plist.append(cplcore.Property("CD3_3", cplcore.Type.DOUBLE, cd33))

        # To be sure to have a standard FITS header we save and reload the imagelist
        img = cplcore.Image.zeros(sx, sy, cplcore.Type.INT)
        img.add_scalar(1)
        iml = cplcore.ImageList()
        for i in range(0, sz):
            iml.append(img)
        iml.save(filename, plist, cplcore.io.CREATE, cplcore.Type.INT)
        loaded_plist = cplcore.PropertyList.load(filename, 0)
        return loaded_plist

    def hdrl_resample_util_crea_header_image(
        self,
        filename,
        naxis,
        sx,
        sy,
        ra,
        dec,
        cd11,
        cd12,
        cd21,
        cd22,
        crpix1,
        crpix2,
        crval1,
        crval2,
        cdelt1,
        cdelt2,
        ctype1,
        ctype2,
        cunit1,
        cunit2,
    ):
        plist = cplcore.PropertyList()

        plist.append(cplcore.Property("NAXIS", cplcore.Type.INT, naxis))

        plist.append(cplcore.Property("NAXIS1", cplcore.Type.INT, sx))
        plist.append(cplcore.Property("NAXIS2", cplcore.Type.INT, sy))

        plist.append(cplcore.Property("RA", cplcore.Type.DOUBLE, ra))
        plist.append(cplcore.Property("DEC", cplcore.Type.DOUBLE, dec))

        plist.append(cplcore.Property("CRPIX1", cplcore.Type.DOUBLE, crpix1))
        plist.append(cplcore.Property("CRPIX2", cplcore.Type.DOUBLE, crpix2))
        plist.append(cplcore.Property("CRVAL1", cplcore.Type.DOUBLE, crval1))
        plist.append(cplcore.Property("CRVAL2", cplcore.Type.DOUBLE, crval2))
        plist.append(cplcore.Property("CDELT1", cplcore.Type.DOUBLE, cdelt1))
        plist.append(cplcore.Property("CDELT2", cplcore.Type.DOUBLE, cdelt2))
        plist.append(cplcore.Property("CTYPE1", cplcore.Type.STRING, ctype1))
        plist.append(cplcore.Property("CTYPE2", cplcore.Type.STRING, ctype2))
        plist.append(cplcore.Property("CUNIT1", cplcore.Type.STRING, cunit1))
        plist.append(cplcore.Property("CUNIT2", cplcore.Type.STRING, cunit2))
        plist.append(cplcore.Property("CD1_1", cplcore.Type.DOUBLE, cd11))
        plist.append(cplcore.Property("CD1_2", cplcore.Type.DOUBLE, cd12))
        plist.append(cplcore.Property("CD2_1", cplcore.Type.DOUBLE, cd21))
        plist.append(cplcore.Property("CD2_2", cplcore.Type.DOUBLE, cd22))

        # To be sure to have a standard FITS header we save and reload the image
        img = cplcore.Image.zeros(sx, sy, cplcore.Type.INT)
        img.add_scalar(1)
        img.save(filename, plist, cplcore.io.CREATE, cplcore.Type.INT)
        loaded_plist = cplcore.PropertyList.load(filename, 0)
        return loaded_plist

    def test_basic_imagelist(self):
        data = [
            [0.0, 1.0],
            [2.0, 3.0],
            [4.0, -5.0],
            [6.5, 7.0],
            [8.0, 9.0],
            [10.0, 11.0],
        ]
        edata = [
            [2.0, 3.0],
            [4.0, 5.0],
            [6.0, -7.0],
            [6.0, 7.5],
            [8.0, 9.0],
            [10.0, 11.0],
        ]
        img = cplcore.Image(data, cplcore.Type.FLOAT)
        err = cplcore.Image(edata, cplcore.Type.FLOAT)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True
        mask[2][0] = True
        mask[3][1] = True
        img.reject_from_mask(mask)

        ilist = cplcore.ImageList()
        ilist.append(img)
        ilist.append(err)

        casted_ilist = hdrldebug.Types.imagelist(ilist)
        assert str(ilist) == str(casted_ilist)

    def test_basic_vector(self):
        vec = cplcore.Vector.zeros(5)
        vec[0] = 3.14515
        vec[3] = 1e20
        print(type(vec))
        casted_vector = hdrldebug.Types.vector(vec)
        assert str(vec) == str(casted_vector)

    def test_basic_mask(self):
        data = [[0, 1], [2, 3], [4, -5], [6, 7], [8, 9], [10, 11]]
        img = cplcore.Image(data, cplcore.Type.INT)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True
        mask[2][0] = True
        mask[3][1] = True

        casted_mask = hdrldebug.Types.mask(mask)
        assert str(mask) == str(casted_mask)

    def test_basic_table(self):
        t = cplcore.Table.empty(3)
        col = "ivals"
        t.new_column(col, cplcore.Type.INT)
        t[col] = [-1, 5, 3]
        t.set_invalid(col, 0)
        casted_table = hdrldebug.Types.table(t)
        assert str(t) == str(casted_table)

    def test_table_arrays(self):
        # create a table with array type columns of various types
        # to test the handling of array types by the type caster
        t = cplcore.Table.empty(3)
        t["ints"] = np.array([[5, 6, 7], [3, 4, 2], [10, 20, 30]])
        t.new_column_array("strings", cplcore.Type.STRING, 3)
        t.new_column_array("cfloats", cplcore.Type.FLOAT_COMPLEX, 2)
        t.new_column_array("cdouble", cplcore.Type.DOUBLE_COMPLEX, 2)
        # note that strings cannot be set using this method. See PIPE-11183
        # t["strings"] = np.array([["aaa", "bbb","ddd"], ["ccc", "ddd","aaa"], ["eee","fff","eee"]])
        t["strings"][0] = np.array(["a", "b", "c"])
        t["strings"][1] = np.array(["what", "", "e"])
        t["strings"][2] = np.array(["", "d", "e"])

        t["floats"] = np.array([[0.0, 1.4], [2.3, 3.2], [4.5, 5.5]], dtype=np.float32)
        t["doubles"] = np.array([[0.1e10, 1.4e12], [2.3e67, 3.2e-230], [4.5e33, 5.5e50]], dtype=np.double)
        t["longlongs"] = np.array(
            [[1e6, 534523623465], [55342342342434232, 300000], [400000, 500000]],
            dtype=np.longlong,
        )
        t["cfloats"][0] = np.array([5.2, 7e5 + 22.2j], dtype=np.csingle)
        t["cfloats"][1] = np.array([60.22e2 + 56e-3j, 55.11 + 22j], dtype=np.csingle)
        t["cdouble"][0] = np.array([5.2, 7e50 + 22.2j], dtype=np.cdouble)
        t["cdouble"][1] = np.array([60.22e50 + 56e-300j, 55.11 + 22j], dtype=np.cdouble)

        # set some rows as invalid to check that the invalid rows are handled properly
        for cname in [
            "ints",
            "strings",
            "floats",
            "longlongs",
            "doubles",
            "cdouble",
            "cfloats",
        ]:
            t.set_invalid(cname, 1)

        casted_table = hdrldebug.Types.table(t)

        for cname in [
            "ints",
            "strings",
            "floats",
            "longlongs",
            "doubles",
            "cdouble",
            "cfloats",
        ]:
            assert np.array_equal(t[cname].as_array, casted_table[cname].as_array)
        assert str(t) == str(casted_table)

    def test_table_all_types(self):
        t = cplcore.Table.empty(3)
        t.new_column("ints", cplcore.Type.INT)
        t.new_column("longlongs", cplcore.Type.LONG_LONG)
        t.new_column("floats", cplcore.Type.FLOAT)
        t.new_column("strings", cplcore.Type.STRING)
        t.new_column("doubles", cplcore.Type.DOUBLE)
        t.new_column("float_complex", cplcore.Type.FLOAT_COMPLEX)
        t.new_column("double_complex", cplcore.Type.DOUBLE_COMPLEX)
        t["ints"] = [1, 2, 3]
        t["floats"] = [1.2, 5.6, 2.3]
        t["longlongs"] = [31230000000000000, 31000000000232332, 9223372036854775800]
        t["doubles"] = [1.2e30, 5.6e50, 2.3e-66]
        t["strings"] = ["foo", "barr", "c"]
        t["float_complex"] = [96.3 + 4j, 22.2 + 5j, 96.3 + 4j]
        t["double_complex"] = [3 + 2j, -60000 + 2323j, 322323.0002 - 3.14j]
        for cname in [
            "ints",
            "floats",
            "longlongs",
            "doubles",
            "strings",
            "double_complex",
            "float_complex",
        ]:
            t.set_invalid(cname, 2)
        casted_table = hdrldebug.Types.table(t)
        assert str(t) == str(casted_table)

    def test_table_long(self):
        t = cplcore.Table.empty(81)
        col = "ra"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
            3.52452e02,
            3.52433e02,
            3.52413e02,
            3.52394e02,
            3.52374e02,
            3.52355e02,
            3.52336e02,
            3.52316e02,
            3.52297e02,
        ]

        col = "dec"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90366e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90266e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90166e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.90066e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89966e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89866e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89766e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89666e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
            -5.89566e01,
        ]

        col = "lambda"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
            0.00000e00,
        ]

        col = "data"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            4.90000e01,
            4.90000e01,
            4.90000e01,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            4.90000e01,
            4.90000e01,
            4.90000e01,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            4.90000e01,
            4.90000e01,
            4.90000e01,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
        ]

        col = "bpm"
        t.new_column(col, cplcore.Type.INT)
        t[col] = [
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            0,
            0,
            0,
            1,
            1,
            1,
            1,
            1,
            1,
            0,
            0,
            0,
            1,
            1,
            1,
            1,
            1,
            1,
            0,
            0,
            0,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
        ]

        col = "errors"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            7.00000e00,
            7.00000e00,
            7.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            7.00000e00,
            7.00000e00,
            7.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            7.00000e00,
            7.00000e00,
            7.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
            1.00000e00,
        ]

        casted_table = hdrldebug.Types.table(t)
        assert str(t) == str(casted_table)

    def test_propertylist_values(self):
        plist = cplcore.PropertyList()

        p = cplcore.Property("p1", cplcore.Type.CHAR)
        p.value = "c"
        plist.append(p)

        p = cplcore.Property("p2", cplcore.Type.BOOL)
        p.value = True
        p.comment = "bool property"
        plist.append(p)

        p = cplcore.Property("p3", cplcore.Type.INT)
        p.value = 42
        p.comment = "int property"
        plist.append(p)

        p = cplcore.Property("p4", cplcore.Type.LONG)
        p.value = 2100000000
        p.comment = "long property"
        plist.append(p)

        p = cplcore.Property("p5", cplcore.Type.LONG_LONG)
        p.value = 500000000000
        p.comment = "long long property"
        plist.append(p)

        p = cplcore.Property("p6", cplcore.Type.DOUBLE)
        p.value = 3.0e40
        p.comment = "double property"
        plist.append(p)

        p = cplcore.Property("p7", cplcore.Type.FLOAT)
        p.value = 4.50
        p.comment = "float property"
        plist.append(p)

        p = cplcore.Property("p8", cplcore.Type.DOUBLE_COMPLEX)
        p.value = 4.0e50 - 3.0e-1j
        p.comment = "double complex property"
        plist.append(p)

        p = cplcore.Property("p9", cplcore.Type.FLOAT_COMPLEX)
        p.value = 4.0 + 3.0j
        p.comment = "float complex property"
        plist.append(p)

        p = cplcore.Property("p10", cplcore.Type.STRING)
        p.value = "This is a long string value"
        p.comment = "string property"
        plist.append(p)

        casted_plist = hdrldebug.Types.propertylist(plist)

        assert casted_plist[0].name == "p1"
        assert casted_plist[0].value == "c"
        # assert casted_plist[0].type == cplcore.Type.CHAR
        assert casted_plist[0].comment is None

        assert casted_plist[1].name == "p2"
        assert casted_plist[1].value == True
        assert casted_plist[1].type == cplcore.Type.BOOL
        assert casted_plist[1].comment == "bool property"

        assert casted_plist[2].name == "p3"
        assert casted_plist[2].value == 42
        assert casted_plist[2].type == cplcore.Type.INT
        assert casted_plist[2].comment == "int property"

        assert casted_plist[3].name == "p4"
        assert casted_plist[3].value == 2100000000
        assert casted_plist[3].type == cplcore.Type.LONG
        assert casted_plist[3].comment == "long property"

        assert casted_plist[4].name == "p5"
        assert casted_plist[4].value == 500000000000
        assert casted_plist[4].type == cplcore.Type.LONG_LONG
        assert casted_plist[4].comment == "long long property"

        assert casted_plist[5].name == "p6"
        assert casted_plist[5].value == pytest.approx(3.0e40)
        assert casted_plist[5].type == cplcore.Type.DOUBLE
        assert casted_plist[5].comment == "double property"

        assert casted_plist[6].name == "p7"
        assert casted_plist[6].value == pytest.approx(4.50)
        assert casted_plist[6].type == cplcore.Type.FLOAT
        assert casted_plist[6].comment == "float property"

        assert casted_plist[7].name == "p8"
        assert casted_plist[7].value == pytest.approx(4.0e50 - 3.0e-1j)
        assert casted_plist[7].type == cplcore.Type.DOUBLE_COMPLEX
        assert casted_plist[7].comment == "double complex property"

        assert casted_plist[8].name == "p9"
        assert casted_plist[8].value == pytest.approx(4.0 + 3.0j)
        # assert casted_plist[8].type == cplcore.Type.FLOAT_COMPLEX
        assert casted_plist[8].comment == "float complex property"

        assert casted_plist[9].name == "p10"
        assert casted_plist[9].value == "This is a long string value"
        assert casted_plist[9].type == cplcore.Type.STRING
        assert casted_plist[9].comment == "string property"

    def test_basic_vector_2(self):
        vec = cplcore.Vector.zeros(5)
        vec[0] = 3.14515
        vec[3] = 1e20
        print(type(vec))
        casted_vector = hdrldebug.Types.vector(vec)
        assert str(vec) == str(casted_vector)

    @pytest.mark.xfail(reason="cpl.core.Property does not handle errors properly. See PIPE-11144.")
    def test_propertylist_novalues(self):
        plist = cplcore.PropertyList()

        p = cplcore.Property("p1", cplcore.Type.CHAR)
        plist.append(p)

        p = cplcore.Property("p2", cplcore.Type.BOOL)
        p.comment = "bool property"
        plist.append(p)

        p = cplcore.Property("p3", cplcore.Type.INT)
        p.comment = "int property"
        plist.append(p)

        p = cplcore.Property("p4", cplcore.Type.LONG)
        p.comment = "long property"
        plist.append(p)

        p = cplcore.Property("p5", cplcore.Type.LONG_LONG)
        p.comment = "long long property"
        plist.append(p)

        p = cplcore.Property("p6", cplcore.Type.DOUBLE)
        p.comment = "double property"
        plist.append(p)

        p = cplcore.Property("p7", cplcore.Type.FLOAT)
        p.comment = "float property"
        plist.append(p)

        p = cplcore.Property("p8", cplcore.Type.DOUBLE_COMPLEX)
        p.comment = "double complex property"
        plist.append(p)

        p = cplcore.Property("p9", cplcore.Type.FLOAT_COMPLEX)
        p.comment = "float complex property"
        plist.append(p)

        p = cplcore.Property("p10", cplcore.Type.STRING)
        p.comment = "string property"
        plist.append(p)

        casted_plist = hdrldebug.Types.propertylist(plist)

        assert casted_plist[0].name == "p1"
        assert casted_plist[0].value == None  # ''
        # assert casted_plist[0].type == cplcore.Type.CHAR
        assert casted_plist[0].comment is None

        assert casted_plist[1].name == "p2"
        assert casted_plist[1].value == None  # False
        assert casted_plist[1].type == cplcore.Type.BOOL
        assert casted_plist[1].comment == "bool property"

        assert casted_plist[2].name == "p3"
        assert casted_plist[2].value == None  # 0
        assert casted_plist[2].type == cplcore.Type.INT
        assert casted_plist[2].comment == "int property"

        assert casted_plist[3].name == "p4"
        assert casted_plist[3].value == None  # 0
        assert casted_plist[3].type == cplcore.Type.LONG
        assert casted_plist[3].comment == "long property"

        assert casted_plist[4].name == "p5"
        assert casted_plist[4].value == None  # 0
        assert casted_plist[4].type == cplcore.Type.LONG_LONG
        assert casted_plist[4].comment == "long long property"

        assert casted_plist[5].name == "p6"
        assert casted_plist[5].value == None  # 0.0
        assert casted_plist[5].type == cplcore.Type.DOUBLE
        assert casted_plist[5].comment == "double property"

        assert casted_plist[6].name == "p7"
        assert casted_plist[6].value == None  # 0.0
        assert casted_plist[6].type == cplcore.Type.FLOAT
        assert casted_plist[6].comment == "float property"

        assert casted_plist[7].name == "p8"
        assert casted_plist[7].value == None  # 0.0+0.0j
        assert casted_plist[7].type == cplcore.Type.DOUBLE_COMPLEX
        assert casted_plist[7].comment == "double complex property"

        assert casted_plist[8].name == "p9"
        assert casted_plist[8].value == None  # 0.0+0.0j
        # assert casted_plist[8].type == cplcore.Type.FLOAT_COMPLEX
        assert casted_plist[8].comment == "float complex property"

        assert casted_plist[9].name == "p10"
        assert casted_plist[9].value == None
        assert casted_plist[9].type == cplcore.Type.STRING
        assert casted_plist[9].comment == "string property"

    def test_image_int(self):
        data = [[0, 1], [2, 3], [4, -5], [6, 7], [8, 9], [10, 11]]
        img = cplcore.Image(data, cplcore.Type.INT)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True
        mask[2][0] = True
        mask[3][1] = True

        # no mask
        casted_image = hdrldebug.Types.image(img)
        assert str(img) == str(casted_image)

        # with mask
        img.reject_from_mask(mask)
        casted_image = hdrldebug.Types.image(img)
        assert img.bpm[1][1] == True
        assert img.bpm[2][0] == True
        assert img.bpm[3][1] == True

        assert str(img) == str(casted_image)

    def test_image_float(self):
        data = [
            [0.0, 1.0],
            [2.0, 3.0],
            [4.0, -5.0],
            [6.5, 7.0],
            [8.0, 9.0],
            [10.0, 11.0],
        ]
        img = cplcore.Image(data, cplcore.Type.FLOAT)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True
        mask[2][0] = True
        mask[3][1] = True

        # no mask
        casted_image = hdrldebug.Types.image(img)
        assert str(img) == str(casted_image)

        # with mask
        img.reject_from_mask(mask)
        casted_image = hdrldebug.Types.image(img)
        assert img.bpm[1][1] == True
        assert img.bpm[2][0] == True
        assert img.bpm[3][1] == True

        assert str(img) == str(casted_image)

    def test_image_double(self):
        data = [
            [0.0, 1.0],
            [2.0, 3.0e30],
            [4.0, -5.0],
            [6.5, 7.0],
            [8.0, 9.0],
            [10.0, 11.0],
        ]
        img = cplcore.Image(data, cplcore.Type.DOUBLE)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True
        mask[2][0] = True
        mask[3][1] = True

        # no mask
        casted_image = hdrldebug.Types.image(img)
        assert str(img) == str(casted_image)

        # with mask
        img.reject_from_mask(mask)
        casted_image = hdrldebug.Types.image(img)
        assert img.bpm[1][1] == True
        assert img.bpm[2][0] == True
        assert img.bpm[3][1] == True

        assert str(img) == str(casted_image)

    def test_image_float_complex(self):
        data = [[0.0 + 10j, 1.0 + 30.04j], [2.0 - 1j, 3.0 + 3.4j]]
        img = cplcore.Image(data, cplcore.Type.FLOAT_COMPLEX)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True

        # no mask
        casted_image = hdrldebug.Types.image(img)
        assert str(img) == str(casted_image)

        # with mask
        img.reject_from_mask(mask)
        casted_image = hdrldebug.Types.image(img)
        assert img.bpm[1][1] == True

        assert str(img) == str(casted_image)

    def test_image_double_complex(self):
        data = [[0.0 + 10j, 1.0 + 30j], [2.0 - 1j, 3.0e30]]
        img = cplcore.Image(data, cplcore.Type.DOUBLE_COMPLEX)
        mask = cplcore.Mask(img.width, img.height)
        mask[1][1] = True

        # no mask
        casted_image = hdrldebug.Types.image(img)
        assert str(img) == str(casted_image)

        # with mask
        img.reject_from_mask(mask)
        casted_image = hdrldebug.Types.image(img)
        assert img.bpm[1][1] == True

        assert str(img) == str(casted_image)
