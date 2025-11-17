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

import subprocess

from astropy.io import fits
import pytest

try:
    from tests.utilities.regtests import get_sofs, sof_recipe_name
except RuntimeError as err:
    # Runtime error will be raised by that import if required environment variables are not
    # set, in which case skip all these tests.
    pytest.skip(f"Skipping validation tests: {err}", allow_module_level=True)

try:
    from tests.utilities.rex import esorex_exe, pyesorex_exe
except RuntimeError as err:
    # Runtime error will be raised by that import if required environment variables are not
    # set or esorex or pyesorex could not be found, in which case skip all these tests.
    pytest.skip(f"Skipping validation tests: {err}", allow_module_level=True)

# Never wait() without a timeout...
# This is the maximum time to wait for a single recipe to execute, in seconds, and has been set to
# 10 minutes. With the GIRAFFE pipeline regtests and modest (laptop) hardware this is more than
# long enough but it is possible that this value may need to be changed if using these tests with
# other recipes or very low performance hardware.
TIMEOUT = 600
sofs = get_sofs()


@pytest.fixture(scope="module")
def rex_out_path(tmp_path_factory):
    rex_out = tmp_path_factory.mktemp("validation")
    return rex_out


@pytest.fixture(scope="function")
def rex_run(request, rex_out_path):
    def _rex_run(sof, rex_exe):
        recipe_name = sof_recipe_name(sof)
        rex_sof_path = rex_out_path / sof.stem / rex_exe.stem
        rex_sof_path.mkdir(parents=True, exist_ok=True)

        rex_process = subprocess.Popen(
            [rex_exe, recipe_name, str(sof)], cwd=rex_sof_path
        )

        def kill_rex():
            # Kills pyesorex process if it's still running.
            if rex_process.poll() is None:
                rex_process.kill()

        request.addfinalizer(kill_rex)

        return rex_sof_path, rex_process

    return _rex_run


@pytest.mark.parametrize("sof", sofs, ids=lambda x: x.stem)
def test(sof, rex_run):
    # An esorex process is run in the background whilst pycpl runs.
    esorex_sof_path, esorex_process = rex_run(sof, esorex_exe)
    # Run the same recipe & sof via pyesorex.
    pyesorex_sof_path, pyesorex_process = rex_run(sof, pyesorex_exe)
    # Wait (if necessary) for esorex to finish, check it didn't return an error.
    return_code = esorex_process.wait(timeout=TIMEOUT)
    assert (
        return_code == 0
    ), f"EsoRex error, Recipe:{sof_recipe_name(sof)} Sof file:{sof} Error:{return_code}:"
    # Do the same for pyesorex.
    return_code = pyesorex_process.wait(timeout=TIMEOUT)
    assert (
        return_code == 0
    ), f"PyEsoRex error, Recipe:{sof_recipe_name(sof)} Sof file:{sof} Error:{return_code}:"

    # Ensure that pycpl creates all files that esorex does
    # {set} comparison doesn't care about order
    assert {file_path.name for file_path in esorex_sof_path.glob("*.fits")} == {
        file_path.name for file_path in pyesorex_sof_path.glob("*.fits")
    }
    assert {file_path.name for file_path in esorex_sof_path.glob("*.paf")} == {
        file_path.name for file_path in pyesorex_sof_path.glob("*.paf")
    }

    for pyesorex_fits_path in pyesorex_sof_path.glob("*.fits"):
        # Take FITSDiff between corresponding esorex and pycpl output FITS files.
        fits_diff = fits.FITSDiff(
            str(esorex_sof_path / pyesorex_fits_path.name),
            str(pyesorex_fits_path),
            ignore_keywords=["*"],
        )  # ignore all keywords such as date
        assert (
            fits_diff.identical
        ), f"\n{sof.stem}\n{fits_diff.report()}"  # Report differences
