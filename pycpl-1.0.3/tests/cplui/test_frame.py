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
from pathlib import Path
import shutil
from sys import platform
import subprocess

import pytest

from cpl import core as cplcore
from cpl import ui as cplui


class TestFrame:
    def test_contructor_sets_properties(self, make_mock_fits):
        fits_filename = make_mock_fits()
        single_frame = cplui.Frame(
            file=fits_filename,
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )

        assert Path(single_frame.file) == Path(fits_filename)
        assert single_frame.tag == "thetag"
        assert single_frame.group == cplui.Frame.FrameGroup.RAW
        assert single_frame.level == cplui.Frame.FrameLevel.INTERMEDIATE
        assert single_frame.type == cplui.Frame.FrameType.IMAGE

    def test_contructor_path_sets_properties(self, make_mock_fits):
        fits_filename = make_mock_fits()
        single_frame = cplui.Frame(
            file=Path(fits_filename),
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )

        assert Path(single_frame.file) == Path(fits_filename)
        assert single_frame.tag == "thetag"
        assert single_frame.group == cplui.Frame.FrameGroup.RAW
        assert single_frame.level == cplui.Frame.FrameLevel.INTERMEDIATE
        assert single_frame.type == cplui.Frame.FrameType.IMAGE

    @pytest.mark.skipif(
        platform == "darwin", reason="OSX not compatible with utf-8 encoding"
    )
    def test_constructor_not_unicode(self, make_mock_fits):
        # \x80\x81 is not valid UTF8
        fits_filename = os.fsdecode(
            bytes(str(Path.cwd()), encoding="utf-8") + bytes.fromhex("8081")
        )

        # Ensure the file exists
        ensure_file = open(fits_filename, "w+")
        ensure_file.close()
        try:
            single_frame = cplui.Frame(
                file=fits_filename,
                tag="thetag",
                group=cplui.Frame.FrameGroup.RAW,
                level=cplui.Frame.FrameLevel.INTERMEDIATE,
                frameType=cplui.Frame.FrameType.IMAGE,
            )

            assert os.path.exists(single_frame.file)
            assert fits_filename == single_frame.file
        finally:
            os.remove(fits_filename)

    def test_constructor_for_invalid_file(self, make_mock_fits):
        fits_filename = "NON-EXISTANT FILE"

        if os.path.exists(fits_filename):
            raise RuntimeError(
                "This unit test requires that NON-EXISTANT FILE not exist."
            )

        with pytest.raises(cplcore.FileIOError):
            _ = cplui.Frame(
                file=fits_filename,
                tag="thetag",
                group=cplui.Frame.FrameGroup.RAW,
                level=cplui.Frame.FrameLevel.INTERMEDIATE,
                frameType=cplui.Frame.FrameType.IMAGE,
            )

    def test_constructor_defaults(self, make_mock_fits):
        fits_filename = make_mock_fits()
        single_frame = cplui.Frame(file=fits_filename)

        assert Path(single_frame.file) == Path(fits_filename)
        assert single_frame.tag == ""
        assert single_frame.group == cplui.Frame.FrameGroup.NONE
        assert single_frame.level == cplui.Frame.FrameLevel.NONE
        assert single_frame.type == cplui.Frame.FrameType.NONE

    def test_file_not_found(self, tmp_path):
        non_existant_path = str(tmp_path.joinpath("non_existant"))

        with pytest.raises(cplcore.FileIOError) as excinfo:
            _ = cplui.Frame(non_existant_path)

        assert isinstance(excinfo.value, cplcore.FileIOError)
        # TODO: Once exception inheritance works, enable this assertion
        # assert isinstance(excinfo.value, FileNotFoundError)

    def test_same_file_equal(self, make_mock_fits):
        # 1 on-disk filename shared by 2 Frame objects
        file_name1 = make_mock_fits()
        single_frame1 = cplui.Frame(
            file=file_name1,
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )
        single_frame2 = cplui.Frame(
            file=file_name1,
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )
        assert single_frame1 == single_frame2

    def test_different_names_not_equal(self, make_mock_fits):
        # 2 (possibly) identical files except for filename:
        file_name1 = make_mock_fits()
        file_name2 = make_mock_fits()
        single_frame1 = cplui.Frame(
            file=file_name1,
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )
        single_frame2 = cplui.Frame(
            file=file_name2,
            tag="thetag",
            group=cplui.Frame.FrameGroup.RAW,
            level=cplui.Frame.FrameLevel.INTERMEDIATE,
            frameType=cplui.Frame.FrameType.IMAGE,
        )

        assert single_frame1 != 0
        assert single_frame1 != single_frame2

    def test_set_file(self, make_mock_fits):
        single_frame = cplui.Frame(make_mock_fits())

        # Different filename
        new_val = make_mock_fits()
        single_frame.file = new_val
        assert Path(single_frame.file) == Path(new_val)
        single_frame.file = Path(new_val)
        assert Path(single_frame.file) == Path(new_val)

    def test_set_tag(self, make_mock_fits):
        single_frame = cplui.Frame(make_mock_fits())

        new_val = "A_tag_without_spaces"
        single_frame.tag = new_val
        assert single_frame.tag == new_val

    def test_set_group(self, make_mock_fits):
        single_frame = cplui.Frame(make_mock_fits())

        new_val = cplui.Frame.FrameGroup.PRODUCT
        single_frame.group = new_val
        assert single_frame.group == new_val

    def test_set_level(self, make_mock_fits):
        single_frame = cplui.Frame(make_mock_fits())

        new_val = cplui.Frame.FrameLevel.TEMPORARY
        single_frame.level = new_val
        assert single_frame.level == new_val

    def test_set_type(self, make_mock_fits):
        single_frame = cplui.Frame(make_mock_fits())

        new_val = cplui.Frame.FrameType.MATRIX
        single_frame.type = new_val
        assert single_frame.type == new_val


