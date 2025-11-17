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

from contextlib import contextmanager
import os
from pathlib import Path

import numpy as np
import pytest

import cpl


@contextmanager
def set_directory(path: Path):
    """Sets the cwd within the context

    Args:
        path (Path): The path to change the working directory to

    Yields:
        None
    """
    origin = Path().cwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(origin)


class TestDFS:
    @pytest.fixture(scope="function")
    def dfs_path(self, tmp_path_factory):
        dfs_path = tmp_path_factory.mktemp("test_dfs")
        return dfs_path

    # First basic test
    def test_sign_products(self, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fs = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT),
                cpl.ui.Frame(fits_filename2),
            ]
        )
        cpl.dfs.sign_products(fs)
        pl1 = cpl.core.PropertyList.load(fits_filename1, 0)
        pl2 = cpl.core.PropertyList.load(fits_filename2, 0)
        added_properties = ["PIPEFILE", "DATAMD5", "CHECKSUM", "DATASUM"]
        for prop in added_properties:
            assert prop in pl1
            assert prop not in pl2

    def test_sign_products_from_frameset(self, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fs = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT),
                cpl.ui.Frame(fits_filename2),
            ]
        )
        fs.sign_products()
        pl1 = cpl.core.PropertyList.load(fits_filename1, 0)
        pl2 = cpl.core.PropertyList.load(fits_filename2, 0)
        added_properties = ["PIPEFILE", "DATAMD5", "CHECKSUM", "DATASUM"]
        for prop in added_properties:
            assert prop in pl1
            assert prop not in pl2

    def test_save_propertylist_no_regex(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag")]
        )
        proplist_path_str = str(dfs_path / "saved_proplist.fits")
        with set_directory(Path(dfs_path)):
            cpl.dfs.save_propertylist(
                allframes,
                parlist,
                usedframes,
                "recipe",
                applist,
                "pipe_id",
                "saved_proplist.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
            )
            loaded = cpl.core.PropertyList.load(proplist_path_str, 0)
            assert loaded["ESO PRO CATG"].value == "category_tag"
            assert loaded["SIMPLE"].value is True
            assert loaded["ESO PRO REC1 ID"].value == "recipe"
            assert loaded["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded["ESO PRO DATANCOM"].value == 2
            assert loaded["PIPEFILE"].value == "saved_proplist.fits"

    def test_save_propertylist_with_regex(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"
        ".* NOTINCLUDED .*"
        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [
                cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property("PROPTEST", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property(
                    "ESO NOTINCLUDED TEST", cpl.core.Type.STRING, "category_tag"
                ),
                cpl.core.Property("ALSO NOTINCLUDED WOW", cpl.core.Type.DOUBLE, 256.5),
            ]
        )
        proplist_path_str = str(dfs_path / "saved_proplist.fits")
        with set_directory(Path(dfs_path)):
            cpl.dfs.save_propertylist(
                allframes,
                parlist,
                usedframes,
                "recipe",
                applist,
                "pipe_id",
                "saved_proplist.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
                ".* NOTINCLUDED .*",
            )
            loaded = cpl.core.PropertyList.load(proplist_path_str, 0)
            assert "PROPTEST" in loaded
            assert "ESO NOTINCLUDED TEST" not in loaded
            assert "ALSO NOTINCLUDED WOW" not in loaded
            assert loaded["ESO PRO CATG"].value == "category_tag"
            assert loaded["SIMPLE"].value is True
            assert loaded["ESO PRO REC1 ID"].value == "recipe"
            assert loaded["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded["ESO PRO DATANCOM"].value == 2
            assert loaded["PIPEFILE"].value == "saved_proplist.fits"

    def test_update_product_header(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        product_frame = cpl.ui.Frame(
            fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT, tag="category_tag"
        )
        framelist = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1, group=cpl.ui.Frame.FrameGroup.RAW)]
        )

        parlist = cpl.ui.ParameterList()

        header = cpl.core.PropertyList()

        cpl.dfs.setup_product_header(
            header,
            product_frame,
            framelist,
            parlist,
            "recid",
            "pipeline_id",
            "dictionaryid",
        )  # Keywords need to be setup first
        assert header["DATAMD5"].value == "Not computed"
        assert header["ESO PRO CATG"].value == "category_tag"
        assert "SIMPLE" not in header
        assert "NAXIS" not in header
        header_path_str = str(dfs_path / "product.fits")
        header.save(header_path_str, cpl.core.io.CREATE)

        fs = cpl.ui.FrameSet(
            [cpl.ui.Frame(header_path_str, group=cpl.ui.Frame.FrameGroup.PRODUCT)]
        )
        cpl.dfs.update_product_header(fs)
        updated = cpl.core.PropertyList.load(header_path_str, 0)
        # Check if checksum was updated
        assert updated["DATAMD5"].value != "Not computed"
        assert updated["SIMPLE"].value is True
        assert updated["NAXIS"].value == 0
        # Direct compare fine here as it gets the string directly from the frame
        assert header["PIPEFILE"].value == product_frame.file

    def test_update_product_header_from_frameset(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        fits_path = Path(fits_filename1)
        with set_directory(fits_path.parent):
            product_frame = cpl.ui.Frame(
                fits_path.name,
                group=cpl.ui.Frame.FrameGroup.PRODUCT,
                tag="category_tag",
            )
            framelist = cpl.ui.FrameSet(
                [cpl.ui.Frame(fits_path.name, group=cpl.ui.Frame.FrameGroup.RAW)]
            )

            parlist = cpl.ui.ParameterList()

            header = cpl.core.PropertyList()

            cpl.dfs.setup_product_header(
                header,
                product_frame,
                framelist,
                parlist,
                "recid",
                "pipeline_id",
                "dictionaryid",
            )  # Keywords need to be setup first
            assert header["DATAMD5"].value == "Not computed"
            assert header["ESO PRO CATG"].value == "category_tag"
            assert "SIMPLE" not in header
            assert "NAXIS" not in header
            with set_directory(dfs_path):
                header.save("product.fits", cpl.core.io.CREATE)
                # Direct compare fine here as it gets the string directly from the frame
                assert header["PIPEFILE"].value == fits_path.name

                fs = cpl.ui.FrameSet(
                    [
                        cpl.ui.Frame(
                            "product.fits", group=cpl.ui.Frame.FrameGroup.PRODUCT
                        )
                    ]
                )
                fs.update_product_header()
                updated = cpl.core.PropertyList.load("product.fits", 0)
                # Check if checksum was updated
                assert updated["DATAMD5"].value != "Not computed"
                assert updated["SIMPLE"].value is True
                assert updated["NAXIS"].value == 0
                assert updated["PIPEFILE"].value == "product.fits"

    def test_save_image_no_regex(self, make_mock_image, make_mock_fits, dfs_path):
        mock_image = make_mock_image(
            dtype=np.intc, min=0, max=255, width=256, height=512
        )

        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()

        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag")]
        )
        image_path_str = str(dfs_path / "saved_image.fits")
        with set_directory(dfs_path):
            cpl.dfs.save_image(
                allframes,
                parlist,
                usedframes,
                cpl.core.Image(mock_image),
                "recipe",
                applist,
                "pipe_id",
                "saved_image.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
            )
            loaded_image = cpl.core.Image.load(
                image_path_str, dtype=cpl.core.Type.INT, extension=0
            )
            loaded_header = cpl.core.PropertyList.load(image_path_str, 0)
            # Test header
            assert loaded_header["ESO PRO CATG"].value == "category_tag"
            assert loaded_header["SIMPLE"].value is True
            assert loaded_header["NAXIS"].value == 2
            assert loaded_header["ESO PRO REC1 ID"].value == "recipe"
            assert loaded_header["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded_header["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded_header["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded_header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded_header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded_header["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded_header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded_header["ESO PRO DATANCOM"].value == 2
            assert loaded_header["PIPEFILE"].value == "saved_image.fits"
            # Check if image is the same
            assert np.array_equal(loaded_image, mock_image)

    def test_save_image_with_regex(self, make_mock_image, make_mock_fits, dfs_path):
        mock_image = make_mock_image(
            dtype=np.intc, min=0, max=255, width=256, height=512
        )
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"
        ".* NOTINCLUDED .*"
        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [
                cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property("PROPTEST", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property(
                    "ESO NOTINCLUDED TEST", cpl.core.Type.STRING, "category_tag"
                ),
                cpl.core.Property("ALSO NOTINCLUDED WOW", cpl.core.Type.DOUBLE, 256.5),
            ]
        )

        image_path_str = str(dfs_path / "saved_image.fits")
        with set_directory(dfs_path):
            cpl.dfs.save_image(
                allframes,
                parlist,
                usedframes,
                cpl.core.Image(mock_image),
                "recipe",
                applist,
                "pipe_id",
                "saved_image.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
                ".* NOTINCLUDED .*",
            )
            loaded_image = cpl.core.Image.load(
                image_path_str, dtype=cpl.core.Type.INT, extension=0
            )
            loaded_header = cpl.core.PropertyList.load(image_path_str, 0)
            # Test header
            assert "PROPTEST" in loaded_header
            assert "ESO NOTINCLUDED TEST" not in loaded_header
            assert "ALSO NOTINCLUDED WOW" not in loaded_header
            assert loaded_header["ESO PRO CATG"].value == "category_tag"
            assert loaded_header["SIMPLE"].value is True
            assert loaded_header["NAXIS"].value == 2
            assert loaded_header["ESO PRO REC1 ID"].value == "recipe"
            assert loaded_header["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded_header["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded_header["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded_header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded_header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded_header["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded_header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded_header["ESO PRO DATANCOM"].value == 2
            assert loaded_header["PIPEFILE"].value == "saved_image.fits"
            # Check if image is the same
            assert loaded_image.type == cpl.core.Type.INT
            assert np.array_equal(loaded_image, mock_image)

    def test_save_imagelist_no_regex(self, make_mock_image, make_mock_fits, dfs_path):
        img1 = cpl.core.Image([[1, 2, 3]])
        img2 = cpl.core.Image([[2, 3, 4]])
        img3 = cpl.core.Image([[5, 6, 7]])
        imlist = cpl.core.ImageList([img2, img1, img3])

        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag")]
        )
        imagelist_path_str = str(dfs_path / "saved_imagelist.fits")
        with set_directory(dfs_path):
            cpl.dfs.save_imagelist(
                allframes,
                parlist,
                usedframes,
                imlist,
                cpl.core.Type.DOUBLE,
                "recipe",
                applist,
                "pipe_id",
                "saved_imagelist.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
            )
            loaded_imagelist = cpl.core.ImageList.load(imagelist_path_str)
            loaded_header = cpl.core.PropertyList.load(imagelist_path_str, 0)
            # Test header
            assert loaded_header["ESO PRO CATG"].value == "category_tag"
            assert loaded_header["SIMPLE"].value is True
            assert loaded_header["NAXIS"].value == 3
            assert loaded_header["ESO PRO REC1 ID"].value == "recipe"
            assert loaded_header["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded_header["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded_header["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded_header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded_header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded_header["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded_header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded_header["ESO PRO DATANCOM"].value == 2
            assert loaded_header["PIPEFILE"].value == "saved_imagelist.fits"
            # Check if image is the same
            assert np.array_equal(loaded_imagelist[0], img2)
            assert np.array_equal(loaded_imagelist[1], img1)
            assert np.array_equal(loaded_imagelist[2], img3)

    def test_save_imagelist_with_regex(self, make_mock_image, make_mock_fits, dfs_path):
        img1 = cpl.core.Image([[1, 2, 3]])
        img2 = cpl.core.Image([[2, 3, 4]])
        img3 = cpl.core.Image([[5, 6, 7]])
        imlist = cpl.core.ImageList([img2, img1, img3])

        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"
        ".* NOTINCLUDED .*"
        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [
                cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property("PROPTEST", cpl.core.Type.STRING, "category_tag"),
                cpl.core.Property(
                    "ESO NOTINCLUDED TEST", cpl.core.Type.STRING, "category_tag"
                ),
                cpl.core.Property("ALSO NOTINCLUDED WOW", cpl.core.Type.DOUBLE, 256.5),
            ]
        )
        with set_directory(dfs_path):
            cpl.dfs.save_imagelist(
                allframes,
                parlist,
                usedframes,
                imlist,
                cpl.core.Type.DOUBLE,
                "recipe",
                applist,
                "pipe_id",
                "saved_imagelist.fits",
                applist,
                cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW),
                ".* NOTINCLUDED .*",
            )
            loaded_header = cpl.core.PropertyList.load("saved_imagelist.fits", 0)
            loaded_imagelist = cpl.core.ImageList.load("saved_imagelist.fits")

            assert "PROPTEST" in loaded_header
            assert "ESO NOTINCLUDED TEST" not in loaded_header
            assert "ALSO NOTINCLUDED WOW" not in loaded_header
            assert loaded_header["ESO PRO CATG"].value == "category_tag"
            assert loaded_header["SIMPLE"].value is True
            assert loaded_header["NAXIS"].value == 3
            assert loaded_header["ESO PRO REC1 ID"].value == "recipe"
            assert loaded_header["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded_header["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded_header["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded_header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded_header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded_header["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded_header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded_header["ESO PRO DATANCOM"].value == 2
            assert loaded_header["PIPEFILE"].value == "saved_imagelist.fits"
            # Check if image is the same
            assert np.array_equal(loaded_imagelist[0], img2)
            assert np.array_equal(loaded_imagelist[1], img1)
            assert np.array_equal(loaded_imagelist[2], img3)

    def test_save_table(self, make_mock_fits, dfs_path):
        pd = pytest.importorskip(
            "pandas", reason="pandas not installed, cannot run this test."
        )
        df = pd.DataFrame(
            {"a": [1, 2, 3, 4, 5, 6], "b": ["hi", "there", "ESO", "how", "are", "you"]}
        )
        table = cpl.core.Table(df)
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        allframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1), cpl.ui.Frame(fits_filename2)]
        )
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name="test2", description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [
                cpl.ui.Frame(
                    fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW, tag="test"
                ),
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        applist = cpl.core.PropertyList(
            [cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "category_tag")]
        )
        tablelist = cpl.core.PropertyList(
            [cpl.core.Property("PRIMARY KEY", cpl.core.Type.STRING, "a")]
        )
        with set_directory(dfs_path):
            cpl.dfs.save_table(
                allframes,
                parlist,
                usedframes,
                table,
                "recipe",
                applist,
                "pipe_id",
                "saved_table.fits",
                tablelist=tablelist,
            )
            # Table stored in index 1, after header
            loaded_table = cpl.core.Table.load("saved_table.fits", 1)
            loaded_header = cpl.core.PropertyList.load("saved_table.fits", 0)
            loaded_tablelist = cpl.core.PropertyList.load("saved_table.fits", 1)
            # Test header
            assert loaded_header["ESO PRO CATG"].value == "category_tag"
            assert loaded_header["SIMPLE"].value is True
            assert loaded_header["NAXIS"].value == 0
            assert loaded_header["ESO PRO REC1 ID"].value == "recipe"
            assert loaded_header["ESO PRO REC1 RAW1 CATG"].value == "test"
            assert loaded_header["ESO PRO REC1 RAW2 CATG"].value == ""
            assert loaded_header["ESO PRO REC1 PIPE ID"].value == "pipe_id"
            assert loaded_header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
            assert loaded_header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
            assert loaded_header["ESO PRO REC1 PARAM2 NAME"].value == "test2"
            assert loaded_header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
            assert loaded_header["ESO PRO DATANCOM"].value == 2
            assert loaded_tablelist["PRIMARY KEY"].value == "a"
            assert loaded_tablelist["NAXIS"].value == 2
            assert loaded_tablelist["NAXIS2"].value == 6
            assert loaded_tablelist["TFIELDS"].value == 2
            assert loaded_header["PIPEFILE"].value == "saved_table.fits"
            # Check if table is the same
            assert np.array_equal(loaded_table["a"], [1, 2, 3, 4, 5, 6])
            assert np.array_equal(
                loaded_table["b"], ["hi", "there", "ESO", "how", "are", "you"]
            )

    def test_update_product_header_no_setup(self, make_mock_fits):
        # If setup hasn't been established first.
        fits_filename1 = make_mock_fits()
        fs = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT)]
        )
        with pytest.raises(cpl.core.BadFileFormatError):
            cpl.dfs.update_product_header(fs)

    def test_setup_product_header_noinherit(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        product_frame = cpl.ui.Frame(
            fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT, tag="category_tag"
        )
        fits_filename2 = make_mock_fits()
        primary_proplist = cpl.core.PropertyList(
            [
                cpl.core.Property("EPOCH", cpl.core.Type.DOUBLE, 20.5),
                cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "FORS"),
            ]
        )
        proplist_path_str = str(dfs_path / "first.fits")
        primary_proplist.save(proplist_path_str, cpl.core.io.CREATE)

        first_frame = cpl.ui.Frame(proplist_path_str, group=cpl.ui.Frame.FrameGroup.RAW)

        framelist = cpl.ui.FrameSet(
            [
                first_frame,
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name=name, description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        header = cpl.core.PropertyList(
            [cpl.core.Property("EXISTING", cpl.core.Type.BOOL, True)]
        )
        original_size = len(header)
        cpl.dfs.setup_product_header(
            header,
            product_frame,
            framelist,
            parlist,
            "recid",
            "pipeline_id",
            "dictionaryid",
        )
        # Function did add keywords to property list
        assert len(header) > original_size
        assert header["ESO PRO CATG"].value == "category_tag"
        assert header["EPOCH"].value == 20.5
        assert header["EXISTING"].value is True
        assert header["ESO PRO DID"].value == "dictionaryid"
        assert header["ESO PRO REC1 PIPE ID"].value == "pipeline_id"
        assert header["ESO PRO REC1 ID"].value == "recid"
        assert header["PIPEFILE"].value == product_frame.file
        assert header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
        assert header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
        assert header["ESO PRO REC1 PARAM2 NAME"].value == name
        assert header["ESO PRO REC1 PARAM2 VALUE"].value == "229"

    def test_setup_product_header_withinherit(self, make_mock_fits, dfs_path):
        fits_filename1 = make_mock_fits()
        product_frame = cpl.ui.Frame(
            fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT
        )
        fits_filename2 = make_mock_fits()
        primary_proplist = cpl.core.PropertyList(
            [cpl.core.Property("EPOCH", cpl.core.Type.DOUBLE, 20.5)]
        )
        first_path_str = str(dfs_path / "first.fits")
        primary_proplist.save(first_path_str, cpl.core.io.CREATE)

        first_frame = cpl.ui.Frame(first_path_str, group=cpl.ui.Frame.FrameGroup.RAW)

        framelist = cpl.ui.FrameSet(
            [
                first_frame,
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name=name, description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW)]
        )
        header = cpl.core.PropertyList(
            [cpl.core.Property("EXISTING", cpl.core.Type.BOOL, True)]
        )
        original_size = len(header)

        inherited_props = cpl.core.PropertyList(
            [
                cpl.core.Property("HIERARCH ESO ORIGIN", cpl.core.Type.STRING, "STEAM"),
                cpl.core.Property(
                    "HIERARCH ESO FUN", cpl.core.Type.STRING, "NOTREDUCED"
                ),
            ]
        )
        inherited_path_str = str(dfs_path / "inherited.fits")
        inherited_props.save(inherited_path_str, cpl.core.io.CREATE)
        inherited_frame = cpl.ui.Frame(
            inherited_path_str, group=cpl.ui.Frame.FrameGroup.RAW
        )
        framelist.append(inherited_frame)
        usedframes.append(inherited_frame)
        cpl.dfs.setup_product_header(
            header,
            product_frame,
            framelist,
            parlist,
            "recid",
            "pipeline_id",
            "dictionaryid",
            inherited_frame,
        )
        # Function did add keywords to property list
        assert len(header) > original_size
        assert header["EPOCH"].value == 20.5
        assert header["EXISTING"].value is True
        assert header["ESO PRO DID"].value == "dictionaryid"
        assert header["ESO PRO REC1 PIPE ID"].value == "pipeline_id"
        assert header["ESO PRO REC1 ID"].value == "recid"
        assert header["PIPEFILE"].value == product_frame.file
        assert header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
        assert header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
        assert header["ESO PRO REC1 PARAM2 NAME"].value == name
        assert header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
        assert header["ESO ORIGIN"].value == "STEAM"
        assert header["ESO FUN"].value == "NOTREDUCED"

    def test_setup_product_header_noinherit_from_proplist(
        self, make_mock_fits, dfs_path
    ):
        fits_filename1 = make_mock_fits()
        product_frame = cpl.ui.Frame(
            fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT, tag="category_tag"
        )
        fits_filename2 = make_mock_fits()
        primary_proplist = cpl.core.PropertyList(
            [
                cpl.core.Property("EPOCH", cpl.core.Type.DOUBLE, 20.5),
                cpl.core.Property("ESO PRO CATG", cpl.core.Type.STRING, "FORS"),
            ]
        )
        first_path_str = str(dfs_path / "first.fits")
        primary_proplist.save(first_path_str, cpl.core.io.CREATE)

        first_frame = cpl.ui.Frame(first_path_str, group=cpl.ui.Frame.FrameGroup.RAW)

        framelist = cpl.ui.FrameSet(
            [
                first_frame,
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name=name, description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        header = cpl.core.PropertyList(
            [cpl.core.Property("EXISTING", cpl.core.Type.BOOL, True)]
        )
        original_size = len(header)
        header.setup_product_header(
            product_frame, framelist, parlist, "recid", "pipeline_id", "dictionaryid"
        )
        # Function did add keywords to property list
        assert len(header) > original_size
        assert header["ESO PRO CATG"].value == "category_tag"
        assert header["EPOCH"].value == 20.5
        assert header["EXISTING"].value is True
        assert header["ESO PRO DID"].value == "dictionaryid"
        assert header["ESO PRO REC1 PIPE ID"].value == "pipeline_id"
        assert header["ESO PRO REC1 ID"].value == "recid"
        assert header["PIPEFILE"].value == product_frame.file
        assert header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
        assert header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
        assert header["ESO PRO REC1 PARAM2 NAME"].value == name
        assert header["ESO PRO REC1 PARAM2 VALUE"].value == "229"

    def test_setup_product_header_withinherit_from_proplist(
        self, make_mock_fits, dfs_path
    ):
        fits_filename1 = make_mock_fits()
        product_frame = cpl.ui.Frame(
            fits_filename1, group=cpl.ui.Frame.FrameGroup.PRODUCT
        )
        fits_filename2 = make_mock_fits()
        primary_proplist = cpl.core.PropertyList(
            [cpl.core.Property("EPOCH", cpl.core.Type.DOUBLE, 20.5)]
        )
        first_path_str = str(dfs_path / "first.fits")
        primary_proplist.save(first_path_str, cpl.core.io.CREATE)

        first_frame = cpl.ui.Frame(first_path_str, group=cpl.ui.Frame.FrameGroup.RAW)

        framelist = cpl.ui.FrameSet(
            [
                first_frame,
                cpl.ui.Frame(fits_filename2, group=cpl.ui.Frame.FrameGroup.RAW),
            ]
        )
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cpl.ui.ParameterValue(
            name="test1", description=description, context=context, default=True
        )
        new_parameter2 = cpl.ui.ParameterValue(
            name=name, description=description, context=context, default=229
        )
        parlist = cpl.ui.ParameterList([new_parameter1, new_parameter2])
        fits_filename3 = make_mock_fits()
        usedframes = cpl.ui.FrameSet(
            [cpl.ui.Frame(fits_filename3, group=cpl.ui.Frame.FrameGroup.RAW)]
        )
        header = cpl.core.PropertyList(
            [cpl.core.Property("EXISTING", cpl.core.Type.BOOL, True)]
        )
        original_size = len(header)

        inherited_props = cpl.core.PropertyList(
            [
                cpl.core.Property("HIERARCH ESO ORIGIN", cpl.core.Type.STRING, "STEAM"),
                cpl.core.Property(
                    "HIERARCH ESO FUN", cpl.core.Type.STRING, "NOTREDUCED"
                ),
            ]
        )
        inherited_path_str = str(dfs_path / "inherited.fits")
        inherited_props.save(inherited_path_str, cpl.core.io.CREATE)
        inherited_frame = cpl.ui.Frame(
            inherited_path_str, group=cpl.ui.Frame.FrameGroup.RAW
        )
        framelist.append(inherited_frame)
        usedframes.append(inherited_frame)
        header.setup_product_header(
            product_frame,
            framelist,
            parlist,
            "recid",
            "pipeline_id",
            "dictionaryid",
            inherited_frame,
        )
        # Function did add keywords to property list
        assert len(header) > original_size
        assert header["EPOCH"].value == 20.5
        assert header["EXISTING"].value is True
        assert header["ESO PRO DID"].value == "dictionaryid"
        assert header["ESO PRO REC1 PIPE ID"].value == "pipeline_id"
        assert header["ESO PRO REC1 ID"].value == "recid"
        assert header["PIPEFILE"].value == product_frame.file
        assert header["ESO PRO REC1 PARAM1 NAME"].value == "test1"
        assert header["ESO PRO REC1 PARAM1 VALUE"].value == "true"
        assert header["ESO PRO REC1 PARAM2 NAME"].value == name
        assert header["ESO PRO REC1 PARAM2 VALUE"].value == "229"
        assert header["ESO ORIGIN"].value == "STEAM"
        assert header["ESO FUN"].value == "NOTREDUCED"

    def test_save_paf(self, dfs_path):
        paflist = cpl.core.PropertyList(
            [
                cpl.core.Property("DATE OBS", cpl.core.Type.STRING, "1/4/09"),
                cpl.core.Property("ESO DET DIT", cpl.core.Type.BOOL, True),
                cpl.core.Property("ARCFILE", cpl.core.Type.STRING, "test.file"),
                cpl.core.Property("TPL ESO ID", cpl.core.Type.STRING, "test.file"),
            ]
        )
        paf_path_str = str(dfs_path / "rrrecipe.paf")
        cpl.dfs.save_paf("IIINSTRUMENT", "rrrecipe", paflist, paf_path_str)
        lines = []
        with open(paf_path_str, "r") as paf_file:
            lines.extend(paf_file.readlines())
        assert lines  # lines should not be empty at this point
        # Expected results
        expected = {
            "PAF.ID": '"IIINSTRUMENT/rrrecipe"',
            "PAF.NAME": f'"{paf_path_str}"',
            "DATE.OBS": '"1/4/09"',
            "DET.DIT": 1,  # ESO prefixes are removed in the file
            "ARCFILE": '"test.file"',
            "TPL.ESO.ID": '"test.file"',
        }
        found = []
        for line in lines:
            for e in expected:
                if line.startswith(e):
                    found.append(e)
                    assert line.endswith(str(expected[e]) + "\n")
        found.sort()
        assert found == sorted(expected.keys())

    def test_save_paf_illegal_filename(self):
        paflist = cpl.core.PropertyList(
            [
                cpl.core.Property("PROPTEST1", cpl.core.Type.INT, 5),
                cpl.core.Property("PROPTEST2", cpl.core.Type.BOOL, True),
            ]
        )
        with pytest.raises(cpl.core.FileIOError):
            cpl.dfs.save_paf("IIINSTRUMENT", "rrrecipe", paflist, ".")
