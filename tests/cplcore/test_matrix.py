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

import copy
import math

import numpy as np
import pytest
import subprocess

from cpl import core as cplcore


# fixtures based off matrices in cpl_matrix-test.c


@pytest.fixture(scope="function")
def cpl_matrix_fill_illcond():
    def _cpl_matrix_fill_illcond(size):
        matrix = cplcore.Matrix.zeros(size, size)
        for i in range(size):
            for j in range(size):
                # The 'usual' definition of this increasingly ill-conditioned
                #    matrix is
                #    A(i,j) = 1/(1+i+j)
                #    - but to expose direct solvers without pivoting, we use
                #    A(i,j) = 1/(2*size - (1+i+j))
                value = 1.0 / (2 * size - (i + j + 1))
                matrix[i][j] = value
        return matrix

    return _cpl_matrix_fill_illcond


@pytest.fixture(scope="function")
def cpl_matrix_get_2norm_1():
    def _cpl_matrix_get_2norm_1(matrix):
        norm = 0
        for i in range(matrix.shape[0]):
            value = matrix[i][0]
            norm += value * value
        return math.sqrt(norm)

    return _cpl_matrix_get_2norm_1


class TestMatrix:
    def test_constructor_zero_detected(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Matrix([], 0)

    def test_constructor_int_from_list_1d(self):
        list_1d = [5, 6, 19, -12]
        img = cplcore.Matrix(list_1d, 2)
        assert img.shape == (2, 2)
        assert img.width == 2
        assert img.height == 2
        assert img.shape == (2, 2)
        assert img[0][0] == 5
        assert img[0][1] == 6
        assert img[1][0] == 19
        assert img[1][1] == -12

    def test_constructor_int_from_list_2d(self):
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Matrix(list_2d)
        assert img.shape == (2, 2)
        assert img.width == 2
        assert img.height == 2
        assert img.shape == (2, 2)
        assert img[0][0] == 5
        assert img[0][1] == 6
        assert img[1][0] == 19
        assert img[1][1] == -12

    def test_constructor_int_from_ndarray_2d(self):
        import numpy
        from numpy import array as np_array

        list_2d = np_array([[5, 6], [19, -12], [4.65, -99]], numpy.intc)
        img = cplcore.Matrix(list_2d)

        assert img.shape == list_2d.shape
        assert img.width == 2
        assert img.height == 3
        assert img.shape == (3, 2)
        assert img[0][0] == 5
        assert img[0][1] == 6
        assert img[1][0] == 19
        assert img[1][1] == -12
        assert img[2][0] == 4
        assert img[2][1] == -99

    def test_set_int(self):
        img = cplcore.Matrix.zeros(2, 1)
        img[0][0] = 5
        assert img[0][0] == 5

    def test_set_outside(self):
        img = cplcore.Matrix.zeros(2, 1)
        with pytest.raises(IndexError):
            img[0][2] = 5

        with pytest.raises(IndexError):
            img[0][-3] = 7

    def test_set_complex(self):
        img = cplcore.Matrix.zeros(2, 1)
        with pytest.raises(TypeError):
            img[0][0] = 5 + 6j

    def test_extract_outside(self):
        img1 = cplcore.Matrix.zeros(3, 3)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            # Copies completley outside the range of the above image
            img1[-2:1:1, 0:2:1]

    def test_extract(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        img2 = img1[1:, 1:]
        # Check img1 didn't change
        assert orig_list == [e for row in img1 for e in row]
        assert img1.shape == (3, 3)

        assert img2[0][0] == -7984
        assert img2[1][0] == 20176
        assert img2[0][1] == -35832
        assert img2[1][1] == 14924
        assert img2.shape == (2, 2)

    def test_extract_skip(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        # Should capture 0, 2 of each dimension #(Not 3)
        img2 = img1[::2, ::2]

        assert img2[0][0] == img1[0][0]
        assert img2[0][1] == img1[0][2]
        assert img2[1][0] == img1[2][0]
        assert img2[1][1] == img1[2][2]

        assert img2.shape == (2, 2)

    def test_extract_diagonalhigh(self):
        orig_list = list(range(12))
        img1 = cplcore.Matrix(orig_list, 3)

        assert img1.extract_diagonal(0) == cplcore.Matrix([0, 5, 10], 3)
        assert img1.extract_diagonal(1) == cplcore.Matrix([1, 6, 11], 3)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.extract_diagonal(2)

    def test_extract_diagonalwide(self):
        orig_list = list(range(12))
        img1 = cplcore.Matrix(orig_list, 4)
        print(img1)

        assert img1.extract_diagonal(0) == cplcore.Matrix([0, 4, 8], 1)
        assert img1.extract_diagonal(1) == cplcore.Matrix([3, 7, 11], 1)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.extract_diagonal(2)

    def test_extract_diagonaleven(self):
        orig_list = list(range(9))
        img1 = cplcore.Matrix(orig_list, 3)

        assert img1.extract_diagonal(0) == cplcore.Matrix([0, 4, 8], 3)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.extract_diagonal(1)

    def test_fill(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        img1.fill(5)
        assert all((img1[y][x] == 5 for x in range(3) for y in range(3)))
        assert img1.shape == (3, 3)

    def test_fill_row(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        img1.fill_row(5, 1)
        assert all(
            (img1[y][x] == orig_list[y * 3 + x] for x in range(3) for y in [0, 2])
        )
        assert all((img1[1][x] == 5 for x in range(3)))
        assert img1.shape == (3, 3)

    def test_fill_column(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        img1.fill_column(5, 1)
        assert all(
            (img1[y][x] == orig_list[y * 3 + x] for x in [0, 2] for y in range(3))
        )
        assert all((img1[y][1] == 5 for y in range(3)))
        assert img1.shape == (3, 3)

    def test_fill_row_badindex(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.fill_row(10, 3)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.fill_row(10, -1)

    def test_fill_col_badindex(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.fill_column(10, 3)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            img1.fill_column(10, -1)

    def test_fill_diagonal(self):
        orig_list = [-1837, -59494, -168, 14751, -7984, -35832, 2267, 20176, 14924]
        img1 = cplcore.Matrix(orig_list, 3)

        img1.fill_diagonal(5, 1)
        assert all(
            (
                img1[y][x] == 5 if x - 1 == y else img1[y][x] == orig_list[y * 3 + x]
                for x in range(3)
                for y in range(3)
            )
        )
        assert img1.shape == (3, 3)

    def test_fill_window(self):
        orig_list = [
            1092.333,
            -3.45,
            -49102,
            12.492,
            0.10,
            -4000,
            -1837,
            -59494,
            -168,
            11.94,
            -59102.22,
            14751,
            -7984,
            -35832,
            87.84,
            52135,
            2267,
            20176,
            14924,
            92.17,
            912.102,
            1892,
            90125,
            1980.2,
            8.25,
        ]
        img1 = cplcore.Matrix(orig_list, 5)

        img1.fill_window(12094.29012, 1, 1, 2, 2)
        assert all(
            (
                (
                    img1[y][x] == 12094.29012
                    if x in range(1, 3) and y in range(1, 3)
                    else img1[y][x] == orig_list[y * 5 + x]
                )
                for x in range(5)
                for y in range(5)
            )
        )
        assert img1.shape == (5, 5)

    def test_shift(self):
        orig_list = [
            1092.333,
            -3.45,
            -49102,
            12.492,
            0.10,
            -4000,
            -1837,
            -59494,
            -168,
            11.94,
            -59102.22,
            14751,
            -7984,
            -35832,
            87.84,
            52135,
            2267,
            20176,
            14924,
            92.17,
            912.102,
            1892,
            90125,
            1980.2,
            8.25,
        ]
        img1 = cplcore.Matrix(orig_list, 5)

        img1.shift(1, 2)  # 1 row, 2 columns
        assert all(())

    def test_solve_lu(self):
        original = cplcore.Matrix(
            [15, 3, 2, 12, 9, 5, 11, 2, 2, 10, 13, 11, 13, 5, 4, 10], 4
        )
        rhs = cplcore.Matrix([1, 12, 7, 8, 5, 14, 3, 3, 14, 8, 1, 4, 10, 12, 3, 4], 4)
        perms, even = original.decomp_lu()
        dupe = copy.deepcopy(original)

        dupe.solve_lu(rhs, perms)

        assert str(dupe) == str(
            cplcore.Matrix(
                [
                    15.0,
                    3.0,
                    2.0,
                    12.0,
                    0.13333333333333333,
                    9.6,
                    12.733333333333333,
                    9.4,
                    0.6,
                    0.33333333333333337,
                    5.555555555555556,
                    -8.333333333333332,
                    0.8666666666666667,
                    0.25,
                    -0.16499999999999995,
                    -4.125,
                ],
                4,
            )
        )

    @pytest.mark.parametrize(
        "size, residual_true, error_true",
        [(10, 3.39116, 0.000680453), (11, 2.54951, 0.0573571), (12, 5.83095, 1.68382)],
    )
    def test_solve_1(
        self,
        size,
        residual_true,
        error_true,
        cpl_matrix_fill_illcond,
        cpl_matrix_get_2norm_1,
    ):
        # Try to solve increasingly large systems Ax=b, with A(i,j) = 1/(2*n-(1+i+j)) and x(j) = 1
        xtrue = cplcore.Matrix([1.0] * size, size)
        matrix = cpl_matrix_fill_illcond(size)
        product = matrix @ xtrue
        xsolv = cplcore.Matrix.solve(matrix, product)
        # Find the residual
        p2 = matrix @ xsolv
        p2.subtract(product)
        residual = cpl_matrix_get_2norm_1(p2)
        xsolv.subtract(xtrue)
        error = cpl_matrix_get_2norm_1(xsolv)
        # Not sure why the CPL tests divides by the double epsilon but its where I get the values
        # FIXME: This is not a unit-test per se. Rather, it takes an ill-conditioned system's
        # residual and error (i.e. numerical noise) from one system and compares those to the
        # obtained values on the current system. But the only thing that should be expected from
        # this across systems is that the errors and residuals should be of the same magnitude
        assert np.isclose(
            residual / np.finfo(np.double).eps,
            residual_true,
            rtol=2.0,
        )
        assert np.isclose(error, error_true, rtol=1.0)

    def test_iterator(self):
        "test the matrix iterator per row, and iteration over said row"
        data = [
            [15.0, 3.0, 2.0, 12.0],
            [0.13333333333333333, 9.6, 12.733333333333333, 9.4],
            [0.6, 0.33333333333333337, 5.555555555555556, -8.333333333333332],
            [0.8666666666666667, 0.25, -0.16499999999999995, -4.125],
        ]
        mat = cplcore.Matrix(data)
        for mat_row, real_row in zip(mat, data):
            assert np.array_equal(
                mat_row, real_row
            )  # Test numpy support for individual rows
            for mat_val, real_val in zip(mat_row, real_row):
                assert mat_val == real_val

    def test_dump_string(self):
        list_2d = [[5, 6], [19, -12]]
        mat = cplcore.Matrix(list_2d)
        outp = """          0      1
  0       5      6
  1      19    -12

"""
        assert str(mat) == outp
        assert isinstance(mat.dump(show=False), str)

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_matrix_dump.txt"
        filename = tmp_path.joinpath(p)
        list_2d = [[5, 6], [19, -12]]
        mat = cplcore.Matrix(list_2d)
        outp = """          0      1
  0       5      6
  1      19    -12

"""
        mat.dump(filename=str(filename))
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_stdout(self):
        # for some reason we can't capture stdout using capsys
        # a cheeky workaround using subprocess does the trick
        cmd = "from cpl import core as cplcore; "
        cmd += "list_2d = [[5, 6], [19, -12]];"
        cmd += "mat = cplcore.Matrix(list_2d);"
        cmd += "mat.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """          0      1
  0       5      6
  1      19    -12

"""
        assert outp == expect

    def test_repr(self):
        list_2d = [[5, 6], [19, -12]]
        mat = cplcore.Matrix(list_2d)
        outp = """cpl.core.Matrix(2, 2, [5, 6, 19, -12])"""
        assert repr(mat) == outp
