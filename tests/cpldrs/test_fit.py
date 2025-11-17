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

from copy import deepcopy
import math

import numpy as np
import pytest

import cpl


def imagelist_polynomial_window_test(
    x_pos, values, window, mindeg, maxdeg, is_symsamp, pixeltype, fiterror
):
    self_list = cpl.drs.fit.imagelist_polynomial(
        x_pos,
        values,
        mindeg,
        maxdeg,
        is_symsamp,
        pixeltype,
        fiterror=fiterror,
        window=window,
    )
    npos = len(x_pos)
    icopy = deepcopy(values)
    vcopy = cpl.core.Vector.zeros(npos + 1)
    bad = icopy[0].duplicate()
    mx = bad.width
    my = bad.height
    mask = cpl.core.Mask(mx, my)
    baderror = fiterror.duplicate() if fiterror else None
    if pixeltype == cpl.core.Type.INT:
        tol = 0.0
    else:
        tol = 30.0 * np.finfo(np.single).eps
    icopy.append(bad)
    mask = ~mask
    bad.reject_from_mask(mask)
    assert bad.count_rejected() == mx * my
    assert len(icopy) == npos + 1
    assert len(vcopy) == npos + 1
    for i, value in enumerate(x_pos):
        vcopy[i] = x_pos[i]
    vcopy[npos] = np.finfo(np.double).max
    fitbad = cpl.drs.fit.imagelist_polynomial(
        vcopy, icopy, mindeg, maxdeg, is_symsamp, pixeltype, baderror, window
    )
    for selfim, fitbadim in zip(self_list, fitbad):
        assert np.allclose(selfim, fitbadim, atol=tol)
    if fiterror:
        assert np.allclose(fiterror, baderror, atol=tol)
    return self_list


def eval_gauss(x, a):
    CPL_MATH_2PI = 6.2831853071795864769252867665590057683943387987502116
    if a[5] == 0.0:
        if x[0] == a[3]:
            return np.finfo(np.double).max
        else:
            return 0.0
    elif a[6] == 00:
        if x[1] == a[4]:
            return np.finfo(np.double).max
        else:
            return 0.0
    else:
        b1 = -0.5 / (1 - a[2] * a[2])
        b2 = (x[0] - a[3]) / a[5]
        b3 = (x[1] - a[4]) / a[6]
        return a[0] + a[1] / (
            CPL_MATH_2PI * a[5] * a[6] * np.sqrt(1 - a[2] * a[2])
        ) * np.exp(b1 * (b2 * b2 - 2 * a[2] * b2 * b3 + b3 * b3))


