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

import cpl.ui as cplui
import os
import faulthandler
from astropy.io import fits
import shutil
import shelve
import pytest
from pathlib import Path

try:
    from tests.utilities import get_sofs, sof_recipe_name
except RuntimeError as err:
    # Runtime error will be raised by that import if required environment variables are not
    # set or esorex could not be found, in which case skip all these tests.
    pytest.skip(f"Skipping regression tests: {err}", allow_module_level=True)

faulthandler.enable()

# Change these:
regression = shelve.open("regression")

sofs = get_sofs()


def run(sof):
    frameset = cplui.FrameSet(str(sof))
    r = cplui.CRecipe(sof_recipe_name(sof))
    return r.run(frameset, {})


@pytest.mark.parametrize("test_sof", sofs)
def test_regression(test_sof):
    res = run(test_sof)
    oldSet = regression[str(test_sof)]
    for i in range(len(res)):
        fd = fits.FITSDiff(
            oldSet[i].file, res[i].file, ignore_keywords=["*"]
        )  # ignore all keywords such as date
        assert fd.identical, "\n" + test_sof + "\n" + fd.report()


if __name__ == "__main__":
    old_path = Path.cwd().joinpath("old").resolve()
    if old_path.is_dir():
        shutil.rmtree(old_path)
    old_path.mkdir()
    os.chdir(old_path)
    for sof in sofs:
        frames = [f for f in run(sof)]
        regression[str(sof)] = frames
    print("Install new version of pycpl and run in pytest")