class TestFrameSet:
    def test_noseg_with_junk_sof(self, tmp_path):
        """
        Tests with a SOF file made of junk data
        I encountered this when fits_fixture.py hadn't taken into account bytes-like paths,
        and it generated a SOF containing the following:
        <unit_test.BytesPathLike object at 0x7ff17d0940a0> example_tag
        Which ended up segfaulting pycpl.

        This tests aims to ensure this segfault, and other errors,
        don't break pycpl irrepairably.
        """

        sof_filename1 = tmp_path / "junk_data_sof1"
        sof_writer = sof_filename1.open("w+")
        sof_writer.write(
            "very<bad>name)$(with_unknown_command_expansion) example_tag group and left over elements"
        )
        sof_writer.close()

        _ = cplui.FrameSet(sof_filename=str(sof_filename1))

    def test_nonexistant_file(self):
        sof_filename = "NON-EXISTANT FILE"

        if os.path.exists(sof_filename):
            raise RuntimeError(
                "This unit test requires that NON-EXISTANT FILE not exist."
            )

        with pytest.raises(cplcore.FileNotFoundError):
            _ = cplui.FrameSet(sof_filename=sof_filename)

    @pytest.mark.skipif(
        platform == "darwin", reason="OSX not compatible with utf-8 encoding"
    )
    def test_construct_not_utf8_fits(self, make_mock_fits, make_mock_sof):
        fits_filename1 = make_mock_fits()
        fits_filename2 = os.fsdecode(
            bytes(fits_filename1, encoding="utf-8") + bytes.fromhex("8081")
        )
        shutil.copyfile(fits_filename1, fits_filename2)

        sof_filename1 = make_mock_sof([(fits_filename1, "example_tag")])
        sof_filename2 = make_mock_sof([(fits_filename2, "example_tag")])

        try:
            # similar to test_identical_sofs
            frame_set1 = cplui.FrameSet(sof_filename=sof_filename1)
            frame_set2 = cplui.FrameSet(sof_filename=sof_filename2)

            assert frame_set1[0].file == fits_filename1
            assert frame_set2[0].file == fits_filename2
        finally:
            os.remove(fits_filename2)

    @pytest.mark.skipif(
        platform == "darwin", reason="OSX not compatible with utf-8 encoding"
    )
    def test_construct_not_utf8_sof(self, make_mock_fits, make_mock_sof):
        fits_filename1 = make_mock_fits()

        sof_filename1 = make_mock_sof([(fits_filename1, "example_tag")])
        sof_filename2 = os.fsdecode(
            bytes(sof_filename1, encoding="utf-8") + bytes.fromhex("8081")
        )

        shutil.copyfile(sof_filename1, sof_filename2)

        try:
            # similar to test_identical_sofs
            frame_set1 = cplui.FrameSet(sof_filename=sof_filename1)
            frame_set2 = cplui.FrameSet(sof_filename=sof_filename2)

            assert frame_set1 == frame_set2
        finally:
            os.remove(sof_filename2)

    def test_different_fits(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()

        sof_filename1 = make_mock_sof([(fits_filename1, "example_tag")])
        sof_filename2 = make_mock_sof([(fits_filename2, "example_tag")])

        frame_set1 = cplui.FrameSet(sof_filename=sof_filename1)
        frame_set2 = cplui.FrameSet(sof_filename=sof_filename2)

        assert frame_set1 != frame_set2

    def test_different_tags(self, make_mock_sof, make_mock_fits):
        fits_filename = make_mock_fits()
        sof_filename1 = make_mock_sof([(fits_filename, "example_tag")])
        sof_filename2 = make_mock_sof([(fits_filename, "another_tag")])

        frame_set1 = cplui.FrameSet(sof_filename=sof_filename1)
        frame_set2 = cplui.FrameSet(sof_filename=sof_filename2)

        assert frame_set1 != frame_set2

    def test_shared_sof(self, make_mock_sof, make_mock_fits):
        fits_filename = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename, "example_tag")])

        frame_set1 = cplui.FrameSet(sof_filename=sof_filename)
        frame_set2 = cplui.FrameSet(sof_filename=sof_filename)

        assert frame_set1 == frame_set2

    def test_identical_sofs(self, make_mock_sof, make_mock_fits):
        # For this test we create 1 FITS frame file
        # and link to it from 2 SOF files
        # The resulting framesets should be ==
        fits_filename = make_mock_fits()
        sof_filename1 = make_mock_sof([(fits_filename, "example_tag")])
        sof_filename2 = make_mock_sof([(fits_filename, "example_tag")])

        # It is required, for the test to have any meaning, that it is testing
        # the parsing of 2 distinct files, not the same files.
        assert sof_filename1 != sof_filename2

        frame_set1 = cplui.FrameSet(sof_filename=sof_filename1)
        frame_set2 = cplui.FrameSet(sof_filename=sof_filename2)

        assert frame_set1 == frame_set2

    def test_frameset_matches_equivalent_frame(self, make_mock_sof, make_mock_fits):
        # We create a FITS file, with some random data in it
        # Then a SOF file is created, inside it, it names said FITS file.
        fits_filename = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename, "thetag")])

        frame_set = cplui.FrameSet(sof_filename=sof_filename)

        expected_frame = cplui.Frame(fits_filename, "thetag")

        assert len(frame_set) == 1
        assert frame_set[0] == expected_frame

    def test_multiple_frames(self, make_mock_sof, make_mock_fits):
        # Multiple identical frames in 1 SOF file
        # Then does matches_equivalent_frame matching
        fits_filename_1 = make_mock_fits()
        fits_filename_2 = make_mock_fits()
        sof_filename = make_mock_sof(
            [(fits_filename_1, "the_tag_1"), (fits_filename_2, "another_tag_2")]
        )

        frame_set = cplui.FrameSet(sof_filename=sof_filename)

        expected_frame_1 = cplui.Frame(fits_filename_1, "the_tag_1")
        expected_frame_2 = cplui.Frame(fits_filename_2, "another_tag_2")

        assert len(frame_set) == 2
        assert frame_set[0] == expected_frame_1
        assert frame_set[1] == expected_frame_2

    def test_new_has_no_elements(self):
        frame_set = cplui.FrameSet()

        assert len(frame_set) == 0
        with pytest.raises(IndexError):
            frame_set[0]

    def test_frameset_append_single(self, make_mock_fits):
        frame_set = cplui.FrameSet()

        to_add = cplui.Frame(make_mock_fits(), "thetag")
        frame_set.append(to_add)

        assert len(frame_set) == 1
        assert frame_set[0] == to_add

    def test_frameset_append_multiple(self, make_mock_fits):
        frame_set = cplui.FrameSet()

        to_add_1 = cplui.Frame(make_mock_fits(), "thetag")
        to_add_2 = cplui.Frame(make_mock_fits(), "another_tag")
        frame_set.append(to_add_1)
        frame_set.append(to_add_2)

        assert len(frame_set) == 2
        assert frame_set[0] == to_add_1
        assert frame_set[1] == to_add_2

    def test_add_frame_to_sof_file(self, make_mock_sof, make_mock_fits):
        # difference between this and append_multiple
        # is that we are appending to an already existing one
        fits_filename = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename, "thetag")])
        frame_set = cplui.FrameSet(sof_filename=sof_filename)

        # Now it's the same as append_multiple from here on
        to_add_1 = cplui.Frame(make_mock_fits(), "tag_two")
        to_add_2 = cplui.Frame(make_mock_fits(), "another_tag")
        frame_set.append(to_add_1)
        frame_set.append(to_add_2)

        assert len(frame_set) == 3
        assert frame_set[1] == to_add_1
        assert frame_set[2] == to_add_2

    def test_sof_formatting(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fits_filename3 = make_mock_fits()
        sof_filename = make_mock_sof(
            [
                (fits_filename1, "thetag1"),
                (fits_filename2, "thetag2"),
                (fits_filename3, "thetag3"),
            ]
        )
        frame_set = cplui.FrameSet(sof_filename=sof_filename)
        original_pwd = os.getcwd()
        os.chdir("/")
        assert os.path.exists(
            frame_set[0].file
        )  # Check if it can been seen from a non-relative directory: Frame.file should be full path
        assert frame_set[0].tag == "thetag1"
        assert os.path.exists(frame_set[1].file)
        assert frame_set[1].tag == "thetag2"
        assert os.path.exists(frame_set[2].file)
        assert frame_set[2].tag == "thetag3"
        os.chdir(original_pwd)

    def test_frame_repr(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fits_filename3 = make_mock_fits()
        sof_filename = make_mock_sof(
            [
                (fits_filename1, "thetag1"),
                (fits_filename2, "thetag2"),
                (fits_filename3, "thetag3"),
            ]
        )
        frame_set = cplui.FrameSet(sof_filename=sof_filename)
        fmt = "cpl.ui.Frame('{}', '{}', {}, {}, {})"
        assert repr(frame_set[0]) == fmt.format(
            frame_set[0].file,
            frame_set[0].tag,
            "cpl.ui.Frame.FrameGroup.NONE",
            "cpl.ui.Frame.FrameLevel.NONE",
            "cpl.ui.Frame.FrameType.NONE",
        )
        assert repr(frame_set[1]) == fmt.format(
            frame_set[1].file,
            frame_set[1].tag,
            "cpl.ui.Frame.FrameGroup.NONE",
            "cpl.ui.Frame.FrameLevel.NONE",
            "cpl.ui.Frame.FrameType.NONE",
        )
        assert repr(frame_set[2]) == fmt.format(
            frame_set[2].file,
            frame_set[2].tag,
            "cpl.ui.Frame.FrameGroup.NONE",
            "cpl.ui.Frame.FrameLevel.NONE",
            "cpl.ui.Frame.FrameType.NONE",
        )

    def test_frame_str(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fits_filename3 = make_mock_fits()
        sof_filename = make_mock_sof(
            [
                (fits_filename1, "thetag1"),
                (fits_filename2, "thetag2"),
                (fits_filename3, "thetag3"),
            ]
        )
        frame_set = cplui.FrameSet(sof_filename=sof_filename)
        fmt = "(file={}, tag={}, group={}, level={}, type={})"
        assert str(frame_set[0]) == fmt.format(
            frame_set[0].file,
            frame_set[0].tag,
            "FrameGroup.NONE",
            "FrameLevel.NONE",
            "FrameType.NONE",
        )
        assert str(frame_set[1]) == fmt.format(
            frame_set[1].file,
            frame_set[1].tag,
            "FrameGroup.NONE",
            "FrameLevel.NONE",
            "FrameType.NONE",
        )
        assert str(frame_set[2]) == fmt.format(
            frame_set[2].file,
            frame_set[2].tag,
            "FrameGroup.NONE",
            "FrameLevel.NONE",
            "FrameType.NONE",
        )

    def test_frame_dump_string(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename1, "thetag1")])
        fs = cplui.FrameSet(sof_filename=sof_filename)
        expect = """{}  \t{}  {}  {}  {}  \n""".format(
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        assert fs[0].dump(show=False) == expect
        assert isinstance(fs[0].dump(show=False), str)

    def test_frame_dump_file(self, make_mock_sof, make_mock_fits, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_frame_dump.txt"
        filename = tmp_path.joinpath(p)

        fits_filename1 = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename1, "thetag1")])
        fs = cplui.FrameSet(sof_filename=sof_filename)
        fs[0].dump(filename=filename)
        # read in the file contents
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        expect = """{}  \t{}  {}  {}  {}  \n""".format(
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        assert contents == expect

    def test_frame_dump_stdout(self, make_mock_sof, make_mock_fits):
        # since we cannot call fixtures directly, we work around this by first making the mock files
        # and then use the filenames in the script
        fits_filename1 = make_mock_fits()
        sof_filename = make_mock_sof([(fits_filename1, "thetag1")])
        fs = cplui.FrameSet(sof_filename=sof_filename)
        expect = """{}  \t{}  {}  {}  {}  \n""".format(
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )

        cmd = "from cpl import ui as cplui; "
        cmd += "fs = cplui.FrameSet(sof_filename='{}');".format(sof_filename)
        cmd += "fs[0].dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """{}  \t{}  {}  {}  {}  \n""".format(
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        assert outp == expect

    def test_frameset_repr(self, make_mock_sof, make_mock_fits):
        # first test an empty FrameSet
        empty = cplui.FrameSet()
        assert repr(empty) == "cpl.ui.FrameSet([])"
        # now with more frames
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fits_filename3 = make_mock_fits()
        sof_filename = make_mock_sof(
            [
                (fits_filename1, "thetag1"),
                (fits_filename2, "thetag2"),
                (fits_filename3, "thetag3"),
            ]
        )
        frame_set = cplui.FrameSet(sof_filename=sof_filename)
        expect = (
            "cpl.ui.FrameSet([cpl.ui.Frame('{}', '{}', {}, {}, {}),\n"
            "                 cpl.ui.Frame('{}', '{}', {}, {}, {}),\n"
            "                 cpl.ui.Frame('{}', '{}', {}, {}, {})])".format(
                frame_set[0].file,
                frame_set[0].tag,
                "cpl.ui.Frame.FrameGroup.NONE",
                "cpl.ui.Frame.FrameLevel.NONE",
                "cpl.ui.Frame.FrameType.NONE",
                frame_set[1].file,
                frame_set[1].tag,
                "cpl.ui.Frame.FrameGroup.NONE",
                "cpl.ui.Frame.FrameLevel.NONE",
                "cpl.ui.Frame.FrameType.NONE",
                frame_set[2].file,
                frame_set[2].tag,
                "cpl.ui.Frame.FrameGroup.NONE",
                "cpl.ui.Frame.FrameLevel.NONE",
                "cpl.ui.Frame.FrameType.NONE",
            )
        )
        assert repr(frame_set) == expect

    def test_frameset_str(self, make_mock_sof, make_mock_fits):
        # first test an empty FrameSet
        empty = cplui.FrameSet()
        assert str(empty) == "[]"
        # now with more frames
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        fits_filename3 = make_mock_fits()
        sof_filename = make_mock_sof(
            [
                (fits_filename1, "thetag1"),
                (fits_filename2, "thetag2"),
                (fits_filename3, "thetag3"),
            ]
        )
        frame_set = cplui.FrameSet(sof_filename=sof_filename)
        expect = (
            "[(file={}, tag={}, group={}, level={}, type={}),\n"
            " (file={}, tag={}, group={}, level={}, type={}),\n"
            " (file={}, tag={}, group={}, level={}, type={})]".format(
                frame_set[0].file,
                frame_set[0].tag,
                "FrameGroup.NONE",
                "FrameLevel.NONE",
                "FrameType.NONE",
                frame_set[1].file,
                frame_set[1].tag,
                "FrameGroup.NONE",
                "FrameLevel.NONE",
                "FrameType.NONE",
                frame_set[2].file,
                frame_set[2].tag,
                "FrameGroup.NONE",
                "FrameLevel.NONE",
                "FrameType.NONE",
            )
        )
        assert str(frame_set) == expect

    def test_frameset_dump_string(self, make_mock_sof, make_mock_fits):
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        sof_filename = make_mock_sof(
            [(fits_filename1, "thetag1"), (fits_filename2, "thetag2")]
        )
        fs = cplui.FrameSet(sof_filename=sof_filename)
        dump_str = fs.dump(show=False)
        # strip the first line off the dump, as it contains a memaddress we don't need to match for
        dump_str = "\n".join(dump_str.split("\n")[1:])
        expect = """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            0,
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        expect += """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            1,
            fs[1].file,
            fs[1].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        assert dump_str == expect
        assert isinstance(fs.dump(show=False), str)

    def test_frameset_dump_file(self, make_mock_sof, make_mock_fits, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_frameset_dump.txt"
        filename = tmp_path.joinpath(p)

        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        sof_filename = make_mock_sof(
            [(fits_filename1, "thetag1"), (fits_filename2, "thetag2")]
        )
        fs = cplui.FrameSet(sof_filename=sof_filename)
        fs.dump(filename=filename)

        # read in the file contents
        contents = ""
        lnum = 0
        with open(str(filename), "r") as f:
            for line in f.readlines():
                # skip first line with memaddress (see test_frameset_dump_string above)
                if lnum > 0:
                    contents += line
                lnum += 1
        expect = """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            0,
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        expect += """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            1,
            fs[1].file,
            fs[1].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        assert contents == expect

    def test_frameset_dump_stdout(self, make_mock_sof, make_mock_fits):
        # since we cannot call fixtures directly, we work around this by first making the mock files
        # and then use the filenames in the script
        fits_filename1 = make_mock_fits()
        fits_filename2 = make_mock_fits()
        sof_filename = make_mock_sof(
            [(fits_filename1, "thetag1"), (fits_filename2, "thetag2")]
        )
        fs = cplui.FrameSet(sof_filename=sof_filename)

        expect = """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            0,
            fs[0].file,
            fs[0].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )
        expect += """  {}  {}  \t{}  {}  {}  {}  \n""".format(
            1,
            fs[1].file,
            fs[1].tag,
            "CPL_FRAME_TYPE_NONE",
            "CPL_FRAME_GROUP_NONE",
            "CPL_FRAME_LEVEL_NONE",
        )

        cmd = "from cpl import ui as cplui; "
        cmd += "fs = cplui.FrameSet(sof_filename='{}');".format(sof_filename)
        cmd += "fs.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        # strip the first line off the dump, as it contains a memaddress we don't need to match for
        outp = "\n".join(outp.split("\n")[1:])
        assert outp == expect
