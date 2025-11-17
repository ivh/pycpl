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
from shutil import which


# esorex executable required to generate the output we're validating against
# either it's on the $PATH or it's pointed to by ESOREX_EXE
def find_esorex_exe():
    if "ESOREX_EXE" in environ and which(environ["ESOREX_EXE"]) is not None:
        return Path(environ["ESOREX_EXE"]).resolve()
    elif which("esorex") is not None:
        return Path(which("esorex")).resolve()
    else:
        return None


def find_pyesorex_exe():
    if "PYESOREX_EXE" in environ and which(environ["PYESOREX_EXE"]) is not None:
        return Path(environ["PYESOREX_EXE"]).resolve()
    elif which("pyesorex") is not None:
        return Path(which("pyesorex")).resolve()
    else:
        return None


esorex_exe = find_esorex_exe()
if esorex_exe is None:
    raise RuntimeError(
        "Environment variable ESOREX_EXE (set to 'esorex') is required to be set, or 'esorex' must be on PATH"
    )

pyesorex_exe = find_pyesorex_exe()
if pyesorex_exe is None:
    raise RuntimeError(
        "Environment variable PYESOREX_EXE (set to 'pyesorex') is required to be set, or 'pyesorex' must be on PATH"
    )
