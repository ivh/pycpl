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

import math
import sys

import numpy as np
import pytest
import subprocess

from cpl import core as cplcore


class TestBivector:
    def test_constructor_zero(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Bivector.zeros(0)

    def test_constructor_large(self):
        bivec = cplcore.Bivector.zeros(1024 * 1024)
        assert len(bivec) == 2
        assert len(bivec[0]) == 1024 * 1024
        assert len(bivec[1]) == 1024 * 1024

    def test_constructor_tuple(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector((vec1, vec2))
        vec1[0] = 0

        assert len(bivec[0]) == 4
        assert len(bivec[1]) == 4
        assert bivec[0][0] == 9101
        assert bivec[0][1] == 89172
        assert bivec[0][2] == -104
        assert bivec[0][3] == 11.1289
        assert bivec[1][0] == -13.21
        assert bivec[1][1] == 62.212
        assert bivec[1][2] == 2235
        assert bivec[1][3] == 2

    def test_constructor_list(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector([vec1, vec2])
        vec1[0] = 0

        assert bivec[0][0] == 9101
        assert bivec[0][1] == 89172
        assert bivec[0][2] == -104
        assert bivec[0][3] == 11.1289
        assert bivec[1][0] == -13.21
        assert bivec[1][1] == 62.212
        assert bivec[1][2] == 2235
        assert bivec[1][3] == 2

    def test_constructor_empty(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Bivector(([], []))

    def test_constructor_mismatched_1(self):
        vec1 = cplcore.Vector([9101, 89172, -104])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Bivector((vec1, vec2))

    def test_constructor_mismatched_2(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235])
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Bivector((vec1, vec2))

    def test_set_element(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector((vec1, vec2))

        bivec[0][0] = 9401.18923

        assert bivec[0][0] == 9401.18923

    def test_iter_2(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector((vec1, vec2))

        i = iter(bivec)
        e1 = next(i)
        assert e1 == vec1
        e2 = next(i)
        assert e2 == vec2

    def test_iter_vec(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector((vec1, vec2))

        for e1, e2 in zip(vec1, bivec[0]):
            assert e1 == e2

        for e1, e2 in zip(vec2, bivec[1]):
            assert e1 == e2

    def test_equal(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec1 = cplcore.Bivector((vec1, vec2))
        vec3 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec4 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec2 = cplcore.Bivector((vec3, vec4))

        assert bivec1 == bivec2

    def test_notequal_element(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 55])  # 11.1289
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec1 = cplcore.Bivector((vec1, vec2))
        vec3 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec4 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec2 = cplcore.Bivector((vec3, vec4))

        assert bivec1 != bivec2

    def test_notequal_mismatch(self):
        vec1 = cplcore.Vector([9101, 89172, -104])  # 11.1289
        vec2 = cplcore.Vector([-13.21, 62.212, 2235])
        bivec1 = cplcore.Bivector((vec1, vec2))
        vec3 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec4 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec2 = cplcore.Bivector((vec3, vec4))

        assert bivec1 != bivec2

    def test_noequal_other(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec = cplcore.Bivector((vec1, vec2))

        assert bivec != 5

    def test_copy(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        bivec2 = cplcore.Bivector((vec1, vec2))

        bivec1 = bivec2.copy()

        assert bivec1[0][0] == bivec2[0][0]
        assert bivec1[0][1] == bivec2[0][1]
        assert bivec1[0][2] == bivec2[0][2]
        assert bivec1[0][3] == bivec2[0][3]
        assert bivec1[1][0] == bivec2[1][0]
        assert bivec1[1][1] == bivec2[1][1]
        assert bivec1[1][2] == bivec2[1][2]
        assert bivec1[1][3] == bivec2[1][3]
        assert bivec1[0][0] == 9101
        assert bivec1[0][1] == 89172
        assert bivec1[0][2] == -104
        assert bivec1[0][3] == 11.1289
        assert bivec1[1][0] == -13.21
        assert bivec1[1][1] == 62.212
        assert bivec1[1][2] == 2235
        assert bivec1[1][3] == 2

    def test_read(self, tmp_path):
        vfile = tmp_path.joinpath("bivector_1")

        with vfile.open("w+") as vfile_write:
            # (1) Lines beginning with a hash are ignored,
            # (2) blank lines also.
            # (3) Valid lines require 2 integers separated by whitespace
            vfile_write.write("89471\n")  # Fails (3)
            vfile_write.write("9101 -13.21\n")  # OK
            vfile_write.write("\n")  # Fails (2)
            vfile_write.write("89172 62.212\n")  # OK
            vfile_write.write("# 1 2 234Anything\n ")  # Fails (1)
            vfile_write.write(
                " # 1239024 12312\n "
            )  # Fails (1) with leading whitespace
            vfile_write.write("-104 2235\n")  # OK
            vfile_write.write("20012_912837\n")  # Fails (3)
            vfile_write.write("11.1289\t2\n")  # OK

        bivec = cplcore.Bivector.read(vfile)
        assert len(bivec[0]) == 4
        assert len(bivec[1]) == 4
        assert bivec[0][0] == 9101
        assert bivec[0][1] == 89172
        assert bivec[0][2] == -104
        assert bivec[0][3] == 11.1289
        assert bivec[1][0] == -13.21
        assert bivec[1][1] == 62.212
        assert bivec[1][2] == 2235
        assert bivec[1][3] == 2

    def test_main(self, tmp_path):
        scale = (math.pi * 2) / 1024
        filename = tmp_path.joinpath("cpl_bivector_dump.txt")
        linear = [2.0, 3.0, 4.0]
        interpol = [math.log(10), math.e, math.pi]
        # cpl_test_init_macro("/dev/stdin", "cpl-help@eso.org", CPL_MSG_WARNING)
        # # INVALID         i=None.size
        # assert i < 0
        # #INVALID         dnull=None.x
        # assert dnull == None
        # #INVALID         dnull=None.x
        # assert dnull == None
        # #INVALID         dnull=None.y
        # assert dnull == None
        # #INVALID         dnull=None.y
        # assert dnull == None
        # #INVALID         vnull=None.x
        # assert vnull == None
        # #INVALID         vnull=None.x
        # assert vnull == None
        # #INVALID         vnull=None.y
        # assert vnull == None
        # #INVALID         vnull=None.y
        # assert vnull == None
        sinus = cplcore.Bivector.zeros(1024)
        # data_x = sinus.x  data_x isn't used after this line.
        data_y = sinus.y
        for i in range(1024):
            sinus.x[i] = i * scale
            sinus.y[i] = math.sin(sinus.x[i])

        cosinus = cplcore.Bivector.zeros(1024)
        # data_x = cosinus.x  data_x isn't used after this line
        data_y = cosinus.y
        for i in range(1024):
            cosinus.x[i] = (i * scale) - math.pi
            cosinus.y[i] = math.cos(cosinus.x[i])

        sinus.y
        sinus.x
        assert sinus.x is not sinus.y
        assert sinus.size == 1024
        sinus.dump(filename=filename)
        with pytest.raises(TypeError):
            tmp_fun = cplcore.Bivector.read(None)

        # assert tmp_fun == None
        with pytest.raises(cplcore.BadFileFormatError):
            tmp_fun = cplcore.Bivector.read("/dev/null")

        # assert tmp_fun == None
        tmp_fun = cplcore.Bivector(sinus)

        sinus = cplcore.Bivector.read(filename)
        np.testing.assert_allclose(
            tmp_fun.x, sinus.x, atol=48.0 * float(np.finfo(np.float32).eps)
        )
        np.testing.assert_allclose(
            tmp_fun.y, sinus.y, atol=6.0 * float(np.finfo(np.float32).eps)
        )

        tmp_fun = cplcore.Bivector.zeros(1024)
        tmp_fun.x = sinus.x.copy()
        tmp_fun.x.add_scalar(0.5)
        with pytest.raises(cplcore.DataNotFoundError):
            _ = sinus.interpolate_linear(tmp_fun.x)

        tmp_fun.x.subtract_scalar(1.0)
        with pytest.raises(cplcore.DataNotFoundError):
            _ = sinus.interpolate_linear(tmp_fun.x)

        sinus.x.fill(0)
        sinus.y.fill(0)
        tmp_fun.x.fill(0)

        tmp_fun = sinus.interpolate_linear(tmp_fun.x)
        np.testing.assert_equal(tmp_fun.y, sinus.y)

        sinus = cplcore.Bivector.zeros(1024 + 1)
        # data_x = sinus.x   data_x wasn't used after this line
        data_y = sinus.y
        for i in range(1024 + 1):
            sinus.x[i] = (i - 0.5) * scale
            sinus.y[i] = math.sin(sinus.x[i])

        tmp_fun = cplcore.Bivector.zeros(1024)
        vec1 = tmp_fun.x
        for i in range(1024):
            vec1[i] = i * scale

        tmp_fun = sinus.interpolate_linear(tmp_fun.x)
        data_y = tmp_fun.y
        assert math.isclose(data_y[0], 0, abs_tol=np.finfo(np.float32).eps)
        assert math.isclose(
            data_y[int(1024 / 2)], 0, abs_tol=10 * sys.float_info.epsilon
        )
        assert math.isclose(1.0, data_y[int(1024 / 4)], abs_tol=1.0 / 1024)
        assert math.isclose(-1.0, data_y[int((3 * 1024) / 4)], abs_tol=1.0 / 1024)

        tmp_fun = cplcore.Bivector(sinus)
        tmp_fun.x = sinus.x.copy()
        tmp_fun.y.fill(0)
        tmp_fun = sinus.interpolate_linear(tmp_fun.x)
        np.testing.assert_allclose(
            tmp_fun.y, sinus.y, atol=10.0 * sys.float_info.epsilon
        )

        vec1 = cplcore.Vector(linear)
        source = cplcore.Bivector((vec1, vec1))
        # This unit test previously contained the following code:
        # vec2=cplcore.Vector(interpol)
        # vec3=cplcore.Vector(3)
        # tmp_fun=cplcore.Bivector(vec2, vec3)
        # However this didn't work out well because there's no
        # python equivalent to Bivector's wrap_vectors function
        # (Since Bivectors, in C++, own their cpl::core::Vectors
        # they aren't references to anything python/shared_ptr)
        tmp_fun = source.interpolate_linear(cplcore.Vector(interpol))
        np.testing.assert_allclose(tmp_fun.x, tmp_fun.y, atol=sys.float_info.epsilon)

    def test_sort(self):
        xval = [8.0, 1.0, 2.0, 3.0, 4.0, 4.0, 6.0, 7.0, 0]
        yval = [8.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0]
        to_sort = cplcore.Bivector((xval, yval))

        # Invalid direction or mode values should raise TypeError
        with pytest.raises(TypeError):
            to_sort.sort(reverse="Yes, please")
        with pytest.raises(TypeError):
            to_sort.sort(mode=2)

        # Sort by X and by Y, both ascending and descending, and check that it is sorted.
        to_sort.sort()
        assert list(to_sort.x) == sorted(to_sort.x)
        to_sort.sort(reverse=True)
        assert list(to_sort.x) == sorted(to_sort.x, reverse=True)
        to_sort.sort(mode=cplcore.SortMode.BY_Y)
        assert list(to_sort.y) == sorted(to_sort.y)
        to_sort.sort(reverse=True, mode=cplcore.SortMode.BY_Y)
        assert list(to_sort.y) == sorted(to_sort.y, reverse=True)

    random_sizes = [1, 21, 41, 100, 1001]

    @pytest.mark.parametrize("size", random_sizes)
    def test_random_sort(self, size):
        assert size >= 1
        # Create a Bivector of the specified size filled with random numbers
        random_numbers = cplcore.Image.create_noise_uniform(
            size, 2, cplcore.Type.DOUBLE, -1.0, 1.0
        )
        random_bivector = cplcore.Bivector((random_numbers[0], random_numbers[1]))

        # Sort by X and by Y, both ascending and descending, and check that it is sorted.
        random_bivector.sort()
        assert list(random_bivector.x) == sorted(random_bivector.x)
        random_bivector.sort(reverse=True)
        assert list(random_bivector.x) == sorted(random_bivector.x, reverse=True)
        random_bivector.sort(mode=cplcore.SortMode.BY_Y)
        assert list(random_bivector.y) == sorted(random_bivector.y)
        random_bivector.sort(reverse=True, mode=cplcore.SortMode.BY_Y)
        assert list(random_bivector.y) == sorted(random_bivector.y, reverse=True)

    def test_sorted(self):
        xval = [8.0, 1.0, 2.0, 3.0, 4.0, 4.0, 6.0, 7.0, 0]
        yval = [8.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0]
        to_sort = cplcore.Bivector((xval, yval))

        # Invalid direction or mode values should raise TypeError
        with pytest.raises(TypeError):
            _ = to_sort.sorted(reverse="Yes, please")
        with pytest.raises(TypeError):
            _ = to_sort.sorted(mode=2)

        # Sort by X and by Y, both ascending and descending, and check that it is sorted.
        sort_res = to_sort.sorted()
        assert list(sort_res.x) == sorted(sort_res.x)
        sort_res = to_sort.sorted(reverse=True)
        assert list(sort_res.x) == sorted(sort_res.x, reverse=True)
        sort_res = to_sort.sorted(mode=cplcore.SortMode.BY_Y)
        assert list(sort_res.y) == sorted(sort_res.y)
        sort_res = to_sort.sorted(reverse=True, mode=cplcore.SortMode.BY_Y)
        assert list(sort_res.y) == sorted(sort_res.y, reverse=True)

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_bivector_dump.txt"
        filename = tmp_path.joinpath(p)
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        vec = cplcore.Bivector((vec1, vec2))
        vec.dump(filename=str(filename))
        outp = """#--- Bi-vector ---
#       X      Y  
9101	-13.21
89172	62.212
-104	2235
11.1289	2
#-----------------------
"""  # noqa
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_string(self):
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        vec = cplcore.Bivector((vec1, vec2))
        outp = """#--- Bi-vector ---
#       X      Y  
9101	-13.21
89172	62.212
-104	2235
11.1289	2
#-----------------------
"""  # noqa
        assert str(vec) == outp
        assert isinstance(vec.dump(show=False), str)

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "vec1 = cplcore.Vector([9101, 89172, -104, 11.1289]);"
        cmd += "vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2]);"
        cmd += "vec = cplcore.Bivector((vec1, vec2));"
        cmd += "vec.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """#--- Bi-vector ---
#       X      Y  
9101	-13.21
89172	62.212
-104	2235
11.1289	2
#-----------------------
"""  # noqa
        assert outp == expect

    def test_repr(self):
        expect = """cpl.core.Bivector([9101, 89172, -104, 11.1289], [-13.21, 62.212, 2235, 2])"""
        vec1 = cplcore.Vector([9101, 89172, -104, 11.1289])
        vec2 = cplcore.Vector([-13.21, 62.212, 2235, 2])
        vec = cplcore.Bivector((vec1, vec2))
        assert repr(vec) == expect