def py_vector_fit_gaussian(x, y, sigma_y):
    """
    Recreation of cpl_vector_fit_gaussian
    """
    CPL_MATH_SQRT2PI = 2.5066282746310005024157652848110452530069867406099383

    def gauss(x, a: list):
        my, sigma = a[0], a[1]
        if sigma != 0:
            A, B = a[2], a[3]
            return B + A / (CPL_MATH_SQRT2PI * math.fabs(sigma)) * math.exp(
                -(x[0] - my) * (x[0] - my) / (2 * sigma * sigma)
            )
        else:
            return 0.0 if x[0] != my else np.finfo(np.double).max

    def gauss_derivative(x, a):
        result = np.empty(len(a))
        if a[1] != 0.0:
            my, sigma, A = a[0], a[1], a[2]
            # a[3] not used
            factor = math.exp(-(x[0] - my) * (x[0] - my) / (2.0 * sigma * sigma)) / (
                CPL_MATH_SQRT2PI * math.fabs(sigma)
            )
            result[0] = A * factor * (x[0] - my) / (sigma * sigma)
            result[1] = (
                A * factor * ((x[0] - my) * (x[0] - my) / (sigma * sigma) - 1) / sigma
            )
            result[2] = factor
            result[3] = 1.0
        else:
            result.fill(0.0)
        return result

    # Initial parameter values
    x0_guess = 0
    sigma_guess = 0
    N = x.size
    x_matrix = cpl.core.Matrix(x, rows=N)
    # /* Compute guess parameters using robust estimation
    #  * (This step might be improved by taking into account the sigmas,
    #  *  but for simplicity do not)
    summ = 0.0
    fraction = [0.25, 0.5, 0.75]
    quartile = [0, 0, 0]

    y_dup = list(y)
    offset_guess = sorted(y_dup)[int(N / 4)]

    #    The algorithm requires the input to be sorted
    #    as function of x, so do that (using qsort), and
    #    work on the sorted copy. Of course, the y-vector
    #    must be re-ordered along with x.
    #    sigma_x and sigma_y are not used, so don't copy those.
    fit_gaussian_input = np.array(
        sorted(list(zip(np.array(x), np.array(y))), key=lambda pair: pair[0])
    ).transpose()
    x_data = fit_gaussian_input[0]
    y_data = fit_gaussian_input[1]

    for i in range(N):
        flux = y_data[i]
        summ += flux - offset_guess

    if summ > 0.0:
        running_sum = 0.0
        i = 0
        flux = y[i] - offset_guess
        for j in range(3):
            limit = fraction[j] * summ
            while running_sum + flux < limit and i < N - 1:
                running_sum += flux
                i += 1
                flux = y[i] - offset_guess
            # Fraction [0;1] of current flux needed to reach the quartile
            k = (limit - running_sum) / flux
            if k <= 0.05:
                if 0 < i:
                    x1 = x_data[i - 1]
                    x2 = x_data[i]
                    quartile[j] = x1 * (0.5 - k) + x2 * (0.5 + k)
                else:
                    quartile[j] = x_data[i]
            else:
                # Interpolate linearly between current and next position */
                if i < N - 1:
                    x1 = x_data[i]
                    x2 = x_data[i + 1]
                    quartile[j] = x1 * (1.5 - k) + x2 * (-0.5 + k)
                else:
                    quartile[j] = x_data[i]
    else:
        # If there's no flux (sum = 0) then set quartiles to something that's not completely insensible.
        quartile[1] = x_matrix.mean()
        quartile[2] = quartile[1]
        quartile[0] = quartile[2] - 1.0

    # x0_guess = median of distribution
    x0_guess = quartile[1]

    sigma_guess = (quartile[2] - quartile[0]) / (2 * 0.6744)
    area_guess = (y.max() - offset_guess) * CPL_MATH_SQRT2PI * sigma_guess

    # ensure area/sigma are positive
    area_guess = 1.0 if area_guess <= 0 else area_guess
    sigma_guess = 1.0 if sigma_guess <= 0 else sigma_guess
    a = cpl.core.Vector([x0_guess, sigma_guess, area_guess, offset_guess])

    ia = [True, True, True, True]  # On everything
    res = cpl.drs.fit.lvmq(
        x_matrix,
        y,
        a,
        gauss,
        gauss_derivative,
        sigma_y=sigma_y,
        participating_parameters=ia,
    )
    return res


