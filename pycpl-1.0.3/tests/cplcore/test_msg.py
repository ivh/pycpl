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

import re

from cpl.core import Msg
import pytest

# The main intention of these tests is to ensure the program doesn't fail while running them
# std out can be checked with  -s option for pytest

CPL_MAX_MSG_LENGTH = 1024
STR_LENGTH = 80
verylongnamesize = 1024 * 1024
TIME_REGEX = "[0-2][0-9]:[0-5][0-9]:[0-5][0-9]"


class TestMessage:
    # First basic test
    def test_display(self):
        Msg.info(
            "TestMessage", f"Display a string of {STR_LENGTH} dots: {'.' * STR_LENGTH}"
        )
        Msg.info("TestMessage", "This is an info message")
        Msg.warning("TestMessage", "This is an warning message")
        Msg.error("TestMessage", "This is an error message")

    @pytest.mark.xfail(
        reason="Msg state is currently not initialised properly. See PIPE-11000."
    )
    def test_settings(self):
        Msg.set_config(
            level=Msg.SeverityLevel.DEBUG,
            domain="unit_testing",
            show_domain=True,
            show_time=True,
        )
        Msg.info("TestMessage", f"I can see domain is {Msg.get_config()['domain']}")
        Msg.set_config(indent=1)
        Msg.info("TestMessage", "Indent1")
        Msg.set_config(indent=2)
        Msg.info("TestMessage", "Indent2")
        Msg.set_config(indent=3)
        Msg.info("TestMessage", "Indent3")
        assert Msg.get_config() == dict(
            {
                "level": Msg.SeverityLevel.DEBUG,
                "domain": "unit_testing",
                "width": 0,
                "log_name": ".logfile",
                "indent": 3,
                "show_threadid": False,
                "show_domain": True,
                "show_time": True,
                "show_component": False,
            }
        )
        Msg.set_config(indent=0)
        Msg.info("TestMessage", "Back to the start")
        Msg.info(
            "TestMessage",
            f"Display a string of {CPL_MAX_MSG_LENGTH - 1} dots: "
            + "." * (CPL_MAX_MSG_LENGTH - 1),
        )

    # Tests long component name
    def test_longname(self):  # Only meant to see the error line
        Msg.set_config(
            level=Msg.SeverityLevel.ERROR, show_component=True, show_threadid=True
        )
        Msg.info("normalComponent", "test for long component name")
        verylongname = "A" * verylongnamesize
        Msg.debug(verylongname, "test for long component name")
        Msg.info(verylongname, "test for long component name")
        Msg.warning(verylongname, "test for long component name")
        Msg.error(verylongname, "test for long component name")

    def test_file(self, tmp_path):
        component = "test_component"
        Msg.set_config(
            level=Msg.SeverityLevel.ERROR,
            show_component=True,
            domain="test_file",
            show_domain=True,
            show_time=False,
        )
        log_path = tmp_path / "test_log.txt"
        if len(str(log_path)) > 72:
            pytest.xfail("CPL does not support logfile paths longer than 72 characters")
        Msg.start_file(Msg.SeverityLevel.DEBUG, log_path)
        Msg.debug(component, "Debug line")
        Msg.info(component, "Info line")
        Msg.warning(component, "Warning line")
        Msg.error(component, "Error line: Oh no!")
        Msg.set_config(indent=1)
        Msg.info(component, "Indented line")
        Msg.stop_file()
        with log_path.open("r") as f:
            log_lines = f.readlines()
            # See cpl_msg.c:266 _cpl_timestamp_iso8601 for ISO-8601 implementation
            # for this first Start TIme
            assert (
                re.fullmatch(
                    r"Start time     : [0-9]{4}-[0-1][0-9]-[0-3][0-9]T"
                    + TIME_REGEX
                    + "\n",
                    log_lines[1],
                )
                is not None
            )
            assert (
                re.fullmatch(r"Program name   : test_file\n", log_lines[2]) is not None
            )
            assert (
                re.fullmatch(r"Severity level : \[ DEBUG \] \n", log_lines[3])
                is not None
            )
            # The rest of these times are from cpl_msg.c:560 cpl_msg_out
            # [tid=XXX] is present when your CPL library was compiled with openMP.
            assert (
                re.fullmatch(
                    TIME_REGEX
                    + r" \[ DEBUG \] test_component: (\[tid=[0-9]{0,3}\] )?Debug line\n",
                    log_lines[5],
                )
                is not None
            )
            assert (
                re.fullmatch(
                    TIME_REGEX
                    + r" \[ INFO  \] test_component: (\[tid=[0-9]{0,3}\] )?Info line\n",
                    log_lines[6],
                )
                is not None
            )
            assert (
                re.fullmatch(
                    TIME_REGEX
                    + r" \[WARNING\] test_component: (\[tid=[0-9]{0,3}\] )?Warning line\n",
                    log_lines[7],
                )
                is not None
            )
            assert (
                re.fullmatch(
                    TIME_REGEX
                    + r" \[ ERROR \] test_component: (\[tid=[0-9]{0,3}\] )?Error line: Oh no!\n",
                    log_lines[8],
                )
                is not None
            )
            assert (
                re.fullmatch(
                    TIME_REGEX
                    + r" \[ INFO  \] test_component: (\[tid=[0-9]{0,3}\] )?  Indented line\n",
                    log_lines[9],
                )
                is not None
            )
