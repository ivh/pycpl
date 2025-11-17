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

from os import environ
from pathlib import Path
from re import match
from warnings import warn

# Before any tests are run in this script,
# Certain environment variables must be set for the validation tests
# to know where to look for the regression test data itself, and the
# esorex tool for comparison
#
# REQUIRED ENVIRONMENT VARIABLES:
# RECIPE_DIR: contains esopipes-plugins/giraffe, default value is dependent on the platform and compile flags of the C++ library. may be /usr/local or /opt/local
# REGTESTS: contains 'sof' directory, with .sof files immediately within. Default is $PWD/pipedata/regression/giraffe.
# REGTESTS_{RAW,STATIC,CALIB}: environment variables used by the above .sof files. This defaults to $REGTESTS/{raw,static,data}
# ESOREX_EXE: points at the esorex executable. Defaults to the esorex from $PATH
#
# Usage of this module:
#     esorex_exe : Path
#     frameset_dir : Path
#     sof_recipe_name: Path --> str
#     get_sofs: () --> Paths
#     after usage, REGTESTS_{RAW,STATIC,CALIB} will be set as environment variables.
#     and after usage, cpl.ui.Recipe.recipe_dir will be set appropriately

# To find regression tests, we look for a directory that has 'sof' in it,
# either in an environment variable REGTESTS,
# or in the current directory/pipedata/regression/giraffe.


def find_frameset_dir():
    regtests_environ = (
        Path(environ["REGTESTS"]).resolve() if "REGTESTS" in environ else None
    )
    regtests_cwd = (
        Path.cwd()
        .joinpath("pipedata")
        .joinpath("regression")
        .joinpath("giraffe")
        .joinpath("dfs")
        .resolve()
    )

    if regtests_environ is None and regtests_cwd is None:
        return None
    else:
        if (
            regtests_environ is not None
            and regtests_environ.is_dir()
            and regtests_environ.joinpath("sof").is_dir()
        ):
            regtests = regtests_environ
        elif (
            regtests_cwd is not None
            and regtests_cwd.is_dir()
            and regtests_cwd.joinpath("sof").is_dir()
        ):
            regtests = regtests_cwd
        else:
            return None  # Neither directory meets requirements

        # These environment variables are used within the .sof files.
        if "REGTESTS_RAW" not in environ:
            environ["REGTESTS_RAW"] = str(regtests.joinpath("raw"))
        if "REGTESTS_STATIC" not in environ:
            environ["REGTESTS_STATIC"] = str(regtests.joinpath("datastatic"))
        if "REGTESTS_CALIB" not in environ:
            environ["REGTESTS_CALIB"] = str(regtests.joinpath("cal"))

        return regtests.joinpath("sof")


def set_recipe_dir():
    from cpl.ui import CRecipe

    if "RECIPE_DIR" in environ:
        CRecipe.recipe_dir = environ["RECIPE_DIR"]


def sof_recipe_name(sof):
    # e.g. 0031.giwavecalibration.sof --> giwavecalibration
    return match("([0-9]+)\\.(.*)", sof.stem).group(2)


def sof_number(sof):
    # e.g. 0031.giwavecalibration.sof --> 0031
    return match("([0-9]+)\\.(.*)", sof.stem).group(1)


def get_sofs():
    sofs = sorted(frameset_dir.glob("*.sof"))
    if "SOFS" in environ:
        numbers_to_sofs = dict(
            ((int(sof_number(sof_path)), sof_path) for sof_path in sofs)
        )
        sofs = []

        try:
            for range_or_single in environ["SOFS"].split(","):
                if "-" in range_or_single:
                    [start, end] = [int(x) for x in range_or_single.split("-")]
                    sofs += [numbers_to_sofs[x] for x in range(start, end + 1)]
                else:
                    sofs.append(numbers_to_sofs[int(range_or_single)])
        except Exception:
            warn(
                "Invalid format for SOFS variable. Accepted format: ',' delimited list of integer or integer-integer"
            )
            sofs = sorted(frameset_dir.glob("*.sof"))

    return sofs


frameset_dir = find_frameset_dir()
if frameset_dir is None:
    raise RuntimeError(
        "Environment variable REGTESTS (containing sof, raw, datastatic, cal) is required to be set, or pipedata/regression/giraffe must exist"
    )

set_recipe_dir()