class TestFit:
    def test_fit_polynomial_bpm_perfect_linear_fit(self):
        # Test 1: perfectly linear fit
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        mx = 2  # Two pixels, one all good, one mixed
        my = 1
        dfiterror = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
        imlist = cpl.core.ImageList()

        for i in range(ndits):
            value = ditval[i]
            bad = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
            bad[0][0] = value
            bad[0][1] = value
            imlist.append(bad)
            if i & 1:
                bad.reject(0, 0)
        fit = cpl.drs.fit.imagelist_polynomial(
            vdit, imlist, 0, 1, False, cpl.core.Type.DOUBLE, dfiterror
        )
        assert len(fit) == 2
        assert np.isclose(fit[0][0][0], 0.0, atol=4.0 * np.finfo(np.double).eps)
        assert np.isclose(fit[1][0][0], 1.0, atol=2.0 * np.finfo(np.double).eps)
        assert np.isclose(dfiterror[0][0], 0.0, atol=np.finfo(np.double).eps)

    def test_fit_polynomial_bpm_perfect_quadratic_fit(self):
        # Test 2: perfectly quadratic fit
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        mx = 2  # Two pixels, one all good, one mixed
        my = 1
        dfiterror = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
        imlist = cpl.core.ImageList()

        for i in range(ndits):
            value = ditval[i] * ditval[i]
            bad = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
            bad[0][0] = value
            bad[0][1] = value
            imlist.append(bad)
            if i & 1:
                bad.reject(0, 0)
        fit = cpl.drs.fit.imagelist_polynomial(
            vdit, imlist, 0, 2, False, cpl.core.Type.DOUBLE, dfiterror
        )

        assert len(fit) == 3
        assert np.isclose(fit[0][0][0], 0.0, atol=12.0 * np.finfo(np.double).eps)
        assert np.isclose(fit[1][0][0], 0.0, atol=24.0 * np.finfo(np.double).eps)
        assert np.isclose(fit[2][0][0], 1.0, atol=2.0 * np.finfo(np.double).eps)
        assert np.isclose(dfiterror[0][0], 0.0, atol=np.finfo(np.double).eps)

    def test_fit_polynomial_bpm_perfect_quadratic_fit_noterms(self):
        # Test 3: A perfectly quadratic fit, without the constant+linear terms
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        mx = 2  # Two pixels, one all good, one mixed
        my = 1
        dfiterror = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
        imlist = cpl.core.ImageList()

        for i in range(ndits):
            value = ditval[i] * ditval[i]
            bad = cpl.core.Image.zeros(mx, my, cpl.core.Type.DOUBLE)
            bad[0][0] = value
            bad[0][1] = value
            imlist.append(bad)
            if i & 1:
                bad.reject(0, 0)
        fit = cpl.drs.fit.imagelist_polynomial(
            vdit, imlist, 2, 2, False, cpl.core.Type.DOUBLE, dfiterror
        )

        assert len(fit) == 1
        assert np.isclose(fit[0][0][0], 1.0, atol=2.0 * np.finfo(np.double).eps)
        assert np.isclose(dfiterror[0][0], 0.0, atol=np.finfo(np.double).eps)

    def test_polynomial_window_incompatible_input(self):
        IMAGESZFIT = 256
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.FLOAT)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.INT)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.FLOAT_COMPLEX)
        _ = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        _ = 5.52 * np.finfo(np.single).eps
        pixel_type = [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT]
        _ = len(pixel_type)
        input_list = cpl.core.ImageList()
        with pytest.raises(cpl.core.IncompatibleInputError):
            imagelist_polynomial_window_test(
                vdit,
                input_list,
                (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
                0,
                0,
                False,
                cpl.core.Type.FLOAT,
                None,
            )

    @pytest.mark.parametrize(
        "test_type", [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT]
    )
    def test_polynomial_window_types_linear(self, test_type):
        IMAGESZFIT = 256
        IMAGESZ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZ, test_type)
        input_list.append(image)
        image = cpl.core.Image.create_noise_uniform_like(image, 1.0, 20.0)
        image.multiply_scalar(ditval[1])
        input_list.append(image)

        base = input_list[1]
        # A perfectly linear set
        for i in range(2, ndits):
            new_im = base.duplicate()
            new_im.multiply_scalar(ditval[i])
            input_list.append(new_im)
        fit = imagelist_polynomial_window_test(
            vdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZ - 1),
            1,
            ndits - 1,
            False,
            test_type,
            None,
        )  # Line 269
        assert len(fit) == ndits - 1
        # The linarity must be equal to the values in image 1 - normalize
        fit[0].divide(input_list[1])
        fit[0].subtract_scalar(1.0)
        for im in fit:
            assert np.all(
                np.isclose(im, 0, atol=IMAGESZFIT * mytol)
            )  # Check if all images are full of zeroes

    @pytest.mark.parametrize(
        "test_type", [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT]
    )
    def test_polynomial_window_types_2(self, test_type):
        """
        Repeat with non-equidistant but symmetric x-axis values
        and check validity of the eq_dist in this case
        """
        IMAGESZFIT = 256
        IMAGESZ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        ndits = len(ditval)
        _ = cpl.core.Vector(ditval)
        nvdit = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        image = cpl.core.Image.create_noise_uniform(
            IMAGESZFIT, IMAGESZ, test_type, 1.0, 20.0
        )
        image.multiply_scalar(neqditval[0])
        input_list.append(image)
        base = input_list[0]
        # A perfectly linear set
        for i in range(1, ndits):
            new_im = base.duplicate()
            new_im.multiply_scalar(neqditval[i])
            input_list.append(new_im)
        # Fit without is_eqdist option
        fit = imagelist_polynomial_window_test(
            nvdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZ - 1),
            1,
            ndits - 1,
            False,
            test_type,
            None,
        )  # Line 269
        assert len(fit) == ndits - 1

        # Repeat with eq_dist
        fit_eq = imagelist_polynomial_window_test(
            nvdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZ - 1),
            1,
            ndits - 1,
            True,
            test_type,
            None,
        )

        for fit_im, fit_eq_im in zip(fit, fit_eq):
            assert np.allclose(fit_im, fit_eq_im, atol=mytol)

    def test_polynomial_window_2nd_order_eqdist(self):
        # 2nd order function check with and without eq_dist
        IMAGESZFIT = 256
        _ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.FLOAT)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.INT)
        _ = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.FLOAT_COMPLEX)
        ndits = len(ditval)
        _ = cpl.core.Vector(ditval)
        nvdit = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        for i in range(0, ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(neqditval[i] * neqditval[i])
            input_list.append(image)
        fit = imagelist_polynomial_window_test(
            nvdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
            1,
            ndits,
            False,
            cpl.core.Type.FLOAT,
            None,
        )

        fit_eq = imagelist_polynomial_window_test(
            nvdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
            1,
            ndits,
            True,
            cpl.core.Type.FLOAT,
            None,
        )
        for fit_im, fit_eq_im in zip(fit, fit_eq):
            assert np.allclose(fit_im, fit_eq_im, atol=mytol)

    def test_polynomial_window_integer_error_1(self):
        """
        Fit with zero-order term
        Also, try to use an integer-type image for fitting error
        """
        IMAGESZFIT = 256
        _ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        ifiterror = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.INT)
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        # Create a list of images with a 2nd order function
        for i in range(ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(ditval[i] * ditval[i])
            input_list.append(image)
        # Fit with zero-order term
        # Also, try to use an integer-type image for fitting error
        fit = imagelist_polynomial_window_test(
            vdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
            0,
            2,
            True,
            cpl.core.Type.FLOAT,
            ifiterror,
        )
        assert len(fit) == 3
        assert np.all(
            np.isclose(ifiterror, 0.0, atol=mytol)
        )  # Check if all images are full of zeroes

        fit[2].subtract_scalar(1.0)
        for im in fit:
            assert np.all(np.isclose(im, 0.0, atol=mytol))

    def test_polynomial_window_integer_error_2(self):
        """
        Fit with zero-order term
        Also, try to use an integer-type image for fitting error
        """
        IMAGESZFIT = 256
        _ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        ifiterror = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.INT)
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        # Create a list of images with a 2nd order function
        for i in range(ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(ditval[i] * ditval[i])
            input_list.append(image)
        # Fit with zero-order term
        # Also, try to use an integer-type image for fitting error
        fit = imagelist_polynomial_window_test(
            vdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
            0,
            ndits - 1,
            True,
            cpl.core.Type.FLOAT,
            ifiterror,
        )
        assert len(fit) == ndits
        assert np.all(
            np.isclose(ifiterror, 0.0, atol=mytol)
        )  # Check if all images are full of zeroes

        fit[2].subtract_scalar(1.0)
        for im in fit:
            assert np.all(np.isclose(im, 0.0, atol=mytol))

    def test_polynomial_window_double_error(self):
        """
        Fit with zero-order term
        Also, try to use an integer-type image for fitting error
        """
        IMAGESZFIT = 256
        _ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        dfiterror = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        # Create a list of images with a 2nd order function
        for i in range(ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(ditval[i] * ditval[i])
            input_list.append(image)
        # Fit with zero-order term
        # Also, try to use an integer-type image for fitting error
        fit = imagelist_polynomial_window_test(
            vdit,
            input_list,
            (0, 0, IMAGESZFIT - 1, IMAGESZFIT - 1),
            1,
            ndits - 1,
            False,
            cpl.core.Type.FLOAT,
            dfiterror,
        )
        assert len(fit) == ndits - 1

        fit[1].subtract_scalar(1.0)
        for im in fit:
            assert np.all(np.isclose(im, 0.0, atol=mytol))

    def test_polynomial_complex(self):
        """
        Complex types unsupported
        """
        IMAGESZFIT = 256
        _ = 10
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]
        ffiterror = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.FLOAT)
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        _ = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        # Create a list of images with a 2nd order function
        for i in range(ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(ditval[i] * ditval[i])
            input_list.append(image)
        # valid fit, but fitting type is complex
        with pytest.raises(cpl.core.UnsupportedModeError):
            _ = cpl.drs.fit.imagelist_polynomial(
                vdit,
                input_list,
                1,
                ndits - 1,
                False,
                cpl.core.Type.FLOAT_COMPLEX,
                ffiterror,
            )

    def test_polynomial_window_double_error_2(self):
        """
        A valid fit, except that the fitting type is complex
        """
        IMAGESZFIT = 256
        IMAGESZ = 10
        WINSTART = 3
        ditval = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]
        neqditval = [1.0, 2.0, 4.0, 6.0, 10.0, 14.0, 16.0, 18.0, 19.0]

        dfiterrorwin = cpl.core.Image.zeros(
            IMAGESZFIT - 2 * WINSTART + 1,
            IMAGESZ - 2 * WINSTART + 1,
            cpl.core.Type.DOUBLE,
        )
        ndits = len(ditval)
        vdit = cpl.core.Vector(ditval)
        _ = cpl.core.Vector(neqditval)
        _ = cpl.core.Polynomial(1)
        _ = 204.0  # sum of squares of ditvals
        mytol = 5.52 * np.finfo(np.single).eps
        input_list = cpl.core.ImageList()
        # Create a list of images with a 2nd order function
        for i in range(ndits):
            image = cpl.core.Image.zeros(IMAGESZFIT, IMAGESZFIT, cpl.core.Type.DOUBLE)
            image.add_scalar(ditval[i] * ditval[i])
            input_list.append(image)
        # valid fit, but fitting type is complex
        fit = imagelist_polynomial_window_test(
            vdit,
            input_list,
            (
                WINSTART - 1,
                WINSTART - 1,
                IMAGESZFIT - WINSTART - 1,
                IMAGESZ - WINSTART - 1,
            ),
            1,
            ndits - 1,
            False,
            cpl.core.Type.FLOAT,
            dfiterrorwin,
        )
        assert len(fit) == ndits - 1
        assert fit[0].width == IMAGESZFIT - 2 * WINSTART + 1
        assert fit[0].height == IMAGESZ - 2 * WINSTART + 1

        fit[1].subtract_scalar(1.0)

        for im in fit:
            assert np.all(np.isclose(im, 0.0, atol=mytol))
        assert np.all(np.isclose(dfiterrorwin, 0.0, atol=mytol))

    @pytest.mark.parametrize(
        "pixeltype", [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT]
    )
    def test_image_gaussian_tests(self, pixeltype):
        CPL_MATH_DEG_RAD = 57.295779513082320876798154814105170332405472466564322
        nx = 30
        ny = 30
        lnx = 2438
        lny = 2438
        # Parameter values of simulated gaussian
        u = [
            2.130659,
            104274.700696,
            0.779320,
            796.851741,
            1976.324361,
            4.506552,
            3.248286,
        ]
        # Errors on parameters of simulated gaussian
        _ = [2.029266, 955.703656, 0.004452, 0.035972, 0.025949, 0.037186, 0.026913]
        # Physical parameter values of simulated gaussian
        fu = [33.422720, 5.276152, 1.738563]
        # Errors on physical parameters of gaussian
        fe = [0.238359, 0.283202, 0.052671]

        # Fill image with model gaussian
        box = cpl.core.Image.zeros(nx, ny, pixeltype)
        u[3] -= 782
        u[4] -= 1961
        x = [0, 0]
        for j in range(0, ny):
            x[1] = j + 1
            for i in range(0, nx):
                x[0] = i + 1
                value = eval_gauss(x, u)
                box[j][i] = value
        u[3] += 782
        u[4] += 1961
        _ = 1e-5

        # Insert it into a bigger image
        image = cpl.core.Image.zeros(lnx, lny, pixeltype)
        image.copy_into(box, 1962, 783)

        # Add some noise, and create corresponding error image
        eimage = cpl.core.Image.create_noise_uniform(lnx, lny, pixeltype, -50, 50)
        image.add(eimage)
        eimage = cpl.core.Image.create_noise_uniform(lnx, lny, pixeltype, 50, 60)

        # Check if gaussian can be found with any firs guesses
        res = cpl.drs.fit.image_gaussian(
            image,
            797,
            1976,
            30,
            30,
            guesses=u,
            fit_params=[True, True, True, True, True, True, True],
            errors=eimage,
        )
        assert res.phys_cov.shape == (3, 3)
        assert (
            abs(res.angle * CPL_MATH_DEG_RAD - fu[0]) < 3.0 * fe[0] * CPL_MATH_DEG_RAD
        )
        assert abs(res.major - fu[1]) < 3.0 * fe[1]
        assert abs(res.minor - fu[2]) < 3.0 * fe[2]

    @pytest.mark.parametrize(
        "value, major, minor, angle, start_guess, true_params",
        [
            (
                [
                    111,
                    477,
                    1596,
                    1263,
                    194,
                    64,
                    255,
                    970,
                    3210,
                    2640,
                    469,
                    141,
                    1565,
                    8840,
                    30095,
                    17625,
                    2381,
                    656,
                    4982,
                    33104,
                    65535,
                    63588,
                    7276,
                    2377,
                    1822,
                    12167,
                    37548,
                    20715,
                    2597,
                    856,
                    353,
                    1440,
                    4042,
                    3121,
                    535,
                    135,
                ],  # image
                5.80423,
                2.82963,
                0.865826,  # major, minor and angle
                None,  # First test does not have any initial guesses,
                [
                    5.63579e03,
                    8.63334e04,
                    6.10972e-01,
                    2.92544e00,
                    4.24091e00,
                    4.33489e00,
                    4.78587e00,
                ],
            ),  # Expected output params
            (
                [
                    408,
                    880,
                    1043,
                    719,
                    249,
                    93,
                    1512,
                    2730,
                    2253,
                    1159,
                    470,
                    212,
                    11366,
                    22069,
                    18434,
                    7102,
                    1898,
                    789,
                    15630,
                    52992,
                    65535,
                    43330,
                    11619,
                    2834,
                    3422,
                    13283,
                    31354,
                    32311,
                    13946,
                    3225,
                    616,
                    1717,
                    3143,
                    3298,
                    2029,
                    662,
                ],  # image
                1.25846,
                0.661684,
                0.318316,  # major, minor and angle
                5.63579e03,  # starting first param from CPL test
                [
                    6.34119e02,
                    3.58694e05,
                    3.78597e-01,
                    2.95906e00,
                    4.12122e00,
                    1.21305e00,
                    7.41664e-01,
                ],
            ),  # Expected output params
            (
                [
                    161,
                    587,
                    1618,
                    1487,
                    409,
                    103,
                    475,
                    1681,
                    4511,
                    4295,
                    1302,
                    308,
                    2009,
                    10686,
                    40660,
                    42502,
                    10990,
                    1559,
                    4526,
                    26557,
                    65535,
                    65535,
                    16232,
                    2942,
                    1136,
                    6054,
                    17506,
                    13985,
                    3569,
                    697,
                    249,
                    1057,
                    2944,
                    2536,
                    633,
                    115,
                ],
                0.9219,
                0.714119,
                -0.13699,
                6.34119e02,
                [
                    6.80926e02,
                    3.32853e05,
                    -6.96825e-02,
                    3.45315e00,
                    3.74858e00,
                    9.18455e-01,
                    7.18544e-01,
                ],
            ),
            (
                [
                    684,
                    1620,
                    2240,
                    2021,
                    1041,
                    346,
                    3549,
                    8272,
                    11679,
                    9524,
                    4789,
                    1661,
                    18252,
                    45616,
                    65535,
                    57247,
                    28755,
                    9689,
                    7025,
                    17892,
                    26878,
                    23738,
                    12277,
                    4084,
                    905,
                    2057,
                    3227,
                    2941,
                    1546,
                    586,
                    275,
                    651,
                    998,
                    978,
                    581,
                    218,
                ],
                1.35805,
                0.601602,
                0.0127396,
                6.80926e02,
                [
                    1.02857e03,
                    3.51496e05,
                    2.31061e-02,
                    3.22674e00,
                    3.16617e00,
                    1.35796e00,
                    6.01802e-01,
                ],
            ),
            (
                [
                    419,
                    964,
                    1470,
                    1165,
                    428,
                    118,
                    2547,
                    7622,
                    7975,
                    3391,
                    979,
                    335,
                    5931,
                    29549,
                    51281,
                    22243,
                    3797,
                    1143,
                    4522,
                    21513,
                    65535,
                    62954,
                    13954,
                    2253,
                    1293,
                    4305,
                    15660,
                    26446,
                    12002,
                    1870,
                    243,
                    849,
                    2525,
                    3565,
                    2228,
                    491,
                ],
                1.08065,
                0.661207,
                0.613033,
                1.02857e03,
                [
                    9.92992e02,
                    3.48390e05,
                    4.33591e-01,
                    3.30708e00,
                    3.77223e00,
                    9.62266e-01,
                    8.24045e-01,
                ],
            ),
            (
                [
                    207,
                    747,
                    2302,
                    3383,
                    2176,
                    762,
                    867,
                    2512,
                    10026,
                    22327,
                    16365,
                    3663,
                    2689,
                    11998,
                    55855,
                    65535,
                    27336,
                    4542,
                    3587,
                    20606,
                    52265,
                    33975,
                    7420,
                    1594,
                    1275,
                    5343,
                    7709,
                    4121,
                    1060,
                    319,
                    210,
                    744,
                    1303,
                    1024,
                    391,
                    87,
                ],
                1.09118,
                0.679018,
                -0.556366,
                9.92992e02,
                [
                    7.43949e02,
                    3.50185e05,
                    -4.03968e-01,
                    3.54943e00,
                    3.27426e00,
                    9.93576e-01,
                    8.15199e-01,
                ],
            ),
        ],
    )
    def test_image_gaussian_local(
        self, value, major, minor, angle, start_guess, true_params
    ):
        """
        Based of cpl_fit_image_gaussian_test_local, found from line 1974 in cpl_fit-test.c

        That being said the test has been modified somewhat: in the original tests the first element of the result parameters was supposed
        to be used in the following test. I was not aware of this until finding out not doing this was causing the failure of
        the 5th test set.

        While copying that behaviour would be closer to the original tests: keeping it parameterized allows the testing of each parameter set individually.

        Since I already have the resulting parameters from the investigation might as well use those as starting guess inputs, and checking if pycpl also
        returns the correct parameters.
        """
        IMSZ = 6

        tol = 1e-5
        nx = IMSZ
        ny = IMSZ

        image = cpl.core.Image(np.array(value).reshape(IMSZ, IMSZ), cpl.core.Type.INT)
        if start_guess is not None:
            res = cpl.drs.fit.image_gaussian(
                image, int(nx / 2), int(ny / 2), nx, ny, guesses=[start_guess]
            )
            assert np.isclose(res.major, major, tol)
            assert np.isclose(res.minor, minor, tol)
            assert np.isclose(res.angle, angle, tol)
            assert np.allclose(res.parameters, true_params, tol)

            # Retry with the solution as 1st guess
            res = cpl.drs.fit.image_gaussian(
                image, int(nx / 2), int(ny / 2), nx, ny, guesses=res.parameters
            )
            assert np.isclose(res.major, major, tol)
            assert np.isclose(res.minor, minor, tol)
            assert np.isclose(res.angle, angle, tol)
            assert np.allclose(res.parameters, true_params, tol)
        else:
            res = cpl.drs.fit.image_gaussian(image, int(nx / 2), int(ny / 2), nx, ny)
            assert np.isclose(res.major, major, tol)
            assert np.isclose(res.minor, minor, tol)
            assert np.isclose(res.angle, angle, tol)
            assert np.allclose(res.parameters, true_params, tol)

    def test_lvmq(self):
        """
        As lvmq does not have any tests, this adapts a test for cpl_vector_fit_gaussian that utilises lvmq, by recreating cpl_vector_fit_gaussian in python then assert its output
        """
        import math

        N = 50
        yval = cpl.core.Vector.zeros(N)
        xval = cpl.core.Vector.zeros(N)
        ysig = cpl.core.Vector.zeros(N)

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

        res = py_vector_fit_gaussian(xval, yval, ysig)
        fit_gaussian_output = cpl.core.Vector.fit_gaussian(
            xval, yval, ysig, cpl.core.FitMode.ALL
        )
        print(res)

        cov = res.covariance
        assert cov.shape == (4, 4)
        assert np.allclose(cov, cov.transpose_create(), np.finfo(np.double).eps)

        assert res.best_fit[0] == fit_gaussian_output.x0
        assert math.fabs(res.best_fit[1]) == fit_gaussian_output.sigma
        assert res.best_fit[2] == fit_gaussian_output.area
        assert res.covariance == fit_gaussian_output.covariance
