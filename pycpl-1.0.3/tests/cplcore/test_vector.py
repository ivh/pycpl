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

import itertools
import math
import random
import re
import sys
import subprocess


from astropy.io import fits
import numpy as np
import pytest

from cpl import core as cplcore


class TestVector:
    @classmethod
    def isclose(cls, first, second, tolerance):
        """Python port of assert cpl_test_vector_macro_abs, but it's a lot less boilerplate :)"""
        assert first.size == second.size
        diff = cplcore.Vector(first)
        diff.subtract(second)
        difval = max(map(abs, diff))

        return abs(difval) <= tolerance

    def test_constructor_leq_zero(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Vector.zeros(0)

    def test_constructor_large(self):
        length = 512 * 1024 * 1024
        v = cplcore.Vector.zeros(length)
        assert len(v) == length

    def test_constructor_emptylist(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Vector([])

    def test_constructor_largelist(self):
        v = cplcore.Vector([5.12, -1923, -19.3124901, 902834012894, 0] * 1024)
        assert len(v) == 5 * 1024

    def test_constructor_sets_elements(self):
        v = cplcore.Vector([9101, 89172, -104, 11.1289])
        assert v[0] == 9101
        assert v[1] == 89172
        assert v[2] == -104
        assert v[3] == 11.1289

    def test_copy_constructor(self):
        v = cplcore.Vector([9101, 89172, -104, 11.1289])
        v2 = cplcore.Vector(v)

        assert len(v) == len(v2)
        assert v[0] == 9101
        assert v[1] == 89172
        assert v[2] == -104
        assert v[3] == 11.1289
        assert v2[0] == 9101
        assert v2[1] == 89172
        assert v2[2] == -104
        assert v2[3] == 11.1289

    def test_read(self, tmp_path):
        vfile = tmp_path.joinpath("vector_1")
        with vfile.open("w+") as vfile_write:
            # (1) Lines beginning with a hash are ignored,
            # (2) blank lines also.
            # (3) In valid lines the value is preceeded by an integer, which is ignored.
            vfile_write.write("89471\n")  # Fails (3)
            vfile_write.write("1 9101\n")  # OK
            vfile_write.write("\n")  # Fails (2)
            vfile_write.write("2 89172\n")  # OK
            vfile_write.write("# 1 2 234Anything\n ")  # Fails (1)
            vfile_write.write(
                " # 1239024 12312\n "
            )  # Fails (1) with leading whitespace
            vfile_write.write("3 -104\n")  # OK
            vfile_write.write("20012\n")  # Fails (3) turns into (1)
            vfile_write.write("4 11.1289\n")  # OK

        v = cplcore.Vector.read(vfile)
        assert len(v) == 4
        assert v[0] == 9101
        assert v[1] == 89172
        assert v[2] == -104
        assert v[3] == 11.1289

    def test_load(self, make_mock_fits):
        file_contents = fits.HDUList(
            [fits.PrimaryHDU(data=np.array([9101, 89172, -104, 11.1289]))]
        )
        fits_file = make_mock_fits(file_contents)

        v = cplcore.Vector.load(fits_file, 0)
        assert len(v) == 4
        assert v[0] == 9101
        assert v[1] == 89172
        assert v[2] == -104
        assert v[3] == 11.1289

    def test_load_badfile(self, tmp_path):
        vfile = tmp_path.joinpath("vector_1")

        with pytest.raises(cplcore.FileIOError):
            _ = cplcore.Vector.load(vfile, 0)

    def test_load_bigbadext(self, make_mock_fits):
        file_contents = fits.HDUList(
            [fits.PrimaryHDU(data=np.array([9101, 89172, -104, 11.1289]))]
        )
        fits_file = make_mock_fits(file_contents)

        with pytest.raises(cplcore.FileIOError):
            _ = cplcore.Vector.load(fits_file, 1)

    def test_load_smallbadext(self, make_mock_fits):
        file_contents = fits.HDUList(
            [fits.PrimaryHDU(data=np.array([9101, 89172, -104, 11.1289]))]
        )
        fits_file = make_mock_fits(file_contents)

        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Vector.load(fits_file, -1)

    def test_load_badfits(self, tmp_path):
        vfile = tmp_path.joinpath("vector_1")

        with vfile.open("w+") as vfile_write:
            vfile_write.write(
                "89471\n1 9101\n\n2 89172\n# 1 2 234Anything\n  # 1239024 12312\n 3 -104\n20012\n4 11.1289\n"
            )

        with pytest.raises(cplcore.FileIOError):
            _ = cplcore.Vector.load(vfile, 0)

    cycles_to_test = [13, 16, 256, 257]

    @pytest.mark.parametrize("n", cycles_to_test)
    def test_cycle(self, n):
        """Shift something non-constant, so the correctness can be verified"""
        src = cplcore.Vector.kernel_profile(cplcore.Kernel.SINC, 1.0, n)

        dest = cplcore.Vector(src)
        dest.cycle(pow(2, 0.5) + n / 3.0)
        dest.cycle(-pow(2, 0.5) - n / 3.0)

        if math.log2(n).is_integer():
            assert self.isclose(dest, src, 0.5 / n)
        else:
            assert self.isclose(dest, src, 10.0 * sys.float_info.epsilon)

        dest = cplcore.Vector(src)
        dest.cycle(0.0)
        assert self.isclose(dest, src, 0.0)

        dest.cycle(0.0)
        assert self.isclose(dest, src, 0.0)

        """ Should not alias input, but it is supported """
        dest = cplcore.Vector(dest)
        dest.cycle(0.0)
        assert self.isclose(dest, src, 0.0)

        dest = cplcore.Vector(src)
        dest.cycle(-1.0)

        if n > 1:
            assert math.isclose(dest[n - 1], src[0], abs_tol=0.0)
            assert math.isclose(dest[0], src[1], abs_tol=0.0)

        dest.cycle(1.0)
        assert self.isclose(dest, src, 0.0)

        dest.cycle(-2.0)

        if n > 2:
            assert math.isclose(dest[n - 2], src[0], abs_tol=0.0)
            assert math.isclose(dest[0], src[2], abs_tol=0.0)

        dest.cycle(2.0)
        assert self.isclose(dest, src, 0.0)

        """ Perform a range of shifts, that will cancel each other out """
        for i in range(-1 - 2 * n, (1 + 2 * n) + 1):  # inclusive end
            dest.cycle(i)

        assert self.isclose(dest, src, 0.0)

        """ Fill the vector with a sine curve
        - any linear combination of full sine and cosine curves
        will be cycled accurately... """

        for i in range(0, n):
            value = math.sin(i * (2 * math.pi) / n)
            src[i] = value

        dest = cplcore.Vector(src)
        dest.cycle(math.e + n / 5.0)

        dest.cycle(-math.e - n / 5.0)

        assert self.isclose(dest, src, 20.0 * sys.float_info.epsilon)

        """ Vectors of length 1 """

        src.size = 1
        dest.size = 1

        dest = cplcore.Vector(src)
        dest.cycle(0.0)
        assert self.isclose(dest, src, 0.0)

        dest = cplcore.Vector(src)
        dest.cycle(1.0)
        assert self.isclose(dest, src, 0.0)

        dest = cplcore.Vector(src)
        dest.cycle(-1.0)
        assert self.isclose(dest, src, 0.0)

    def test_fit_gaussian_test_one(self):
        N = 50
        yval = cplcore.Vector.zeros(N)
        xval = cplcore.Vector.zeros(N)
        ysig = cplcore.Vector.zeros(N)

        in_sigma = 10.0
        in_centre = 25.0
        peak = 769.52
        pos = 0.0

        for n in range(0, N):
            d = pos - in_centre

            xval[n] = pos
            yval[n] = peak * pow(math.e, -d * d / (2.0 * in_sigma * in_sigma))

            """
            the following line seems to make it fail.
            normally, it should have no influence at all since all sigmas
            are the same. strangely, using 1.0/sqrt(N-1) also fails,
            but modifying this value slightly (e.g. by adding 1e-6)
            lets the fitting succeed. is there a meaning in the failure
            for 1.0/sqrt(integer)?
            """

            ysig[n] = 1.0 / pow(N, 0.5)

            pos += 1.0
            """
            create one missing value,
            this has no special meaning, just replicates the generation of
            the test data with which I found the problem
            """
            if n == 34:
                pos += 1.0

        fit_gaussian_output = cplcore.Vector.fit_gaussian(
            xval, yval, ysig, cplcore.FitMode.ALL
        )
        assert fit_gaussian_output.x0 == 25
        assert fit_gaussian_output.sigma == 10
        assert round(fit_gaussian_output.area, 0) == 19289
        assert np.isclose(
            fit_gaussian_output.offset, 1.29569e-15, np.finfo(np.double).eps
        )
        cov = fit_gaussian_output.covariance
        # """ The covariance matrix must be 4 X 4, symmetric, positive definite """
        assert cov.shape == (4, 4)
        assert np.allclose(cov, cov.transpose_create(), np.finfo(np.double).eps)

    modes = [
        cplcore.FitMode.CENTROID,
        cplcore.FitMode.STDEV,
        cplcore.FitMode.AREA,
        cplcore.FitMode.OFFSET,
    ]
    modes2 = [
        mode1 | mode2
        for mode1, mode2 in itertools.combinations(
            [
                cplcore.FitMode.CENTROID,
                cplcore.FitMode.STDEV,
                cplcore.FitMode.AREA,
                cplcore.FitMode.OFFSET,
            ],
            2,
        )
    ]
    modes3 = [
        mode1 | mode2 | mode3
        for mode1, mode2, mode3 in itertools.combinations(
            [
                cplcore.FitMode.CENTROID,
                cplcore.FitMode.STDEV,
                cplcore.FitMode.AREA,
                cplcore.FitMode.OFFSET,
            ],
            3,
        )
    ]

    @pytest.mark.parametrize("mode", modes + modes2 + modes3)
    def test_fit_gaussian_test_errors(self, mode):
        N = 50
        yval = cplcore.Vector.zeros(N)
        xval = cplcore.Vector.zeros(N)
        ysig = cplcore.Vector.zeros(N)

        in_sigma = 10.0
        in_centre = 25.0
        peak = 769.52
        pos = 0.0

        for n in range(0, N):
            d = pos - in_centre

            xval[n] = pos
            yval[n] = peak * pow(math.e, -d * d / (2.0 * in_sigma * in_sigma))

            """
            the following line seems to make it fail.
            normally, it should have no influence at all since all sigmas
            are the same. strangely, using 1.0/sqrt(N-1) also fails,
            but modifying this value slightly (e.g. by adding 1e-6)
            lets the fitting succeed. is there a meaning in the failure
            for 1.0/sqrt(integer)?
            """

            ysig[n] = 1.0 / pow(N, 0.5)

            pos += 1.0
            """
            create one missing value,
            this has no special meaning, just replicates the generation of
            the test data with which I found the problem
            """
            if n == 34:
                pos += 1.0
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Vector.fit_gaussian(xval, yval, ysig, mode)

    def test_fit_gaussian_test_no_centroid(self):
        N = 50
        yval = cplcore.Vector.zeros(N)
        xval = cplcore.Vector.zeros(N)
        ysig = cplcore.Vector.zeros(N)

        in_sigma = 10.0
        in_centre = 25.0
        peak = 769.52
        pos = 0.0

        for n in range(0, N):
            d = pos - in_centre

            xval[n] = pos
            yval[n] = peak * pow(math.e, -d * d / (2.0 * in_sigma * in_sigma))

            """
            the following line seems to make it fail.
            normally, it should have no influence at all since all sigmas
            are the same. strangely, using 1.0/sqrt(N-1) also fails,
            but modifying this value slightly (e.g. by adding 1e-6)
            lets the fitting succeed. is there a meaning in the failure
            for 1.0/sqrt(integer)?
            """

            ysig[n] = 1.0 / pow(N, 0.5)

            pos += 1.0
            """
            create one missing value,
            this has no special meaning, just replicates the generation of
            the test data with which I found the problem
            """
            if n == 34:
                pos += 1.0
        fit_gaussian_output = cplcore.Vector.fit_gaussian(
            xval,
            yval,
            ysig,
            cplcore.FitMode.STDEV | cplcore.FitMode.AREA | cplcore.FitMode.OFFSET,
            x0=25,
        )
        assert fit_gaussian_output.x0 == 25
        assert fit_gaussian_output.sigma == 10
        assert round(fit_gaussian_output.area, 0) == 19289
        assert np.isclose(
            fit_gaussian_output.offset, 1.29569e-15, np.finfo(np.double).eps
        )
        cov = fit_gaussian_output.covariance
        # """ The covariance matrix must be 4 X 4, symmetric, positive definite """
        assert cov.shape == (4, 4)
        assert np.allclose(cov, cov.transpose_create(), np.finfo(np.double).eps)

    def test_fit_gaussian_test_no_stdev(self):
        N = 50
        yval = cplcore.Vector.zeros(N)
        xval = cplcore.Vector.zeros(N)
        ysig = cplcore.Vector.zeros(N)

        in_sigma = 10.0
        in_centre = 25.0
        peak = 769.52
        pos = 0.0

        for n in range(0, N):
            d = pos - in_centre

            xval[n] = pos
            yval[n] = peak * pow(math.e, -d * d / (2.0 * in_sigma * in_sigma))

            """
            the following line seems to make it fail.
            normally, it should have no influence at all since all sigmas
            are the same. strangely, using 1.0/sqrt(N-1) also fails,
            but modifying this value slightly (e.g. by adding 1e-6)
            lets the fitting succeed. is there a meaning in the failure
            for 1.0/sqrt(integer)?
            """

            ysig[n] = 1.0 / pow(N, 0.5)

            pos += 1.0
            """
            create one missing value,
            this has no special meaning, just replicates the generation of
            the test data with which I found the problem
            """
            if n == 34:
                pos += 1.0
        fit_gaussian_output = cplcore.Vector.fit_gaussian(
            xval,
            yval,
            ysig,
            cplcore.FitMode.CENTROID | cplcore.FitMode.AREA | cplcore.FitMode.OFFSET,
            sigma=10,
        )
        assert fit_gaussian_output.x0 == 25
        assert fit_gaussian_output.sigma == 10
        assert round(fit_gaussian_output.area, 0) == 19289
        assert np.isclose(
            fit_gaussian_output.offset, 1.29569e-15, np.finfo(np.double).eps
        )
        cov = fit_gaussian_output.covariance
        # """ The covariance matrix must be 4 X 4, symmetric, positive definite """
        assert cov.shape == (4, 4)
        assert np.allclose(cov, cov.transpose_create(), np.finfo(np.double).eps)

    def test_median(self):
        """Test on unsorted vector"""
        tmp_vec = cplcore.Vector([0, 1, 3, 2])

        assert math.isclose(1.5, tmp_vec.median(), abs_tol=sys.float_info.epsilon)

        """Test on sorted vector"""
        tmp_vec = cplcore.Vector([0, 1, 2, 3])

        assert math.isclose(1.5, tmp_vec.median(), abs_tol=sys.float_info.epsilon)

    def test_size(self):
        """Create the vector sinus"""
        sinus = cplcore.Vector.zeros(256)

        assert sinus.size == 256

    def create_sinus(self):
        """Create the vector sinus"""
        sinus = cplcore.Vector.zeros(256)

        for i in range(256):
            value = math.sin(i * math.pi * 2 / 256)
            sinus[i] = value
            assert sinus[i] == value

        return sinus

    def create_taylor(self):
        """Create a Taylor-expansion of exp()"""
        taylor = cplcore.Vector.zeros(20)
        taylor[0] = 1
        for i in range(1, 20):
            taylor[i] = taylor[i - 1] / i

        return taylor

    def create_cosinus(self):
        """Create the vector cosinus"""
        cosinus = cplcore.Vector.zeros(256)

        for i in range(256):
            cosinus[i] = math.cos(i * math.pi * 2 / 256)

        return cosinus

    def test_exp_horners(self):
        sinus = self.create_sinus()
        taylor = self.create_taylor()

        """Evaluate exp(sinus) using Horners scheme on the Taylor expansion """
        tmp_vec2 = cplcore.Vector.zeros(256)
        tmp_vec2.fill(taylor[19])

        for k in range(19, 0, -1):
            tmp_vec2.multiply(sinus)
            if k % 2 == 1:
                tmp_vec2.add_scalar(taylor[k - 1])
            else:
                tmp_vec2.subtract_scalar(-taylor[k - 1])

        """Verify the result (against cpl_vector_exponential() )"""
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.exponential(math.e)

        tmp_vec2.subtract(tmp_vec)
        tmp_vec2.divide(tmp_vec)
        tmp_vec2.divide_scalar(math.e)

        assert abs(tmp_vec2.max()) <= 2.60831
        assert abs(tmp_vec2.min()) <= 2.03626

    def test_exp_pow_taylor(self):
        sinus = self.create_sinus()
        taylor = self.create_taylor()

        # tmp_vec creation copied in from the function above this one.
        tmp_vec = cplcore.Vector(sinus)
        tmp_vec.exponential(math.e)

        tmp_vec2 = cplcore.Vector.zeros(256)
        tmp_vec2.fill(taylor[0])  # fill with 1

        """
        POLY_SIZE > 20 on alphaev56:
        Program received signal SIGFPE, Arithmetic exception.
        0x200000a3ff0 in cpl_vector_multiply_scalar ()
        """
        for k in range(1, 20):
            vtmp = cplcore.Vector(sinus)
            vtmp.power(k)
            vtmp.multiply_scalar(taylor[k])
            tmp_vec2.add(vtmp)

        """ Much less precise than Horner ..."""
        assert self.isclose(tmp_vec, tmp_vec2, 8 * sys.float_info.epsilon)

    def test_fill(self):
        taylor = self.create_taylor()
        taylor.fill(0)

        assert taylor.max() == 0
        assert taylor.min() == 0

    def test_logarithm(self):
        sinus = self.create_sinus()

        tmp_vec = cplcore.Vector(sinus)
        tmp_vec.exponential(math.e)
        tmp_vec.logarithm(math.e)

        for i in range(256):
            value = sinus[i]
            lerror = value - tmp_vec[i]

            if 2 * i == 256:
                """Value should really be zero"""
                assert math.isclose(value, 0, abs_tol=0.552 * sys.float_info.epsilon)
            else:
                if value != 0:
                    lerror /= value
                assert math.isclose(lerror, 0, abs_tol=330 * sys.float_info.epsilon)

    def test_power(self):
        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.exponential(math.e)

        tmp_vec2 = cplcore.Vector(tmp_vec)

        tmp_vec2.sqrt()
        tmp_vec.power(0.5)

        assert self.isclose(tmp_vec, tmp_vec2, 1.1 * sys.float_info.epsilon)

    def test_power2(self):
        tmp_vec = self.create_sinus()
        tmp_vec.exponential(math.e)

        tmp_vec2 = cplcore.Vector(tmp_vec)
        tmp_vec2.sqrt()

        tmp_vec2.multiply(tmp_vec)
        tmp_vec.power(1.5)

        assert self.isclose(tmp_vec, tmp_vec2, 8 * sys.float_info.epsilon)

    def test_power3(self):
        tmp_vec = self.create_sinus()
        tmp_vec.exponential(math.e)
        tmp_vec.power(1.5)

        tmp_vec2 = cplcore.Vector(tmp_vec)
        tmp_vec.power(2)
        tmp_vec2.divide(tmp_vec)
        tmp_vec.power(-0.5)

        assert self.isclose(tmp_vec, tmp_vec2, 8 * sys.float_info.epsilon)

        tmp_vec.fill(1)
        tmp_vec2.power(0)

        assert self.isclose(tmp_vec, tmp_vec2, 0.0)

    def test_zero_pow_zero(self):
        tmp_vec = cplcore.Vector([1] * 256)
        tmp_vec2 = cplcore.Vector([0] * 256)

        tmp_vec2.power(0)
        assert self.isclose(tmp_vec, tmp_vec2, 0)

    def test_read_fail1(self, tmp_path):
        with pytest.raises(cplcore.FileIOError):
            _ = cplcore.Vector.read(tmp_path.joinpath("/nonexisting"))

    def test_read_fail2(self):
        with pytest.raises(cplcore.BadFileFormatError):
            _ = cplcore.Vector.read("/dev/null")

    def test_read_correct(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_dump.txt")
        sinus = self.create_sinus()

        sinus.dump(filename=filename)

        # Shouldn't have changed anything
        assert self.isclose(self.create_sinus(), sinus, 0)

        tmp_vec = cplcore.Vector.read(filename)

        # This precision is limited by how many ASCII characters the
        # dump has, per vector element.
        assert self.isclose(sinus, tmp_vec, pow(0.1, 6))

    def test_save_badmode(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_dump.txt")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        with pytest.raises(cplcore.IllegalInputError):
            tmp_vec.save(
                filename,
                cplcore.Type.DOUBLE,
                None,
                cplcore.io.CREATE | cplcore.io.EXTEND,
            )

    def test_save_badmode2(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        with pytest.raises(cplcore.IllegalInputError):
            tmp_vec.save(filename, cplcore.Type.DOUBLE, None, cplcore.io.APPEND)

    def test_save_badmode3(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        with pytest.raises(cplcore.IllegalInputError):
            tmp_vec.save(
                filename,
                cplcore.Type.DOUBLE,
                None,
                cplcore.io.CREATE | cplcore.io.APPEND,
            )

    def test_save_ok(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.save(
            filename, cplcore.Type.DOUBLE, None, cplcore.io.CREATE | cplcore.io.CREATE
        )
        with fits.open(filename) as hdu_list:
            hdu_list.verify(option="exception")

    def test_save_load_create(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.save(filename, cplcore.Type.DOUBLE, None, cplcore.io.CREATE)

        tmp_vec2 = cplcore.Vector.load(filename, 0)

        assert self.isclose(tmp_vec, tmp_vec2, 0)

    def test_save_load_extend(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.save(filename, cplcore.Type.DOUBLE, None, cplcore.io.CREATE)
        tmp_vec.save(filename, cplcore.Type.DOUBLE, None, cplcore.io.EXTEND)

        with fits.open(filename) as hdu_list:
            hdu_list.verify(option="exception")

        """Verify that the save/load did not change the vector on 0. ext."""
        tmp_vec2 = cplcore.Vector.load(filename, 0)
        assert self.isclose(tmp_vec, tmp_vec2, 0)

        """Verify that the save/load did not change the vector on 1. ext."""
        tmp_vec2 = cplcore.Vector.load(filename, 1)
        assert self.isclose(tmp_vec, tmp_vec2, 0)

    def test_save_load_large(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_save.fits")

        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        tmp_vec.save(filename, cplcore.Type.DOUBLE, None, cplcore.io.CREATE)

        with filename.open("rb+") as filehandle:
            oldtext = filehandle.read()
            # The unit tests turned 'NAXIS1  =                  256'
            # into 'NAXIS1  =                  255'
            # (No quotes)
            newtext = re.sub(b"(NAXIS1.*)256", lambda c: c.group(1) + b"255", oldtext)
            filehandle.seek(0)
            filehandle.truncate(0)
            filehandle.write(newtext)

        tmp_vec2 = cplcore.Vector.load(filename, 0)
        assert tmp_vec2.size == 255

        for e1, e2 in zip(tmp_vec, tmp_vec2):
            assert e1 == e2

    def test_dump_read_precision(self, tmp_path):
        filename = tmp_path.joinpath("cpl_vector_dump.txt")

        sinus = self.create_sinus()
        sinus.dump(filename=filename)

        tmp_vec = cplcore.Vector.read(filename)

        assert self.isclose(tmp_vec, sinus, 10.0 * np.finfo(np.float32).eps)
        tmp_vec.subtract(sinus)
        assert math.isclose(
            tmp_vec.max() + tmp_vec.min(), 0.0, abs_tol=2.5 * sys.float_info.epsilon
        )

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_vector_dump.txt"
        filename = tmp_path.joinpath(p)
        vec = cplcore.Vector([0, 1, 3, 2])
        vec.dump(filename=str(filename))
        outp = """#----- vector -----
#Index		X
1		0
2		1
3		3
4		2
#------------------
"""
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_string(self):
        vec = cplcore.Vector([0, 1, 3, 2])
        outp = """#----- vector -----
#Index		X
1		0
2		1
3		3
4		2
#------------------
"""
        assert str(vec) == outp
        assert isinstance(vec.dump(show=False), str)

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "vec = cplcore.Vector([0, 1, 3, 2]); "
        cmd += "vec.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """#----- vector -----
#Index		X
1		0
2		1
3		3
4		2
#------------------
"""
        assert outp == expect

    def test_repr(self):
        expect = """cpl.core.Vector([0, 1, 3, 2])"""
        vec = cplcore.Vector([0, 1, 3, 2])
        assert repr(vec) == expect

    def test_duplicate(self):
        sinus = self.create_sinus()
        tmp_vec = cplcore.Vector(sinus)

        assert self.isclose(tmp_vec, sinus, 0.0)

    def test_mean(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        assert math.isclose(tmp_vec.mean(), 1.0, abs_tol=sys.float_info.epsilon)

    def test_sum(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        assert math.isclose(
            tmp_vec.sum(), 256, abs_tol=sys.float_info.epsilon * math.sqrt(256)
        )

    def test_extract(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)
        _ = tmp_vec.extract(0, int(256 / 2), 1)

    def test_extract_illegal1(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        with pytest.raises(cplcore.IllegalInputError):
            tmp_vec.extract(2, 1, 1)

    def test_extract_illegal2(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        with pytest.raises(cplcore.IllegalInputError):
            tmp_vec.extract(1, 2, 2)

    def test_extract_range1(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            tmp_vec.extract(-1, 2, 1)

    def test_extract_range2(self):
        tmp_vec = cplcore.Vector.zeros(256)
        tmp_vec.fill(1.0)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            tmp_vec.extract(0, 256 + 2, 1)

    def test_extract_five(self):
        vxc = cplcore.Vector([1, 2, 3, 4, 5])
        vxc1 = vxc.extract(1, 4, 1)
        vxc = cplcore.Vector([2, 3, 4, 5])
        assert self.isclose(vxc, vxc1, 0.0)

    def test_cosinus_stats(self):
        cosinus = self.create_cosinus()
        # sinus = self.create_sinus()

        assert math.isclose(cosinus.mean(), 0.0, abs_tol=1.68 * sys.float_info.epsilon)
        assert math.isclose(
            cosinus.stdev(),
            math.sqrt((256 / 2) / (256 - 1)),
            abs_tol=(0.36 * sys.float_info.epsilon) * math.sqrt(256),
        )

    def test_cosinus_sinus(self):
        cosinus = self.create_cosinus()
        sinus = self.create_sinus()

        tmp_vec = cplcore.Vector(sinus)
        assert self.isclose(tmp_vec, sinus, 0.0)
        tmp_vec.add(cosinus)
        tmp_vec.subtract(sinus)
        assert self.isclose(tmp_vec, cosinus, sys.float_info.epsilon)
        tmp_vec.subtract_scalar(2)
        tmp_vec.divide(tmp_vec)
        assert tmp_vec.mean() - 1 <= sys.float_info.epsilon

    def test_cosinus_sinus_product(self):
        cosinus = self.create_cosinus()
        sinus = self.create_sinus()

        assert sinus.product(cosinus) <= sys.float_info.epsilon * 256
        assert math.isclose(
            abs(sinus.product(sinus) + cosinus.product(cosinus)),
            256,
            abs_tol=sys.float_info.epsilon * 256,
        )
        # tmp_vec=sinus.filter_lowpass_create(CPL_LOWPASS_LINEAR, 2)
        # assert tmp_vec.size == sinus.size

    def test_copy(self):
        cosinus = cplcore.Vector.zeros(256)

        for i in range(256):
            cosinus[i] = math.cos(i * math.pi * 2 / 256)
        tmp_vec = cosinus.copy()
        assert self.isclose(tmp_vec, cosinus, 0.0)

    def test_filter_median1(self):
        sinus = self.create_sinus()
        tmp_vec = sinus.filter_median_create(2)
        assert tmp_vec.size == 256

    def test_filter_median2(self):
        sinus = self.create_sinus()
        tmp_vec = sinus.filter_median_create(int(256 / 2))
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        assert tmp_vec.size == 256
        if (2 * (256 / 2)) == tmp_vec.size:
            assert self.isclose(tmp_vec, sinus, 0.0)

    def test_filter_median3(self):
        sinus = self.create_sinus()
        with pytest.raises(cplcore.IllegalInputError):
            sinus.filter_median_create(-1)

    def test_filter_median4(self):
        sinus = self.create_sinus()
        with pytest.raises(cplcore.IllegalInputError):
            sinus.filter_median_create(int(1 + (256 / 2)))

    def test_sinus_compars_cosinus(self):
        """sinus <- sqrt(1-sinus^2)"""
        sinus = self.create_sinus()
        sinus.multiply(sinus)
        sinus.multiply_scalar(-1)
        sinus.add_scalar(1)
        sinus.sqrt()

        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)

        """ Compare abs(cosinus) with sqrt(1-sinus^2) """
        assert self.isclose(sinus, cosinus, 10.0 * sys.float_info.epsilon)

    def test_sort_asc(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)
        cosinus.sort()
        for i, val in enumerate(cosinus):
            assert i <= 0 or cosinus[i - 1] <= val

    def test_sort_dsc(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)
        sinus = self.create_sinus()
        sinus = cosinus.copy()
        cosinus.sort()
        sinus.sort(reverse=True)
        for i, val in enumerate(sinus):
            assert i <= 0 or val <= sinus[i - 1]

    def test_sorted_asc(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)
        cos_sorted = cosinus.sorted()
        for i, val in enumerate(cos_sorted):
            assert i <= 0 or cos_sorted[i - 1] <= val

    def test_sort_dsc_two(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)
        sinus = self.create_sinus()
        sinus = cosinus.copy()
        cosinus.sort()
        sin_sorted = sinus.sorted(reverse=True)
        for i, val in enumerate(sin_sorted):
            assert i <= 0 or val <= sin_sorted[i - 1]

    def test_sinus_cosinus_sorted_mean(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)

        sinus = self.create_sinus()
        sinus = cosinus.copy()
        cosinus.sort()
        sinus.sort(reverse=True)

        assert math.isclose(
            abs(cosinus.mean()),
            abs(sinus.mean()),
            abs_tol=15.5 * sys.float_info.epsilon,
        )

    def test_zero_single_sorting(self):
        cosinus = self.create_cosinus()
        for i, val in enumerate(cosinus):
            cosinus[i] = abs(val)

        sinus = self.create_sinus()
        sinus = cosinus.copy()
        cosinus.sort()
        sinus.sort(reverse=True)

        assert math.isclose(
            abs(cosinus.mean()),
            abs(sinus.mean()),
            abs_tol=15.5 * sys.float_info.epsilon,
        )
        tmp = 0.0
        tmp_vec = cplcore.Vector([tmp])
        # DELETED cpl_test_nonnull_macro(tmp_vec)
        tmp_vec.sort()
        assert math.isclose(abs(tmp), abs(0.0), abs_tol=0.0)
        tmp_vec.sort(reverse=True)
        assert math.isclose(abs(tmp), abs(0.0), abs_tol=0.0)

    def test_sinus_single_sorting(self):
        sinus = self.create_cosinus()
        for i, val in enumerate(sinus):
            sinus[i] = abs(val)

        sinus.sort(reverse=True)

        with pytest.raises(TypeError):
            sinus.sort(reverse="Yes, please")

        sinus.size = 1
        sinus[0] = 0.0
        sinus.sort(reverse=True)
        assert math.isclose(abs(sinus[0]), abs(0.0), abs_tol=0.0)
        sinus.sort()
        assert math.isclose(abs(sinus[0]), abs(0.0), abs_tol=0.0)

    def test_correlate1(self):
        sinus = cplcore.Vector.zeros(512)
        for i in range(512):
            sinus[i] = math.sin(i * math.pi * 2 / 256)

        cosinus = cplcore.Vector.zeros(256)
        for i in range(256):
            cosinus[i] = math.cos(i * math.pi * 2 / 256)

        tmp_vec = cplcore.Vector.zeros(255)
        tmp_vec.fill(1.0)

        vxc1 = cplcore.Vector.zeros(1)
        vxc3 = cplcore.Vector.zeros(3)

        with pytest.raises(TypeError):
            cosinus, delta = cplcore.Vector.correlate(sinus, sinus, (256 - 1) / 2)

        with pytest.raises(cplcore.IllegalInputError):
            vxc1, delta = cplcore.Vector.correlate(cosinus, sinus, 0)

        with pytest.raises(cplcore.IllegalInputError):
            vxc3, delta = cplcore.Vector.correlate(cosinus, tmp_vec, 1)

        # *** Calculates a cross correlation when doesn't nothing with it? ***
        vxc1, delta = cplcore.Vector.correlate(cosinus, tmp_vec, 0)

        sinus.multiply_scalar(math.sqrt(2))
        sinus.add_scalar(math.pi)
        cosinus.multiply_scalar(math.sqrt(2))

        # Autocorrelation of scaled, offset sine, at zero offset, should be 1.
        vxc1, delta = cplcore.Vector.correlate(sinus, sinus, 0)
        assert (
            1.0 - vxc1[0] <= 144.0 * sys.float_info.epsilon
        )  # *** Test isn't for abs values! ***

        # Autocorrelation of scaled cosine, at zero offset, should also be 1.
        # *** Why a smaller tolerance in this case, though? ***
        vxc1, delta = cplcore.Vector.correlate(cosinus, cosinus, 0)
        xc = vxc1[0]
        # *** math.isclose takes the absolute values for you! Should be math.isclose(xc, 1.0, abs_tol=...)***
        assert math.isclose(abs(xc), abs(1.0), abs_tol=5.0 * sys.float_info.epsilon)

        # *** emax seems to be a record of the largest error found so far that gets updated after
        # each correlation but the value is never checked! ***
        emax = 0
        if abs(1 - xc) > 0:
            emax = abs(1 - xc)

        if 256 % 2 == 0:  # *** 256 % 2 is always 0! ***
            # Cross correlation of sine and cosine at zero offset should be zero.
            vxc1, delta = cplcore.Vector.correlate(sinus, cosinus, 0)
            xc = vxc1[0]
            assert (
                abs(xc) <= 2.82 * sys.float_info.epsilon
            )  # Yet another tolerance value.
            if abs(xc) > emax:
                emax = abs(xc)

        tmp_vec = cplcore.Vector(cosinus)
        # *** tmp_vec is a copy of cosinus so this test assertion is a check of the copy
        # constructor and nothing to do with correlation. ***
        assert self.isclose(tmp_vec, cosinus, 0.0)

        tmp_vec.divide_scalar(-1)
        vxc1, delta = cplcore.Vector.correlate(tmp_vec, cosinus, 0)
        xc = vxc1[0]
        # Cross correlation of cosine with negated cosine at zero offset should be -1
        # *** Should be math.isclose(xc, -1.0, abs_tol=...)! ***
        assert math.isclose(abs(xc), abs(-1.0), abs_tol=5.0 * sys.float_info.epsilon)
        if abs(1 + xc) > emax:
            emax = abs(1 + xc)

        vxc = cplcore.Vector.zeros(1)
        if 256 % 2 == 0:  # *** 256 % 2 is always 0! ***
            for i in range(int(256 / 4)):
                # This is the previously calculated cross correlation at zero offset of cosine with -cosine, ~ -1
                xcp = xc
                half_search = i + 1
                vxc, delta = cplcore.Vector.correlate(
                    sinus, cosinus, half_search
                )  # Delta is index of maximum cross-correlation
                xc = vxc[
                    delta
                ]  # Maximum value of sine-cosine cross correlation within current offset range
                assert (
                    1 if xc > xcp else 0 != True
                )  # *** This does nothing! Assertion passes regardless of values of xxc, xcp ***
                assert (
                    abs(delta - (i + 1)) == i + 1
                )  # delta should be 0 or 2i, i.e. an offset of -i or + i.

            # At max i the max offset corresponds to pi/2, so cross correlation should be 1.
            # *** Should be math.isclose(xc, 1.0, abs_tol=...) ***
            assert math.isclose(abs(xc), abs(1.0), abs_tol=260 * sys.float_info.epsilon)

            half_search = 256 // 3
            vxc, delta = cplcore.Vector.correlate(sinus, cosinus, half_search)
            xc = vxc[delta]
            assert abs(delta - int(256 / 3)) == 256 / 4
            if abs(1 - xc) > emax:
                emax = abs(1 - xc)

        sinus = cplcore.Vector.zeros(256 - 24)

        for i in range(len(sinus)):
            sinus[i] = math.cos((i * 2 * math.pi) / 256)

        half_search = 256
        vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
        xc = vxc[delta]
        delta -= 256
        assert 0 == delta + int(24 / 2)
        assert math.isclose(abs(xc), abs(1.0), abs_tol=16.5 * sys.float_info.epsilon)
        if abs(1 - xc) > emax:
            emax = abs(1 - xc)

        last_k = 0
        for k in range(1, 24):
            for i in range(len(sinus)):
                sinus[i] = math.cos(((i + k) * 2 * math.pi) / 256)

            vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
            xc = vxc[delta]
            delta -= 256
            assert delta + (24 / 2) == k
            assert math.isclose(
                abs(xc), abs(1.0), abs_tol=18.5 * sys.float_info.epsilon
            )
            if abs(1 - xc) > emax:
                emax = abs(1 - xc)

            last_k = k

        """Continue - maximum xc found with drop"""

        for k in range(last_k, 24):
            half_search = int(k - 24 / 2)

            for i in range(len(sinus)):
                sinus[i] = math.cos((i + k) * math.pi * 2 / 256)

            vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
            xc = vxc[delta]
            delta -= half_search
            assert math.isclose(
                abs(xc), abs(1.0), abs_tol=25.0 * sys.float_info.epsilon
            )
            assert delta + (24 / 2) == k
            if abs(1 - xc) > emax:
                emax = abs(1 - xc)

        """Compare with increasing negative shift and increasing drop of elements
       - only up to half the length-difference """

        half_search = 24
        xc = 1

        for k in range(12):
            xcp = xc

            for i in range(len(sinus)):
                sinus[i] = math.cos(((i - k) * 2 * math.pi) / 256)

            vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
            xc = vxc[delta]
            delta -= half_search
            assert xc <= xcp
            assert 0.0 <= (delta + k) + (24 / 2)

        sinus = cplcore.Vector(cosinus)
        assert self.isclose(sinus, cosinus, 0.0)
        half_search = 24
        vxc, delta = cplcore.Vector.correlate(sinus, cosinus, half_search)
        xc = vxc[delta]
        delta -= half_search
        assert math.isclose(abs(xc), abs(1.0), abs_tol=5.0 * sys.float_info.epsilon)
        if abs(1 - xc) > emax:
            emax = abs(1 - xc)

        vxc_swapped, delta_swapped = cplcore.Vector.correlate(
            cosinus, sinus, half_search
        )
        assert delta + half_search == delta_swapped
        assert xc == vxc_swapped[delta + half_search]
        # DELETED cpl_test_nonnull_macro(data)
        half_search = 256 // 2
        xc = 1

        for k in range(int(256 / 50), 7):
            xcp = xc

            for i in range(0, 256):
                sinus[i] = math.cos(((i + k) * 2 * math.pi) / 256)

            vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
            xc = vxc[delta]
            delta -= half_search
            assert k == delta
            assert 1 if xc < xcp else 0 != True
            vxc, delta = cplcore.Vector.correlate(sinus, cosinus, half_search)
            xcn = vxc[delta]
            delta -= half_search
            assert k == -delta
            assert math.isclose(abs(xcn), abs(xc), abs_tol=7.0 * sys.float_info.epsilon)
            for i in range(0, 256):
                sinus[i] = math.cos(((i - k) * 2 * math.pi) / 256)

            vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
            xc = vxc[delta]
            delta -= half_search
            assert 1 if xc < xcp else 0 != True

        half_search = 256

        for i in range(0, 256):
            sinus[i] = 2.0 * random.random() - 1.0

        cosinus = sinus.copy()
        vxc, delta = cplcore.Vector.correlate(cosinus, sinus, half_search)
        assert delta == half_search
        assert 1.0 - vxc[half_search] <= 3.5 * sys.float_info.epsilon
        half_search = 256 // 2

        for k in range(256 - 2, 2):
            cosinus = cplcore.Vector.zeros(256 - k)
            # DELETED cpl_test_nonnull_macro(pcosinus)

            for i in range(256 - k):
                cosinus[i % cosinus.width][int(i / cosinus.width)] = sinus[i]

            vxc, delta = cplcore.Vector.correlate(sinus, cosinus, half_search)
            xc = vxc[delta]
            delta -= half_search
            assert delta <= 0.0
            assert math.isclose(
                abs(xc), abs(1.0), abs_tol=23.5 * sys.float_info.epsilon
            )
            if abs(1 - xc) > emax:
                emax = abs(1 - xc)
