# This file is part of PyCPL the ESO CPL Python language bindings
# Copyright (C) 2020-2024 European Southern Observatory
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

import os

import pytest
import numpy as np
import subprocess

from cpl import core as cplcore


class TestImageList:
    def test_constructor(self):
        imlist = cplcore.ImageList()
        assert len(imlist) == 0

    def test_constructor_list(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img2, img1, img3])

        assert len(imlist) == 3
        assert imlist[0] == img2
        assert imlist[1] == img1
        assert imlist[2] == img3

    def test_repr(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img2, img1, img3])
        outp = """Imagelist with 3 image(s)
Image nb 0 of 3 in imagelist
Image with 3 X 1 pixel(s) of type 'int' and 0 bad pixel(s)
Image nb 1 of 3 in imagelist
Image with 3 X 1 pixel(s) of type 'int' and 0 bad pixel(s)
Image nb 2 of 3 in imagelist
Image with 3 X 1 pixel(s) of type 'int' and 0 bad pixel(s)
"""
        assert repr(imlist) == outp

    def test_dump_bad_window(self):
        img1 = cplcore.Image([[1, 2, 3, 5, 4]])
        img2 = cplcore.Image([[2, 3, 4, 5, 6]])
        img3 = cplcore.Image([[5, 6, 7, 8, 9]])
        imlist = cplcore.ImageList([img1, img2, img3])

        with pytest.raises(cplcore.AccessOutOfRangeError):
            imlist.dump(window=(0, 0, 6, 6))
        with pytest.raises(cplcore.IllegalInputError):
            imlist.dump(window=(5, 0, 1, 1))

    def test_dump_string(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img2, img1, img3])
        outp = """Image nb 0 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
Image nb 1 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
Image nb 2 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	5
	2	1	6
	3	1	7
"""  # noqa
        assert str(imlist) == outp
        assert isinstance(imlist.dump(show=False), str)
        # test some special cases
        assert imlist.dump(window=None, show=False) == outp
        assert imlist.dump(window=(0, 0, 0, 0), show=False) == outp

    def test_dump_stdout(self):
        # for some reason we can't capture stdout using capsys
        # a cheeky workaround using subprocess does the trick
        cmd = "from cpl import core as cplcore; "
        cmd += "img1 = cplcore.Image([[1, 2, 3]]);"
        cmd += "img2 = cplcore.Image([[2, 3, 4]]);"
        cmd += "img3 = cplcore.Image([[5, 6, 7]]);"
        cmd += "imglist = cplcore.ImageList([img2, img1, img3]);"
        cmd += "imglist.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Image nb 0 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
Image nb 1 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
Image nb 2 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	5
	2	1	6
	3	1	7
"""  # noqa
        assert outp == expect

    def test_dump_file(self, tmp_path):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img2, img1, img3])
        outp = """Image nb 0 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	2
	2	1	3
	3	1	4
Image nb 1 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	1
	2	1	2
	3	1	3
Image nb 2 of 3 in imagelist
#----- image: 1 <= x <= 3, 1 <= y <= 1 -----
	X	Y	value
	1	1	5
	2	1	6
	3	1	7
