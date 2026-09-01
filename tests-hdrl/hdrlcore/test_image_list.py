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

import subprocess
import sys

import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestImageList:
    def test_constructor(self):
        himlist = hdrlcore.ImageList()
        assert len(himlist) == 0

    def test_constructor_list(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img1, img2, img3])
        imlist_err = cplcore.ImageList([img1, img2, img3])

        himglist = hdrlcore.ImageList(imlist, imlist_err)
        assert len(himglist) == 3
        assert himglist[0].image[0][0] == 1

        data = cplcore.ImageList()
        errs = cplcore.ImageList()
        img = cplcore.Image.zeros(64, 64, cplcore.Type.DOUBLE)
        err = cplcore.Image.zeros(64, 64, cplcore.Type.DOUBLE)
        img.add_scalar(1.0)
        err.add_scalar(0.05)

        n = 5
        for i in range(n):
            data.append(img.duplicate())
            errs.append(err.duplicate())

        himglist2 = hdrlcore.ImageList(data, errs)
        assert len(himglist2) == n

    def test_get(self):
        himglist = None
        with pytest.raises(AttributeError):
            himglist.size_x == -1
        with pytest.raises(AttributeError):
            himglist.size_y == -1
        with pytest.raises(TypeError):
            len(himglist) == 0

        himglist = hdrlcore.ImageList()

        assert len(himglist) == 0
        with pytest.raises(hdrlcore.IllegalInputError):
            himglist.size_x == -1
        with pytest.raises(hdrlcore.IllegalInputError):
            himglist.size_y == -1

        himglist2 = hdrlcore.ImageList()
        himglist2.append(hdrlcore.Image.zeros(5, 6))
        assert himglist2.size_x == 5
        assert himglist2.size_y == 6
        assert len(himglist2) == 1

        px = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        py = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        himg1 = hdrlcore.Image(px, py)
        himg2 = hdrlcore.Image(px, py)

        himglist.append(himg1)
        himglist.append(himg2)
        assert len(himglist) == 2
        assert himglist.size_x == 3
        assert himglist.size_y == 4

        himglist.add_scalar((2, 5.0))
        himg = himglist.pop(1)
        assert len(himglist) == 1
        assert himg.get_pixel(0, 0).data == 2
        assert himg.get_pixel(0, 0).error == 5

    def test_interface(self):
        himglist = hdrlcore.ImageList()

        px = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        py = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        himg1 = hdrlcore.Image(px, py)
        himg2 = hdrlcore.Image(px, py)

        himglist.append(himg1)
        himglist.append(himg2)
        output = """Imagelist with 2 image(s)
Image nb 0 of 2 in imagelist
Image with 3 X 4 pixel(s) of type 'double' and 0 bad pixel(s)
Image nb 1 of 2 in imagelist
Image with 3 X 4 pixel(s) of type 'double' and 0 bad pixel(s)
"""
        assert len(himglist) == 2
        assert output == repr(himglist)
        himglist.empty()
        assert len(himglist) == 0

    def test_dump_bad_window(self):
        img1 = cplcore.Image([[1, 2, 3, 5, 4]])
        img2 = cplcore.Image([[2, 3, 4, 5, 6]])
        img3 = cplcore.Image([[5, 6, 7, 8, 9]])
        im_err = cplcore.Image.zeros(5, 1, cplcore.Type.DOUBLE)
        himglist = hdrlcore.ImageList(
            cplcore.ImageList([img1, img2, img3]),
            cplcore.ImageList([im_err, im_err, im_err]),
        )

        with pytest.raises(hdrlcore.AccessOutOfRangeError):
            himglist.dump(window=(0, 0, 6, 6))
        with pytest.raises(hdrlcore.IllegalInputError):
            himglist.dump(window=(5, 0, 1, 1))

        def test_str(self):
            img1 = cplcore.Image([
                [
                    1,
                    2,
                ]
            ])
            img2 = cplcore.Image([
                [
                    2,
                    3,
                ]
            ])
            im_err = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
            himglist = hdrlcore.ImageList(cplcore.ImageList([img1, img2]), cplcore.ImageList([im_err, im_err]))
            output = """Image nb 0 of 2 in imagelist
    #----- image: 1 <= x <= 2, 1 <= y <= 1 -----
        X   Y   value
        1   1   1
        2   1   2
    Image nb 1 of 2 in imagelist
    #----- image: 1 <= x <= 2, 1 <= y <= 1 -----
        X   Y   value
        1   1   2
        2   1   3
    """
            assert str(himglist) == output

    def test_dump_string(self):
        img1 = cplcore.Image([[1, 2, 3, 5, 4]])
        img2 = cplcore.Image([[2, 3, 4, 5, 6]])
        im_err = cplcore.Image.zeros(5, 1, cplcore.Type.DOUBLE)
        himglist = hdrlcore.ImageList(cplcore.ImageList([img1, img2]), cplcore.ImageList([im_err, im_err]))
        output = """Image nb 0 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
	4	1	5
	5	1	4
Image nb 1 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
	4	1	5
	5	1	6
"""
        assert himglist.dump(window=None, show=False) == output

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "from hdrl import core as hdrlcore; "
        cmd += "img1 = cplcore.Image([[1, 2, 3, 5, 4]]);"
        cmd += "img2 = cplcore.Image([[2, 3, 4, 5, 6]]);"
        cmd += "imglist = cplcore.ImageList([img1, img2]);"
        cmd += "im_err = cplcore.Image.zeros(5, 1, cplcore.Type.DOUBLE);"
        cmd += "himglist = hdrlcore.ImageList(imglist, cplcore.ImageList([im_err, im_err]));"
        cmd += "himglist.dump()"
        response = subprocess.run([sys.executable, "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Image nb 0 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
	4	1	5
	5	1	4
Image nb 1 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
	4	1	5
	5	1	6
"""
        # print(outp)
        assert outp == expect

    def test_dump_file(self, tmp_path):
        img1 = cplcore.Image([[1, 2, 3, 5, 4]])
        img2 = cplcore.Image([[2, 3, 4, 5, 6]])
        im_err = cplcore.Image.zeros(5, 1, cplcore.Type.DOUBLE)
        himglist = hdrlcore.ImageList(cplcore.ImageList([img1, img2]), cplcore.ImageList([im_err, im_err]))
        outp = """Image nb 0 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
	4	1	5
	5	1	4
Image nb 1 of 2 in imagelist
#----- image: 1 <= x <= 5, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
	4	1	5
	5	1	6
"""
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_imagelist_dump.txt"
        filename = tmp_path.joinpath(p)
        himglist.dump(filename=str(filename))
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_basic_imagelist(self):
        himglist1 = hdrlcore.ImageList()
        himglist2 = hdrlcore.ImageList()
        cimg1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cerr = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg1[0][0] = 8912
        cimg2[0][0] = -4092
        himg1 = hdrlcore.Image(cimg1, cerr)
        himg2 = hdrlcore.Image(cimg2, cerr)
        himglist1.append(himg1)
        himglist2.append(himg2)
        himglist1.add_imagelist(himglist2)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == 8912 + -4092

        himglist1[0].set_pixel(0, 0, (-2.0, 0.0))
        himglist2[0].set_pixel(0, 0, (5.0, 0.0))
        himglist1.sub_imagelist(himglist2)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(-2.0 - 5.0)

        himglist1[0].set_pixel(0, 0, (-2.0, 0.0))
        himglist2[0].set_pixel(0, 0, (5.0, 0.0))
        himglist1.mul_imagelist(himglist2)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(-2.0 * 5.0)

        himglist1[0].set_pixel(0, 0, (-20.0, 0.0))
        himglist2[0].set_pixel(0, 0, (5.0, 0.0))
        himglist1.div_imagelist(himglist2)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(-20.0 / 5.0)

    def test_basic_image(self):
        himglist1 = hdrlcore.ImageList()
        cimg1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg3 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cerr = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg1[0][0] = 1
        cimg2[0][0] = 2
        cimg3[0][0] = 3
        himglist1.append(hdrlcore.Image(cimg1, cerr))
        himglist1.append(hdrlcore.Image(cimg2, cerr))
        himg = hdrlcore.Image(cimg3, cerr)

        himglist1.add_image(himg)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == 1 + 3
        assert himglist1[1].get_pixel(0, 0).data == 2 + 3

        himglist1[0].set_pixel(0, 0, (1.0, 0.0))
        himglist1[1].set_pixel(0, 0, (2.0, 0.0))
        himglist1.sub_image(himg)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(1.0 - 3.0)
        assert himglist1[1].get_pixel(0, 0).data == pytest.approx(2.0 - 3.0)

        himglist1[0].set_pixel(0, 0, (1.0, 0.0))
        himglist1[1].set_pixel(0, 0, (2.0, 0.0))
        himglist1.mul_image(himg)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(1.0 * 3.0)
        assert himglist1[1].get_pixel(0, 0).data == pytest.approx(2.0 * 3.0)

        himglist1[0].set_pixel(0, 0, (21.0, 0.0))
        himglist1[1].set_pixel(0, 0, (18.0, 0.0))
        himglist1.div_image(himg)
        val = himglist1[0].get_pixel(0, 0)
        assert val.data == pytest.approx(21.0 / 3)
        assert himglist1[1].get_pixel(0, 0).data == pytest.approx(18.0 / 3.0)

    def test_imagelist_duplicate(self):
        himglist1 = hdrlcore.ImageList()
        cimg1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cerr = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg1[0][0] = 1
        cimg2[0][0] = 2
        himglist1.append(hdrlcore.Image(cimg1, cerr))
        himglist1.append(hdrlcore.Image(cimg2, cerr))
        copy_himglist1 = himglist1.duplicate()
        assert copy_himglist1[0].get_pixel(0, 0).data == himglist1[0].get_pixel(0, 0).data
        assert copy_himglist1[1].get_pixel(0, 0).data == himglist1[1].get_pixel(0, 0).data

    def test_imagelist_is_consistent(self):
        himglist1 = hdrlcore.ImageList()
        assert himglist1.is_consistent() == 1

        cimg1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cerr = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg1[0][0] = 1
        cimg2[0][0] = 2
        himglist1.append(hdrlcore.Image(cimg1, cerr))
        himglist1.append(hdrlcore.Image(cimg2, cerr))
        assert himglist1.is_consistent() == 0

    def test_basic(self):
        himglist1 = hdrlcore.ImageList()
        himglist2 = hdrlcore.ImageList()
        val = (100.0, 10.0)
        scalar = (1000.0, 100.0)
        exponent = (2.0, 1.0)
        imgsize = 265
        himg1 = hdrlcore.Image.zeros(imgsize, imgsize)
        himg2 = hdrlcore.Image.zeros(imgsize, imgsize)
        himg3 = hdrlcore.Image.zeros(imgsize, imgsize)
        himg1.add_scalar(scalar)
        himg1.sub_scalar(val)

        himglist1.add_image(himg1)
        himglist2.add_image(himg2)
        himglist2.sub_image(himg2)
        himglist2.add_image(himg2)

        himglist1.add_imagelist(himglist2)
        himglist1.sub_imagelist(himglist2)
        himglist1.add_imagelist(himglist2)

        himglist1.div_scalar(scalar)
        himglist1.div_image(himg1)
        himglist1.div_imagelist(himglist2)

        himglist1.mul_scalar(scalar)
        himglist1.mul_image(himg1)
        himglist1.mul_imagelist(himglist2)

        himglist1.pow_scalar(exponent)

        himglist2.add_image(himg3)
        himglist1.add_imagelist(himglist2)
        himglist1.pow_scalar(exponent)

    def test_basic_scalar(self):
        himglist1 = hdrlcore.ImageList()
        cimg1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg3 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cerr = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        cimg1[0][0] = 1
        cimg2[0][0] = 2
        cimg3[0][0] = 3
        himglist1.append(hdrlcore.Image(cimg1, cerr))
        himglist1.append(hdrlcore.Image(cimg2, cerr))
        himglist1.append(hdrlcore.Image(cimg3, cerr))

    # hdrl_imagelist_collpse tests
    def test_collapse(self):
        nimages = 10
        imgsize = 265
        himglist = hdrlcore.ImageList()

        print("adding lots of images")
        for i in range(nimages):
            himg = hdrlcore.Image.zeros(imgsize, imgsize)
            if i == nimages / 2:
                himg.add_scalar((1000, 100))
            else:
                himg.add_scalar((i, 1))
            himglist.append(himg)
        print("done adding lots of images")

        result1 = himglist.collapse_mean()
        mean = hdrlfunc.Collapse.Mean()
        result2 = hdrlfunc.Collapse.compute(mean, himglist)
        # assert himg1.out.image == cplcore.Image([[35.0, 45.0, 55.0], [25.0, 35.0, 45.0]])
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        print("done with collapse mean")

        result1 = himglist.collapse_median()
        median = hdrlfunc.Collapse.Median()
        result2 = hdrlfunc.Collapse.compute(median, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        wmean = hdrlfunc.Collapse.WeightedMean()
        result1 = himglist.collapse_weighted_mean()
        result2 = hdrlfunc.Collapse.compute(wmean, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        sclip = hdrlfunc.Collapse.Sigclip(1.0, 3.0, 10)
        result1 = himglist.collapse_sigclip(1.0, 3.0, 10)
        result2 = hdrlfunc.Collapse.compute(sclip, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        minmaxclip = hdrlfunc.Collapse.MinMax(1.0, 3.0)
        result1 = himglist.collapse_minmax(1.0, 3.0)
        result2 = hdrlfunc.Collapse.compute(minmaxclip, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        hmode = hdrlfunc.Collapse.Mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        result1 = himglist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        result2 = hdrlfunc.Collapse.compute(hmode, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        hmode = hdrlfunc.Collapse.Mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Weighted, 0)
        result1 = himglist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Weighted, 0)
        result2 = hdrlfunc.Collapse.compute(hmode, himglist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        result1 = himglist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        assert result1.out.image.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result1.out.error.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result1.contrib.get_sqflux() == (nimages * nimages) * imgsize * imgsize

        result1 = himglist.collapse_mode(1.0, 20.0, 1.0, hdrlfunc.Collapse.Method.Median, 10)
        assert result1.out.image.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result1.out.error.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result1.contrib.get_sqflux() == (nimages * nimages) * imgsize * imgsize

        result1 = himglist.collapse_mode(-1000.0, -100.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        assert result1.out.image.count_rejected() == imgsize * imgsize
        assert result1.out.error.count_rejected() == imgsize * imgsize
        assert result1.contrib.get_sqflux() == pytest.approx(0.0, abs=1e-12)

        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(None, 3.0, 10)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(1.0, None, 10)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(1.0, 3.0, None)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_mode(None, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0.0)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_mode(10.0, None, 0, hdrlfunc.Collapse.Method.Median, 0.0)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_mode(10, 1.0, None, hdrlfunc.Collapse.Method.Median, 0.0)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_mode(10.0, 1.0, 0, None, 0.0)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, None)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_minmax(None, 3.0)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_minmax(1.0, None)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(None, 3.0, 10)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(1.0, None, 10)
        with pytest.raises(TypeError):
            result1 = himglist.collapse_sigclip(1.0, 3.0, None)
