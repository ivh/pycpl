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

import cpl


buffer = [
    96.807119,
    6.673062,
    47.828109,
    90.953442,
    35.169238,
    93.253366,
    65.443582,
    2.107025,
    51.220486,
    20.201893,
    93.997703,
    20.408227,
    37.882893,
    79.311394,
    28.820079,
    26.715673,
    35.682260,
    12.837355,
    70.329776,
    73.741373,
    80.252114,
    91.523087,
    51.138163,
    76.205738,
    45.638141,
    47.106201,
    29.955025,
    61.255939,
    7.338079,
    49.818536,
    21.958749,
    4.145198,
    56.491598,
    69.786858,
    95.098640,
    91.660836,
    63.040224,
    60.542222,
    93.767861,
    14.260710,
    80.744116,
    87.765564,
    34.668937,
    18.627008,
    67.076958,
    63.489016,
    45.342681,
    2.759218,
    76.326371,
    15.672457,
    76.500591,
    56.578485,
    7.195544,
    27.638754,
    32.784223,
    52.833685,
    74.744955,
    62.739249,
    14.089624,
    82.083033,
    12.557785,
    36.048373,
    86.228231,
    69.049383,
    5.835231,
    81.326871,
    60.710220,
    68.875455,
    41.869094,
    54.478081,
    83.136166,
    22.613209,
    42.243645,
    17.805103,
    41.240218,
    9.320603,
    81.294120,
    86.582899,
    12.079821,
    57.620490,
    2.255356,
    88.580412,
    14.198976,
    9.450900,
    16.219166,
    46.983199,
    62.284586,
    90.964121,
    9.722447,
    76.374210,
    73.047154,
    22.280233,
    12.422583,
    59.275385,
    91.329616,
    18.257814,
    40.602257,
    52.039836,
    87.133270,
    82.471350,
    6.517916,
    70.269436,
    5.084560,
    48.761561,
    88.074539,
    46.324777,
    58.082164,
    69.368659,
    32.907676,
    70.161985,
    26.989149,
    35.163032,
    58.742397,
    41.188125,
    44.613932,
    74.961563,
    88.171324,
    6.898518,
    65.925684,
    97.893771,
    83.272728,
    38.972839,
    20.174004,
    95.695311,
    98.248224,
    11.503620,
    13.953125,
    38.850481,
    63.543456,
    1.086395,
    21.321831,
    70.061372,
    71.355831,
    26.406390,
    18.822933,
    59.430370,
    72.731168,
    76.905097,
    28.799029,
    5.638844,
    47.067082,
    55.788179,
    40.801876,
    5.809480,
    96.976304,
    85.415809,
    80.771043,
    85.147628,
    92.314327,
    46.696728,
    83.041400,
    75.587054,
    85.669566,
    3.215404,
    71.282365,
    83.917790,
    14.719024,
    85.235491,
    22.768271,
    78.262480,
    86.321886,
    44.090102,
    48.323852,
    57.677717,
    70.496492,
    67.146785,
    17.108088,
    43.227660,
    44.051883,
    45.907117,
    48.866504,
    91.118965,
    1.695296,
    89.668380,
    96.928445,
    98.671600,
    75.084189,
    77.699488,
    83.819228,
    67.398515,
    24.396216,
    66.860628,
    42.985570,
    10.065782,
    70.076031,
    14.267935,
    93.983572,
    84.795055,
    99.503426,
    16.751843,
    63.057535,
    85.825312,
    60.841945,
    11.381387,
    43.503029,
    31.338437,
    78.528172,
    60.611117,
    74.566097,
    22.580055,
]
permute = [
    8,
    2,
    1,
    13,
    7,
    3,
    5,
    9,
    14,
    4,
    0,
    6,
    11,
    10,
    12,
    23,
    17,
    16,
    28,
    22,
    18,
    20,
    24,
    29,
    19,
    15,
    21,
    26,
    25,
    27,
]

positions = [0.1, 0.25, 1.0, 3.0, 9.0, 10.0, 11.0, 12.5]