"""  # noqa
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_imagelist_dump.txt"
        filename = tmp_path.joinpath(p)
        imlist.dump(filename=str(filename))
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_constructor_3darray(self):
        imlist = cplcore.ImageList([[[5, 6, 7]], [[2, 3, 4]], [[1, 2, 3]]])
        assert len(imlist) == 3
        assert imlist[0][0][0] == 5
        assert imlist[0][0][1] == 6
        assert imlist[0][0][2] == 7
        assert imlist[1][0][0] == 2
        assert imlist[1][0][1] == 3
        assert imlist[1][0][2] == 4
        assert imlist[2][0][0] == 1
        assert imlist[2][0][1] == 2
        assert imlist[2][0][2] == 3

    def test_constructor_iterator(self):
        imlist = cplcore.ImageList(cplcore.Image([[i, i + 1]]) for i in range(3))

        assert len(imlist) == 3
        for i in range(3):
            assert imlist[i][0][0] == i
            assert imlist[i][0][1] == i + 1

    def test_constructor_numpy(self):
        import numpy as np

        arr1 = np.array([[5, 6, 7]])
        arr2 = np.array([[2, 3, 4]])
        arr3 = np.array([[1, 2, 3]])
        imlist = cplcore.ImageList([arr1, arr2, arr3])
        assert len(imlist) == 3
        assert imlist[0][0][0] == 5
        assert imlist[0][0][1] == 6
        assert imlist[0][0][2] == 7
        assert imlist[1][0][0] == 2
        assert imlist[1][0][1] == 3
        assert imlist[1][0][2] == 4
        assert imlist[2][0][0] == 1
        assert imlist[2][0][1] == 2
        assert imlist[2][0][2] == 3

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
        ],
    )
    def test_constructor_load_native(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type
    ):
        from astropy.io import fits
        import numpy as np

        mock_image1 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image2 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image3 = make_mock_image(dtype=pixel_type, width=256, height=512)

        mock_image_list = np.array([mock_image1, mock_image2, mock_image3])

        # Write the image list to a FITS file
        my_fits_filename = make_mock_fits(
            fits.HDUList([fits.PrimaryHDU(mock_image_list)])
        )

        new_image_list = cplcore.ImageList.load(
            my_fits_filename, dtype=cplcore.Type.UNSPECIFIED
        )

        assert len(new_image_list) == mock_image_list.shape[0]
        assert new_image_list[0].shape == mock_image1.shape
        assert new_image_list[1].shape == mock_image2.shape
        assert new_image_list[2].shape == mock_image3.shape
        assert new_image_list[0].type == cpl_typeid
        assert new_image_list[1].type == cpl_typeid
        assert new_image_list[2].type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image list takes too long
        assert new_image_list[0][0][0] == mock_image_list[0][0][0]
        assert new_image_list[1][61][126] == mock_image_list[1][61][126]
        assert new_image_list[0][172][91] == mock_image_list[0][172][91]
        assert new_image_list[1][482][146] == mock_image_list[1][482][146]
        assert new_image_list[0][49][211] == mock_image_list[0][49][211]
        assert new_image_list[2][46][142] == mock_image_list[2][46][142]
        assert new_image_list[2][44][41] == mock_image_list[2][44][41]
        assert new_image_list[0][393][186] == mock_image_list[0][393][186]
        assert new_image_list[1][184][235] == mock_image_list[1][184][235]
        assert new_image_list[0][466][40] == mock_image_list[0][466][40]
        assert new_image_list[2][130][93] == mock_image_list[2][130][93]
        assert new_image_list[0][511][255] == mock_image_list[0][511][255]

    @pytest.mark.parametrize("pixel_type", [np.intc, np.single, np.double])
    def test_constructor_load_default(
        self, make_mock_image, make_mock_fits, pixel_type
    ):
        from astropy.io import fits
        import numpy as np

        mock_image1 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image2 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image3 = make_mock_image(dtype=pixel_type, width=256, height=512)

        mock_image_list = np.array([mock_image1, mock_image2, mock_image3])

        # Write the image list to a FITS file
        my_fits_filename = make_mock_fits(
            fits.HDUList([fits.PrimaryHDU(mock_image_list)])
        )

        new_image_list = cplcore.ImageList.load(my_fits_filename)

        assert len(new_image_list) == mock_image_list.shape[0]
        assert new_image_list[0].shape == mock_image1.shape
        assert new_image_list[1].shape == mock_image2.shape
        assert new_image_list[2].shape == mock_image3.shape
        assert new_image_list[0].type == cplcore.Type.DOUBLE
        assert new_image_list[1].type == cplcore.Type.DOUBLE
        assert new_image_list[2].type == cplcore.Type.DOUBLE

        # Just check some random indicies
        # Checking the whole image list takes too long
        assert new_image_list[0][0][0] == mock_image_list[0][0][0]
        assert new_image_list[1][61][126] == mock_image_list[1][61][126]
        assert new_image_list[0][172][91] == mock_image_list[0][172][91]
        assert new_image_list[1][482][146] == mock_image_list[1][482][146]
        assert new_image_list[0][49][211] == mock_image_list[0][49][211]
        assert new_image_list[2][46][142] == mock_image_list[2][46][142]
        assert new_image_list[2][44][41] == mock_image_list[2][44][41]
        assert new_image_list[0][393][186] == mock_image_list[0][393][186]
        assert new_image_list[1][184][235] == mock_image_list[1][184][235]
        assert new_image_list[0][466][40] == mock_image_list[0][466][40]
        assert new_image_list[2][130][93] == mock_image_list[2][130][93]
        assert new_image_list[0][511][255] == mock_image_list[0][511][255]

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
        ],
    )
    def test_constructor_load_type(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type
    ):
        from astropy.io import fits
        import numpy as np

        mock_image1 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image2 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image3 = make_mock_image(dtype=pixel_type, width=256, height=512)

        mock_image_list = np.array([mock_image1, mock_image2, mock_image3])

        # Write the image list to a FITS file
        my_fits_filename = make_mock_fits(
            fits.HDUList([fits.PrimaryHDU(mock_image_list)])
        )

        new_image_list = cplcore.ImageList.load(my_fits_filename, dtype=cpl_typeid)

        assert len(new_image_list) == mock_image_list.shape[0]
        assert new_image_list[0].shape == mock_image1.shape
        assert new_image_list[1].shape == mock_image2.shape
        assert new_image_list[2].shape == mock_image3.shape
        assert new_image_list[0].type == cpl_typeid
        assert new_image_list[1].type == cpl_typeid
        assert new_image_list[2].type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image list takes too long
        assert new_image_list[0][0][0] == mock_image_list[0][0][0]
        assert new_image_list[1][61][126] == mock_image_list[1][61][126]
        assert new_image_list[0][172][91] == mock_image_list[0][172][91]
        assert new_image_list[1][482][146] == mock_image_list[1][482][146]
        assert new_image_list[0][49][211] == mock_image_list[0][49][211]
        assert new_image_list[2][46][142] == mock_image_list[2][46][142]
        assert new_image_list[2][44][41] == mock_image_list[2][44][41]
        assert new_image_list[0][393][186] == mock_image_list[0][393][186]
        assert new_image_list[1][184][235] == mock_image_list[1][184][235]
        assert new_image_list[0][466][40] == mock_image_list[0][466][40]
        assert new_image_list[2][130][93] == mock_image_list[2][130][93]
        assert new_image_list[0][511][255] == mock_image_list[0][511][255]

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type, tiny",
        [
            (cplcore.Type.DOUBLE, np.intc, np.finfo(np.double).eps),
            (cplcore.Type.INT, np.single, 1),
            (cplcore.Type.INT, np.double, 1),
        ],
    )
    def test_constructor_load_cast(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type, tiny
    ):
        from astropy.io import fits
        import numpy as np

        mock_image1 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image2 = make_mock_image(dtype=pixel_type, width=256, height=512)
        mock_image3 = make_mock_image(dtype=pixel_type, width=256, height=512)

        mock_image_list = np.array([mock_image1, mock_image2, mock_image3])

        # Write the image list to a FITS file
        my_fits_filename = make_mock_fits(
            fits.HDUList([fits.PrimaryHDU(mock_image_list)])
        )

        new_image_list = cplcore.ImageList.load(my_fits_filename, dtype=cpl_typeid)

        assert len(new_image_list) == mock_image_list.shape[0]
        assert new_image_list[0].shape == mock_image1.shape
        assert new_image_list[1].shape == mock_image2.shape
        assert new_image_list[2].shape == mock_image3.shape
        assert new_image_list[0].type == cpl_typeid
        assert new_image_list[1].type == cpl_typeid
        assert new_image_list[2].type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image list takes too long
        assert np.abs(new_image_list[0][0][0] - mock_image_list[0][0][0]) < tiny
        assert np.abs(new_image_list[1][61][126] - mock_image_list[1][61][126]) < tiny
        assert np.abs(new_image_list[0][172][91] - mock_image_list[0][172][91]) < tiny
        assert np.abs(new_image_list[1][482][146] - mock_image_list[1][482][146]) < tiny
        assert np.abs(new_image_list[0][49][211] - mock_image_list[0][49][211]) < tiny
        assert np.abs(new_image_list[2][46][142] - mock_image_list[2][46][142]) < tiny
        assert np.abs(new_image_list[2][44][41] - mock_image_list[2][44][41]) < tiny
        assert np.abs(new_image_list[0][393][186] - mock_image_list[0][393][186]) < tiny
        assert np.abs(new_image_list[1][184][235] - mock_image_list[1][184][235]) < tiny
        assert np.abs(new_image_list[0][466][40] - mock_image_list[0][466][40]) < tiny
        assert np.abs(new_image_list[2][130][93] - mock_image_list[2][130][93]) < tiny
        assert np.abs(new_image_list[0][511][255] - mock_image_list[0][511][255]) < tiny

    def test_constructor_load_window(self, make_mock_image, make_mock_fits):
        from astropy.io import fits
        import numpy as np

        mock_image1 = np.block(
            [
                [
                    np.array([1] * 9, dtype=np.double).reshape(3, 3),
                    np.array([2] * 9, dtype=np.double).reshape(3, 3),
                ],
                [
                    np.array([3] * 9, dtype=np.double).reshape(3, 3),
                    np.array([4] * 9, dtype=np.double).reshape(3, 3),
                ],
            ]
        )
        mock_image2 = np.block(
            [
                [
                    np.array([1] * 9, dtype=np.double).reshape(3, 3),
                    np.array([2] * 9, dtype=np.double).reshape(3, 3),
                ],
                [
                    np.array([3] * 9, dtype=np.double).reshape(3, 3),
                    np.array([4] * 9, dtype=np.double).reshape(3, 3),
                ],
            ]
        )
        mock_image3 = np.block(
            [
                [
                    np.array([1] * 9, dtype=np.double).reshape(3, 3),
                    np.array([2] * 9, dtype=np.double).reshape(3, 3),
                ],
                [
                    np.array([3] * 9, dtype=np.double).reshape(3, 3),
                    np.array([4] * 9, dtype=np.double).reshape(3, 3),
                ],
            ]
        )

        mock_image_list = np.array([mock_image1, mock_image2, mock_image3])

        # Write the image list to a FITS file
        my_fits_filename = make_mock_fits(
            fits.HDUList([fits.PrimaryHDU(mock_image_list)])
        )

        new_image_list = cplcore.ImageList.load(
            my_fits_filename, dtype=cplcore.Type.DOUBLE, area=(0, 0, 2, 2)
        )

        # Check loaded image structure and type
        assert len(new_image_list) == mock_image_list.shape[0]
        assert new_image_list[0].shape == (3, 3)
        assert new_image_list[1].shape == (3, 3)
        assert new_image_list[2].shape == (3, 3)
        assert new_image_list[0].type == cplcore.Type.DOUBLE
        assert new_image_list[1].type == cplcore.Type.DOUBLE
        assert new_image_list[2].type == cplcore.Type.DOUBLE

        # Verify expected mean value of the loaded block
        assert np.abs(np.mean(new_image_list) - 1.0) < np.finfo(np.double).eps

    def test_empty(self):
        imlist = cplcore.ImageList()

        with pytest.raises(IndexError):
            imlist[0]

    def test_append_one(self):
        imlist = cplcore.ImageList()
        im = cplcore.Image([[0, 1]])
        imlist.append(im)
        assert len(imlist) == 1
        assert im in imlist

    def test_append_two(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[0, 1]])
        img2 = cplcore.Image([[1, 2]])

        imlist.append(img1)
        imlist.append(img2)

        assert len(imlist) == 2

        assert imlist[0] == img1
        assert imlist[1] == img2

        assert img1 in imlist
        assert img2 in imlist

    def test_insert_position(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])

        imlist.append(img1)  # [prop1]
        imlist.insert(0, img2)  # [prop2, prop1]

        imlist.insert(2, img3)  # [prop2, prop1, prop3]

        assert len(imlist) == 3
        assert imlist[0] == img2
        assert imlist[1] == img1
        assert imlist[2] == img3

    def test_insert_position_notfound(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])

        with pytest.raises(IndexError):
            imlist.insert(-1, img1)

        with pytest.raises(IndexError):
            imlist.insert(1, img2)

    def test_set_position(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])

        imlist.append(img1)
        imlist.append(img2)

        imlist[0] = img3

        assert len(imlist) == 2
        assert imlist[0] == img3
        assert imlist[1] == img2

    def test_delete_position(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])

        imlist.append(img1)
        imlist.append(img2)
        imlist.append(img3)

        del imlist[1]

        assert len(imlist) == 2
        assert imlist[0] == img1
        assert imlist[1] == img3

    def test_delete_position_notfound(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])

        imlist.append(img1)

        with pytest.raises(IndexError):
            del imlist[1]

    def test_delete_empty(self):
        imlist = cplcore.ImageList()

        with pytest.raises(IndexError):
            del imlist[0]

    def test_save(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop1.value = 20
        plist1.append(prop1)
        imlist.append(img1)
        imlist.append(img2)
        imlist.save(
            "test_image.fits", plist1, cplcore.io.CREATE, dtype=cplcore.Type.INT
        )
        loaded = cplcore.ImageList.load("test_image.fits", cplcore.Type.INT, 0)
        assert loaded[0][0][0] == 1
        assert loaded[0].shape == (1, 3)
        assert loaded[0].type == cplcore.Type.INT
        assert loaded[1][0][0] == 2
        assert loaded[1].shape == (1, 3)
        assert loaded[1].type == cplcore.Type.INT
        img3 = cplcore.Image([[5.5, 6.6, 7.7], [6.7, 8.9, 9.1]])
        imlist2 = cplcore.ImageList()
        imlist2.append(img3)
        imlist2.save(
            "test_image.fits", plist1, cplcore.io.EXTEND, dtype=cplcore.Type.DOUBLE
        )
        loaded = cplcore.ImageList.load("test_image.fits", cplcore.Type.DOUBLE, 1)
        assert loaded[0][0][0] == 5.5
        assert loaded[0][1][0] == 6.7
        assert loaded[0].shape == (2, 3)
        assert loaded[0].type == cplcore.Type.DOUBLE
        os.remove("test_image.fits")

    def test_add_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = -4092
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)

        imlist.add_scalar(-1982)
        assert imlist[0][0][0] == 8912 + -1982
        assert imlist[1][0][0] == -4092 + -1982

    def test_multiply_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = -4092
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.multiply_scalar(-1982)
        assert imlist[0][0][0] == 8912 * -1982
        assert imlist[1][0][0] == -4092 * -1982

    def test_subtract_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = -4092
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.subtract_scalar(-1982)
        assert imlist[0][0][0] == 8912 - -1982
        assert imlist[1][0][0] == -4092 - -1982

    def test_divide_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = -4092
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.divide_scalar(-1982)
        assert imlist[0][0][0] == 8912 / -1982
        assert imlist[1][0][0] == -4092 / -1982

    def test_add_image(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = -4092
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.divide_scalar(-1982)
        assert imlist[0][0][0] == 8912 / -1982
        assert imlist[1][0][0] == -4092 / -1982

    def test_add(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist1 = cplcore.ImageList()
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist2 = cplcore.ImageList()
        img1[0][0] = 8912
        img2[0][0] = -4092
        imlist1.append(img1)
        imlist2.append(img2)
        imlist1.add(imlist2)

        assert imlist1[0][0][0] == 8912 + -4092

    def test_subtract(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist1 = cplcore.ImageList()
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist2 = cplcore.ImageList()
        img1[0][0] = 8912
        img2[0][0] = -4092
        imlist1.append(img1)
        imlist2.append(img2)
        imlist1.subtract(imlist2)

        assert imlist1[0][0][0] == 8912 - -4092

    def test_multiply(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist1 = cplcore.ImageList()
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist2 = cplcore.ImageList()
        img1[0][0] = 8912
        img2[0][0] = -4092
        imlist1.append(img1)
        imlist2.append(img2)
        imlist1.multiply(imlist2)

        assert imlist1[0][0][0] == 8912 * -4092

    def test_divide(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist1 = cplcore.ImageList()
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        imlist2 = cplcore.ImageList()
        img1[0][0] = 8912
        img2[0][0] = -4092
        imlist1.append(img1)
        imlist2.append(img2)
        imlist1.divide(imlist2)

        assert imlist1[0][0][0] == 8912 / -4092

    def test_add_image2(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img_add = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2[0][0] = -4092
        img_add[0][0] = -1982

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.add_image(img_add)

        assert imlist[0][0][0] == 8912 + -1982
        assert imlist[1][0][0] == -4092 + -1982

    def test_subtract_image(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img_sub = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2[0][0] = -4092
        img_sub[0][0] = -1982

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.subtract_image(img_sub)

        assert imlist[0][0][0] == 8912 - -1982
        assert imlist[1][0][0] == -4092 - -1982

    def test_multiply_image(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img_mul = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2[0][0] = -4092
        img_mul[0][0] = -1982

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.multiply_image(img_mul)

        assert imlist[0][0][0] == 8912 * -1982
        assert imlist[1][0][0] == -4092 * -1982

    def test_divide_image(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img_div = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912
        img2[0][0] = -4092
        img_div[0][0] = -1982

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.divide_image(img_div)

        assert imlist[0][0][0] == 8912 / -1982
        assert imlist[1][0][0] == -4092 / -1982

    def test_exponential(self):
        from math import isclose

        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 5 / 2

        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = 60 / -5

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)

        imlist.exponential(91)
        assert isclose(imlist[0][0][0], pow(91, 5 / 2), rel_tol=1e-5)
        assert isclose(imlist[1][0][0], pow(91, 60 / -5), rel_tol=1e-5)

    def test_astype(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 5.6

        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2[0][0] = 60.2

        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        castedList = imlist.astype(cplcore.Type.INT)
        assert castedList[0].type == cplcore.Type.INT
        assert castedList[0][0][0] == 5
        assert castedList[1].type == cplcore.Type.INT
        assert castedList[1][0][0] == 60

    def test_empty2(self):
        img1 = cplcore.Image([[20, 40, 70]])
        img2 = cplcore.Image([[10, 50, 80]])
        img3 = cplcore.Image([[30, 60, 90]])
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.append(img3)
        imlist.empty()
        assert len(imlist) == 0
        with pytest.raises(IndexError):
            imlist[0]

    def test_collapse(self):
        img1 = cplcore.Image([[50, 60, 70], [10, 20, 30]])
        img2 = cplcore.Image([[20, 30, 40], [40, 50, 60]])
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        im_res = imlist.collapse_create()
        assert im_res == cplcore.Image([[35, 45, 55], [25, 35, 45]])

    def test_threshold(self):
        img1 = cplcore.Image([[-3027, 50012, 32], [364, -20412, -2]])
        img2 = cplcore.Image([[20, 30, 40], [40, 50, 60]])
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.threshold(-50, 50, -1, 1)
        assert imlist[0] == cplcore.Image([[-1, 1, 32], [1, -1, -2]])
        assert imlist[1] == cplcore.Image([[20, 30, 40], [40, 50, 1]])

    def test_collapse_minmax(self):
        img1 = cplcore.Image([[20, 40, 70]])
        img2 = cplcore.Image([[10, 50, 80]])
        img3 = cplcore.Image([[30, 60, 90]])
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.append(img3)
        im_res = imlist.collapse_minmax_create(1, 1)
        assert im_res == cplcore.Image([[20, 50, 80]])

    def test_swap_axis(self, cpl_image_fill_test_create):
        img1 = cpl_image_fill_test_create(10, 2 * 10)
        imlist1 = cplcore.ImageList()
        for i in range(30):
            imlist1.append(img1.duplicate())

        imlist1[0].reject(0, 0)
        imlist1[1].reject(1, 1)
        imlist1[2].reject(4, 1)
        imlist2 = imlist1.swap_axis_create(cplcore.ImageList.SwapAxis.XZ)
        assert imlist2[0].count_rejected() == 1
        assert imlist2[1].count_rejected() == 2
        assert imlist2[2].count_rejected() == 0
        imlist2 = imlist1.swap_axis_create(cplcore.ImageList.SwapAxis.YZ)
        assert imlist2[0].count_rejected() == 1
        assert imlist2[1].count_rejected() == 1
        assert imlist2[2].count_rejected() == 0
        assert imlist2[4].count_rejected() == 1

    def test_median(self):
        from statistics import median

        img1 = cplcore.Image([[20, 40, 70]])
        img2 = cplcore.Image([[10, 50, 80]])
        img3 = cplcore.Image([[30, 60, 90]])
        imlist = cplcore.ImageList()
        imlist.append(img1)
        imlist.append(img2)
        imlist.append(img3)
        im_res = imlist.collapse_median_create()
        assert im_res[0][0] == median([20, 10, 30])
        assert im_res[0][1] == median([40, 50, 60])
        assert im_res[0][2] == median([70, 80, 90])

    def test_from_accepted(self):
        imlist = cplcore.ImageList()
        img1 = cplcore.Image([[20, 40, 70]])
        img2 = cplcore.Image([[10, 50, 80]])
        img3 = cplcore.Image([[30, 60, 90]])
        # Set so that first col has 1 bad pixel, second has 2, third has 3
        img1.reject(0, 0)
        img2.reject(0, 0)
        img2.reject(0, 1)
        img3.reject(0, 0)
        img3.reject(0, 1)
        img3.reject(0, 2)

        imlist.append(img1)
        imlist.append(img2)
        imlist.append(img3)

        res = cplcore.Image.from_accepted(imlist)

        assert res[0][0] == 0
        assert res[0][1] == 1
        assert res[0][2] == 2

    @pytest.mark.parametrize(
        "clip_mode",
        [
            cplcore.ImageList.Collapse.MEAN,
            cplcore.ImageList.Collapse.MEDIAN,
            cplcore.ImageList.Collapse.MEDIAN_MEAN,
        ],
    )
    @pytest.mark.parametrize(
        "pixel_type", [cplcore.Type.INT, cplcore.Type.FLOAT, cplcore.Type.DOUBLE]
    )
    @pytest.mark.parametrize("size", [3, 5, 8])
    def test_collapse_sigclip(self, clip_mode, pixel_type, size):
        import numpy as np
        from math import isclose

        DBL_EPSILON = np.finfo(np.float64).eps
        FLT_EPSILON = np.finfo(np.float32).eps
        # recreate tests up until the cpl_imagelist_collapse_sigclip_create_test_one call (and the whole function
        # itself) from l129 in cpl_imagelist_basic-test.c
        imlist1 = cplcore.ImageList()
        for i in range(size):
            iimg = cplcore.Image.create_noise_uniform(10, 10, pixel_type, -100, 200)
            imlist1.append(iimg)
        # Start of cpl_imagelist_collapse_sigclip_create_test_one

        nsize = len(imlist1)
        nx = imlist1[0].width
        ny = imlist1[0].height
        im_map = cplcore.Image.zeros(nx, ny, cplcore.Type.INT)
        jkeep = nsize
        for ikeep in reversed(range(1, nsize + 1)):
            keepfrac = (ikeep if ikeep == nsize else ikeep + 0.5) / nsize
            clipped, im_map = imlist1.collapse_sigclip_create(
                0.5, 1.5, keepfrac, clip_mode
            )
            assert clipped.type == pixel_type
            assert clipped.width == nx
            assert clipped.height == ny
            # Commented out lines don't seem to have any relevance to function we're testing
            contrib = cplcore.Image.from_accepted(imlist1)

            minpix = contrib.get_min()
            maxpix = contrib.get_max()
            maxbad = nsize - minpix
            bpm = cplcore.Mask(im_map, -0.5, 0.5)
            if im_map.get_min == 0:
                assert bpm == clipped
            else:
                assert bpm.count() == 0
                assert max(1, ikeep - maxbad) <= im_map.get_min()
            assert im_map.get_max() <= min(jkeep, maxpix)
            jkeep = im_map.get_max()
            if ikeep == nsize:
                average = imlist1.collapse_create()

                # The clipped image is not (much) different from the
                # averaged one - except for integer pixels, where the
                # average is computed using integer arithmetics,
                # while the clipped uses floating point
                tolerance = (
                    1
                    if pixel_type == cplcore.Type.INT
                    else 150
                    * (
                        DBL_EPSILON
                        if pixel_type == cplcore.Type.DOUBLE
                        else FLT_EPSILON
                    )
                )
                assert average.width == clipped.width
                assert average.height == clipped.height
                for y in range(average.height):
                    average_row = average[y]
                    clipped_row = clipped[y]
                    for x in range(average.width):
                        assert isclose(
                            average_row[x], clipped_row[x], abs_tol=tolerance
                        )

    def test_is_uniform(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist = cplcore.ImageList([img2, img1, img3])

        imlist.is_uniform()
        assert imlist.is_uniform()

        imlist1 = cplcore.ImageList()
        with pytest.raises(cplcore.IllegalInputError):
            imlist1.is_uniform()
