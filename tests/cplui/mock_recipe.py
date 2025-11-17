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

from cpl.core import Msg, PropertyList, io
from cpl.ui import PyRecipe, ParameterValue, ParameterList, Frame, FrameSet


class mock_recipe(PyRecipe):
    _name = "demo"
    _author = "AAO"
    _copyright = "Use however you want"
    _description = "demo recipe for pycpl"
    _email = "someone@mq.edu.au"
    _synopsis = "demo recipe"
    _version = "2.3.2"

    def __init__(self):
        self.parameters = ParameterList()
        int_param = ParameterValue("multiplier", "test description", "test context", 2)
        int_param.cli_alias = "mul"
        self.parameters.append(int_param)

    def multiply_inputs(self, fs, multi_by):
        created = False
        # Get propertylist from all inputs, then store into a single fits file
        for f in fs:
            if f.tag == "INPUT":
                toChange = PropertyList.load(f.file, 0)
                toChange["INT_PROPERTY"].value *= multi_by
                # If output file has already been created, append, otherwise create
                if created:
                    toChange.save("out.fits", io.EXTEND)
                    created = True
                else:
                    toChange.save("out.fits", io.CREATE)
        return Frame("out.fits", tag="output", group=Frame.FrameGroup.PRODUCT)

    def run(self, frameSet, options):
        for f in frameSet:
            f.group = Frame.FrameGroup.PRODUCT
        return frameSet

        Msg.info(
            "demo",
            "Value of {} with default {} is {}".format(
                self.parameters["multiplier"].name,
                self.parameters["multiplier"].default,
                self.parameters["multiplier"].value,
            ),
        )
        # Run on frameset
        out_frame = self.multiply_inputs(frameSet, self.parameters["multiplier"].value)

        output = FrameSet()
        output.append(out_frame)  # Save to frameset with PRODUCT group
        return output

    def __del__(self):
        print(
            "Cleaning up: All objects should be deleted by python garbage collector but if dev wants to they can force delete or send things off somewhere"
        )