class TestPPM:
    def test_match_points_basic_return(self):
        # Trying to match 3 identical points

        pattern = cpl.core.Matrix([[0, 9, 9, 1, 1, 5, 2, 3], [9, 0, 9, 0, 3, 4, 1, 7]])
        data = copy.deepcopy(pattern)
        # Match 8 identical points
        return_tuple = cpl.drs.ppm.match_points(data, 4, 1, pattern, 3, 0, 0.1, 1)
        assert return_tuple.matches == [0, 1, 2, 3, 4, 5, 6, 7]
        assert return_tuple.mdata == cpl.core.Matrix(
            [0, 9, 9, 1, 1, 5, 2, 3, 9, 0, 9, 0, 3, 4, 1, 7], 2
        )
        assert return_tuple.mpattern == cpl.core.Matrix(
            [0, 9, 9, 1, 1, 5, 2, 3, 9, 0, 9, 0, 3, 4, 1, 7], 2
        )
        assert return_tuple.lin_scale == 1.0
        assert return_tuple.lin_angle == 0.0

    def test_match_points_6_points(self):
        pattern = cpl.core.Matrix([[0, 9, 9], [9, 0, 9]])

        data = copy.deepcopy(pattern)
        # Trying to match 3 identical points

        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            data, 3, 1, pattern, 3, 0, 0.1, 1
        )
        assert len(matches) == 3
        assert mpattern.width == 3
        # scale points by 2
        data.multiply_scalar(2)
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            data, 3, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 3
        assert len(matches) == 3
        # Rotate data points by +45 degrees
        rotate = cpl.core.Matrix(
            [
                [math.sqrt(2) / 2, -math.sqrt(2) / 2],
                [math.sqrt(2) / 2, math.sqrt(2) / 2],
            ]
        )
        rdata = rotate @ data
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, 3, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 3
        assert len(matches) == 3
        assert math.isclose(lin_scale, 2.0)
        assert math.isclose(lin_angle, 45)
        # Shift data points by some vector
        rdata.add_scalar(-15)
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, 3, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 3
        assert len(matches) == 3
        assert math.isclose(lin_scale, 2.0)
        assert math.isclose(lin_angle, 45)

    def test_match_points_16_points(self):
        pattern = cpl.core.Matrix([[0, 9, 9, 1, 1, 5, 2, 3], [9, 0, 9, 0, 3, 4, 1, 7]])
        data = copy.deepcopy(pattern)
        # Match 8 identical points
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            data, 4, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 8
        assert len(matches) == 8
        # Remove a point from data
        rdata = copy.deepcopy(data)
        rdata.erase_columns(6, 1)
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, 4, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 7
        assert len(matches) == 8
        assert math.isclose(lin_angle, 0)
        assert lin_scale == 1.0
        # Below is copied from ppm_test.c for marking invalid values.
        # Currently no support as of yet as we didn't bind cpl_array
        # cpl_test_eq(cpl_array_count_invalid(matches), 1);

        # Rotate data points by -27 degrees
        rotate = cpl.core.Matrix(
            [
                [math.cos(-27.0 / 180.0 * math.pi), -math.sin(-27.0 / 180.0 * math.pi)],
                [
                    math.sin(-27.0 / 180.0 * math.pi),
                    math.cos(-27.0 / 180.0 * math.pi),
                ],
            ]
        )
        rdata = rotate @ data
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, 4, 1, pattern, 3, 0, 0.1, 1
        )
        assert mpattern.width == 8
        assert len(matches) == 8
        assert math.isclose(lin_angle, -27)
        assert lin_scale == 1.0

    def test_match_points_buffer_100(self):
        # At this point we use a random distribution of 100 points.
        # Only the first 70 will be written to pattern, the other 30
        # will be "false detections". And only the first 10 pattern
        # points and the first 20 data points will be used for direct
        # pattern matching - the remaining points will be recovered
        # in the second step. The data matrix will be rotated by 95
        # degrees and rescaled by a factor 2.35.

        pattern = cpl.core.Matrix(buffer, 2)
        data = copy.deepcopy(pattern)
        pattern.erase_columns(70, 30)
        data.multiply_scalar(2.35)
        rotate = cpl.core.Matrix(
            [
                [math.cos(95.0 / 180.0 * math.pi), -math.sin(95.0 / 180.0 * math.pi)],
                [math.sin(95.0 / 180.0 * math.pi), math.cos(95.0 / 180.0 * math.pi)],
            ]
        )
        rdata = rotate @ data
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, 20, 1, pattern, 10, 0, 0.1, 0.1
        )
        assert mpattern.width == 70
        assert len(matches) == 70
        assert lin_scale == 2.35

    def test_match_points_buffer_20(self):
        # Now with a random distribution of 20 points. This time
        # the whole data and the whole pattern (limited to 10 points)
        # are used. This means that a 50% contamination is present
        # on the data side already at the first pattern matching step.
        # The data matrix will be again rotated by 95 degrees and
        # rescaled by a factor 2.35.
        npattern = 10
        ndata = 20

        pattern = cpl.core.Matrix.zeros(2, ndata)
        k = 0
        i = 0
        while i < 2:
            j = 0
            while j < ndata:
                pattern[i][j] = buffer[k]
                j += 1
                k += 1
            i += 1
        data = copy.deepcopy(pattern)
        pattern.erase_columns(npattern, ndata - npattern)
        data.multiply_scalar(2.35)
        rotate = cpl.core.Matrix(
            [
                [math.cos(95.0 / 180.0 * math.pi), -math.sin(95.0 / 180.0 * math.pi)],
                [math.sin(95.0 / 180.0 * math.pi), math.cos(95.0 / 180.0 * math.pi)],
            ]
        )
        rdata = rotate @ data
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, ndata, 1, pattern, npattern, 0, 0.1, 0.1
        )
        assert mpattern.width == npattern
        assert len(matches) == npattern
        assert math.isclose(lin_angle, 95)
        assert lin_scale == 2.35

    def test_match_points_buffer_10(self):
        # Now with a random distribution of 10 points. This time
        # the pattern is larger than the data set by 5 points.
        # This means that points are missing on the data side.
        # The data matrix will be again rotated by 95 degrees and
        # rescaled by a factor 2.35.
        npattern = 15
        ndata = 10

        pattern = cpl.core.Matrix.zeros(2, npattern)
        k = 0
        i = 0
        while i < 2:
            j = 0
            while j < npattern:
                pattern[i][j] = buffer[permute[k]]
                j += 1
                k += 1
            i += 1

        data = cpl.core.Matrix.zeros(2, npattern)
        k = 0
        i = 0
        while i < 2:
            j = 0
            while j < npattern:
                data[i][j] = buffer[k]
                j += 1
                k += 1
            i += 1
        data.erase_columns(ndata, npattern - ndata)
        data.multiply_scalar(2.35)

        rotate = cpl.core.Matrix(
            [
                [math.cos(95.0 / 180.0 * math.pi), -math.sin(95.0 / 180.0 * math.pi)],
                [math.sin(95.0 / 180.0 * math.pi), math.cos(95.0 / 180.0 * math.pi)],
            ]
        )

        rdata = rotate @ data

        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            rdata, ndata, 1, pattern, npattern, 0, 0.1, 0.1
        )

        assert mpattern.width == ndata
        assert len(matches) == npattern
        assert math.isclose(lin_angle, 95)
        assert lin_scale == 2.35

    def test_match_points_buffer_20_aligned(self):
        # Now with a random distribution of 20 aligned points.
        # The whole data and the whole pattern (limited to 10 points)
        # are used. This means that a 50% contamination is present
        # on the data side already at the first pattern matching step.
        # The data matrix will be rescaled by a factor 2.35.
        npattern = 10
        ndata = 20
        pattern = cpl.core.Matrix.zeros(2, ndata)
        k = 0
        for j in range(ndata):
            pattern[0][j] = buffer[k]
            k += 1
        for j in range(ndata):
            pattern[1][j] = 0.0
            k += 1
        data = copy.deepcopy(pattern)
        pattern.erase_columns(npattern, ndata - npattern)
        data.multiply_scalar(2.35)
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            data, ndata, 1, pattern, npattern, 0, 0.1, 0.1
        )
        assert mpattern.width == npattern
        assert len(matches) == npattern
        assert lin_scale == 2.35
        assert math.isclose(lin_angle, 0.0)

    def test_data_as_pattern(self):
        npattern = 10
        ndata = 20
        pattern = cpl.core.Matrix.zeros(2, ndata)
        k = 0
        for j in range(ndata):
            pattern[0][j] = buffer[k]
            k += 1
        for j in range(ndata):
            pattern[1][j] = 0.0
            k += 1
        data = copy.deepcopy(pattern)
        pattern.erase_columns(npattern, ndata - npattern)
        data.multiply_scalar(2.35)
        # Use data also as pattern
        matches, mdata, mpattern, lin_scale, lin_angle = cpl.drs.ppm.match_points(
            data, ndata, 1, data, ndata, 0, 0.1, 0.1
        )
        assert mdata.width == ndata
        assert len(matches) == ndata
        assert lin_scale == 1.0
        assert math.isclose(lin_angle, 0.0)

    def test_errors(self):
        npattern = 10
        ndata = 20
        pattern = cpl.core.Matrix.zeros(2, ndata)
        k = 0
        for j in range(ndata):
            pattern[0][j] = buffer[k]
            k += 1
        for j in range(ndata):
            pattern[1][j] = 0.0
            k += 1
        data = copy.deepcopy(pattern)
        pattern.erase_columns(npattern, ndata - npattern)
        data.multiply_scalar(2.35)

        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.ppm.match_points(data, 2, 1, pattern, npattern, 0, 0.1, 0.1)

        # ndata too large
        with pytest.raises(cpl.core.AccessOutOfRangeError):
            _ = cpl.drs.ppm.match_points(
                data, ndata + 1, 1, pattern, npattern, 0, 0.1, 0.1
            )

        # npattern too large
        with pytest.raises(cpl.core.AccessOutOfRangeError):
            _ = cpl.drs.ppm.match_points(
                data, ndata, 1, pattern, npattern + 1, 0, 0.1, 0.1
            )

        # npattern too small
        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.ppm.match_points(data, ndata, 1, pattern, 2, 0, 0.1, 0.1)

        # err_data and err_pattern non-positive
        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.ppm.match_points(
                data, ndata, 0.0, pattern, npattern, 0.0, 0.1, 0.1
            )

        # tolerance negative
        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.ppm.match_points(
                data, ndata, 1.0, pattern, npattern, 0.0, -0.1, 0.1
            )

        # radius negative
        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.ppm.match_points(
                data, ndata, 1.0, pattern, npattern, 0.0, 0.1, -0.1
            )

        # Too few columns in data
        with pytest.raises(cpl.core.IllegalInputError):
            twobytwo = cpl.core.Matrix.zeros(2, 2)
            _ = cpl.drs.ppm.match_points(
                twobytwo, ndata, 1, pattern, npattern, 0, 0.1, 0.1
            )

        # Too few columns in pattern
        with pytest.raises(cpl.core.IllegalInputError):
            twobytwo = cpl.core.Matrix.zeros(2, 2)
            _ = cpl.drs.ppm.match_points(
                data, ndata, 1, twobytwo, npattern, 0, 0.1, 0.1
            )

    # Test the case where peaks and lines are identical
    def test_match_positions_identical(self):
        peaks = cpl.core.Vector(positions[2:7])
        lines = cpl.core.Vector(positions[2:7])
        matched_positions = cpl.drs.ppm.match_positions(peaks, lines, 0.99, 1.01, 1.0)
        assert len(matched_positions[0]) == len(lines) - 1

    def test_match_positions_extra_peaks(self):
        # Test case where we add additional peaks at the end
        peaks = cpl.core.Vector(positions[2:])
        lines = cpl.core.Vector(positions[2:7])
        matched_positions = cpl.drs.ppm.match_positions(peaks, lines, 0.9, 1.1, 0.9)
        assert len(matched_positions[0]) == len(lines) - 1

        # Test case where we add additional peaks at both ends
        peaks = cpl.core.Vector(positions)
        matched_positions = cpl.drs.ppm.match_positions(peaks, lines, 0.9, 1.1, 0.9)
        assert len(matched_positions[0]) == len(lines) - 1

    # Test case where we add extra lines at both ends compared to the peaks.
    # Inverse of test_match_positions_extra_peaks

    def test_match_positions_extra_lines(self):
        peaks = cpl.core.Vector(positions[2:6])
        lines = cpl.core.Vector(positions)
        matched_positions = cpl.drs.ppm.match_positions(peaks, lines, 0.9, 1.1, 0.9)
        assert len(matched_positions[0]) == len(peaks) - 1

    # Real life scenario example test
    def test_match_positions_real(self):
        peaks = cpl.core.Vector(
            [
                686.772,
                973.745,
                1140.69,
                1184.02,
                1219.17,
                1236.48,
                1262.11,
                1273.67,
                1289.98,
                1332.57,
                1371.65,
                1402.06,
                1425.62,
                1464.21,
                1479.52,
                1562.45,
                1583.41,
                1636.00,
                1678.33,
                1699.17,
                1730.06,
                1899.42,
                1981.58,
                2002.91,
                2094.64,
                2151.58,
                2306.59,
                2333.43,
                2346.61,
                2384.20,
                2390.81,
                2711.13,
                2823.12,
                2852.02,
                2866.53,
                2965.52,
                2998.58,
                3051.23,
                3060.80,
                3093.66,
                3129.96,
                3155.82,
                3195.60,
                3217.07,
                3233.23,
                3268.28,
                3278.29,
                3284.29,
                3305.73,
                3324.35,
                3379.01,
                3386.96,
                3445.52,
                3455.10,
                3498.60,
                3684.23,
                3727.32,
                3742.57,
                3807.69,
                3828.57,
                3997.14,
            ]
        )
        lines = cpl.core.Vector(
            [
                5400.56,
                5764.42,
                5820.16,
                5852.49,
                5872.83,
                5881.90,
                5901.41,
                5944.83,
                5975.12,
                6030.00,
                6074.34,
                6096.16,
                6128.45,
                6143.06,
                6163.59,
                6217.28,
                6266.50,
                6304.79,
                6332.77,
                6382.99,
                6402.25,
                6506.53,
                6532.88,
                6598.95,
                6652.09,
                6678.28,
                6717.04,
                6929.47,
                7024.05,
                7032.41,
                7051.29,
                7059.11,
                7173.94,
                7245.17,
                7438.90,
                7472.44,
                7488.87,
                7535.77,
                7544.04,
                7943.18,
                8082.46,
                8118.55,
                8136.41,
                8259.38,
                8266.08,
                8300.33,
                8365.75,
                8377.51,
                8418.43,
                8463.36,
                8495.36,
                8544.70,
                8571.35,
                8591.26,
                8634.65,
                8647.04,
                8654.38,
                8680.79,
                8704.11,
                8771.66,
                8781.97,
                8853.87,
                8865.70,
                8919.50,
                9148.67,
                9201.76,
                9220.47,
                9300.85,
                9326.51,
                9425.38,
                9534.16,
                9665.42,
            ]
        )
        reference_matches = [
            (1140.69, 5975.12),
            (1184.02, 6030.00),
            (1219.17, 6074.34),
            (1236.48, 6096.16),
            (1262.11, 6128.45),
            (1273.67, 6143.06),
            (1289.98, 6163.59),
            (1332.57, 6217.28),
            (1371.65, 6266.50),
            (1402.06, 6304.79),
            (1425.62, 6332.77),
            (1464.21, 6382.99),
            (1479.52, 6402.25),
            (1562.45, 6506.53),
            (1583.41, 6532.88),
            (1636.00, 6598.95),
            (1678.33, 6652.09),
            (1699.17, 6678.28),
            (1730.06, 6717.04),
            (1899.42, 6929.47),
            (1981.58, 7032.41),
            (2002.91, 7059.11),
            (2094.64, 7173.94),
            (2151.58, 7245.17),
            (2306.59, 7438.90),
            (2333.43, 7472.44),
            (2346.61, 7488.87),
            (2384.20, 7535.77),
            (2390.81, 7544.04),
            (2711.13, 7943.18),
            (2823.12, 8082.46),
            (2852.02, 8118.55),
            (2866.53, 8136.41),
            (2965.52, 8259.38),
            (2998.58, 8300.33),
            (3051.23, 8365.75),
            (3060.80, 8377.51),
            (3093.66, 8418.43),
            (3129.96, 8463.36),
            (3155.82, 8495.36),
            (3195.60, 8544.70),
            (3217.07, 8571.35),
            (3233.23, 8591.26),
            (3268.28, 8634.65),
            (3278.29, 8647.04),
            (3284.29, 8654.38),
            (3305.73, 8680.79),
            (3324.35, 8704.11),
            (3379.01, 8771.66),
            (3386.96, 8781.97),
            (3445.52, 8853.87),
            (3455.10, 8865.70),
            (3498.60, 8919.50),
            (3684.23, 9148.67),
            (3727.32, 9201.76),
            (3742.57, 9220.47),
            (3807.69, 9300.85),
            (3828.57, 9326.51),
        ]
        matched_positions = cpl.drs.ppm.match_positions(
            peaks, lines, 1.1875, 1.3125, 0.1
        )
        assert len(matched_positions[0]) == len(reference_matches)

        epsilon = 100.0 * np.finfo(np.float64).eps
        reference_matches_x = [pos[0] for pos in reference_matches]
        reference_matches_y = [pos[1] for pos in reference_matches]
        assert list(matched_positions.x) == pytest.approx(
            reference_matches_x, abs=epsilon
        )
        assert list(matched_positions.y) == pytest.approx(
            reference_matches_y, abs=epsilon
        )

    def test_match_positions_less_than_4_peaks(self):
        peaks = cpl.core.Vector(positions[2:5])
        lines = cpl.core.Vector(positions[2:5])
        with pytest.raises(cpl.core.DataNotFoundError):
            _ = cpl.drs.ppm.match_positions(peaks, lines, 0.9, 1.1, 0.9)
