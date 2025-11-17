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

"""Tests setting of C recipe parameter values.

The tests in this file require recipes from the GIRAFFE pipeline and sample data and SOF files
from the GIRAFFE regression tests. They will be skipped if the REGTEST environmental variable
(which should point to the location of the regression test data) is not set, or if either
EsoRex or PyEsoRex cannot be found.
"""
import os

import pytest

import cpl.ui as cplui

try:
    from tests.utilities.regtests import get_sofs, sof_recipe_name
except RuntimeError as err:
    # Runtime error will be raised by that import if required environment variables are not
    # set or esorex could not be found, in which case skip all these tests.
    pytest.skip(f"Skipping recipe parameter tests: {err}", allow_module_level=True)

sofs = get_sofs()


@pytest.fixture(scope="module")
def test_recipe_param_path(tmp_path_factory):
    test_path = tmp_path_factory.mktemp("test_recipe_param")
    return test_path


def test_recipe_param_effect(test_recipe_param_path):
    run1_path = test_recipe_param_path / "run1"
    run1_path.mkdir()
    os.chdir(run1_path)
    sof = sofs[0]
    recipe = cplui.CRecipe(sof_recipe_name(sof))
    input_frames = cplui.FrameSet(str(sof))
    output_frames1 = recipe.run(input_frames)
    output_files1 = {frame.file for frame in output_frames1}
    assert len(output_files1) == 2
    assert "bad_pixel_map.fits" in output_files1

    run2_path = test_recipe_param_path / "run2"
    run2_path.mkdir()
    os.chdir(run2_path)
    run2_settings = {
        "giraffe.stacking.method": "median",
        "giraffe.masterbias.bpm.create": False,
    }
    output_frames2 = recipe.run(input_frames, run2_settings)
    output_files2 = {frame.file for frame in output_frames2}
    assert len(output_files2) == 1
    assert "bad_pixel_map.fits" not in output_files2
