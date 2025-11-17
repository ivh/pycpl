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

import numpy as np
import pytest

import cpl.core
import cpl.drs


skeys = ["CTYPE1", "CTYPE2"]
skeys_tab = ["TCTYP3", "TCTYP5"]

dkeys = [
    "CRVAL1",
    "CRVAL2",
    "CRPIX1",
    "CRPIX2",
    "CD1_1",
    "CD1_2",
    "CD2_1",
    "CD2_2",
    "PV2_1",
    "PV2_2",
    "PV2_3",
    "PV2_4",
    "PV2_5",
]
dkeys_tab = [
    "TCRVL3",
    "TCRVL5",
    "TCRPX3",
    "TCRPX5",
    "TC3_3",
    "TC3_5",
    "TC5_3",
    "TC5_5",
    "TV5_1",
    "TV5_2",
    "TV5_3",
    "TV5_4",
    "TV5_5",
]

ikeys = ["NAXIS", "NAXIS1", "NAXIS2"]

svals = ["RA---ZPN", "DEC--ZPN"]
dvals = [
    5.57368333333,
    -72.0576388889,
    5401.6,
    6860.8,
    5.81347849634012e-21,
    9.49444444444444e-05,
    -9.49444444444444e-05,
    -5.81347849634012e-21,
    1.0,
    0.0,
    42.0,
    0.0,
    0.0,
]
ivals = [2, 2048, 2048]

svals2 = ["RA---ZPN", "DEC--ZPN"]
dvals2 = [
    136.018529319233,
    34.5793698413348,
    3000.83678222321,
    -936.601342583387,
    5.33299465243094e-08,
    -0.000111500211205622,
    0.000111494500595739,
    -1.41761930842942e-08,
    1.0,
    0.0,
    -45.0,
    0.0,
    0.0,
]
ivals2 = [2, 2064, 2064]

physin = [1024.0, 1024.0, 1025.0, 1023.0]
worldout = [3.825029720, -71.636524754, 3.824722171, -71.636616487]
stdcout = [-0.554171733, 0.415628800]


worldin = [3.824875946, -71.636570620]
physout = [1024.5, 1023.5]

xy = [
    382.252,
    36.261,
    18.097,
    38.428,
    1551.399,
    51.774,
    800.372,
    85.623,
    296.776,
    97.524,
    1688.610,
    137.485,
    1137.220,
    207.411,
    881.385,
    213.913,
    1301.158,
    239.626,
    1925.201,
    242.518,
    152.270,
    252.434,
    493.289,
    274.073,
    1027.982,
    286.037,
    1123.301,
    302.822,
    791.430,
    309.277,
    1634.414,
    329.992,
    45.455,
    334.707,
    224.075,
    343.282,
    622.339,
    352.331,
    1967.698,
    358.488,
    901.068,
    409.338,
    953.233,
    417.257,
    1289.861,
    426.885,
    596.422,
    430.980,
    129.039,
    446.756,
    1534.073,
    463.523,
    1476.210,
    464.923,
    749.281,
    475.854,
    441.555,
    478.242,
    1333.284,
    482.094,
    662.150,
    536.999,
    1246.223,
    585.173,
    475.595,
    656.727,
    169.960,
    679.461,
    1502.242,
    689.464,
    93.412,
    694.360,
    545.388,
    696.031,
    247.199,
    746.899,
    615.903,
    757.244,
    1927.821,
    864.380,
    1724.554,
    889.479,
    1449.032,
    895.684,
    1558.846,
    906.941,
    777.762,
    912.936,
    1327.582,
    914.718,
    857.511,
    920.258,
    850.675,
    936.752,
    317.737,
    966.788,
    1954.225,
    967.560,
    1334.811,
    1055.225,
    311.918,
    1075.083,
    1132.445,
    1097.327,
    1600.109,
    1096.887,
    1254.242,
    1119.658,
    1285.886,
    1141.102,
    665.050,
    1149.314,
    259.059,
    1182.549,
    1511.898,
    1203.002,
    1041.352,
    1220.464,
    944.823,
    1302.967,
    990.619,
    1303.994,
    1303.476,
    1320.765,
    1059.565,
    1334.145,
    1035.701,
    1340.268,
    441.037,
    1340.244,
    1149.912,
    1355.465,
    225.527,
    1356.680,
    1558.375,
    1387.388,
    2034.853,
    1410.099,
    799.400,
    1429.687,
    1285.559,
    1491.646,
    218.601,
    1500.189,
    726.648,
    1516.822,
    1112.176,
    1520.933,
    147.153,
    1531.091,
    904.416,
    1534.735,
    1736.805,
    1557.387,
    1496.763,
    1603.196,
    1478.237,
    1618.552,
    770.317,
    1656.818,
    1625.301,
    1676.042,
    1207.898,
    1696.559,
    470.996,
    1727.196,
    1807.232,
    1732.755,
    1462.826,
    1735.153,
    1098.744,
    1753.648,
    67.590,
    1757.731,
    431.763,
    1811.486,
    92.574,
    1834.930,
    833.029,
    1939.049,
    244.391,
    1953.157,
    1040.013,
    1959.332,
    874.378,
    1980.592,
    1396.658,
    2014.189,
]

radec = [
    135.88677979,
    34.28698730,
    135.88664246,
    34.24613953,
    135.88479614,
    34.41748047,
    135.88023376,
    34.33363724,
    135.87858582,
    34.27736664,
    135.87304688,
    34.43286514,
    135.86373901,
    34.37126541,
    135.86285400,
    34.34273529,
    135.85929871,
    34.38956833,
    135.85894775,
    34.45923233,
    135.85778809,
    34.26112366,
    135.85470581,
    34.29931259,
    135.85314941,
    34.35920334,
    135.85084534,
    34.36973190,
    135.84997559,
    34.33263397,
    135.84710693,
    34.42679977,
    135.84652710,
    34.24911499,
    135.84542847,
    34.26911163,
    135.84413147,
    34.31375504,
    135.84321594,
    34.46398163,
    135.83653259,
    34.34495163,
    135.83547974,
    34.35073853,
    135.83410645,
    34.38834763,
    135.83352661,
    34.31077576,
    135.83146667,
    34.25836945,
    135.82904053,
    34.41551590,
    135.82885742,
    34.40904236,
    135.82754517,
    34.32785416,
    135.82716370,
    34.29343414,
    135.82658386,
    34.39306641,
    135.81916809,
    34.31809998,
    135.81260681,
    34.38336945,
    135.80287170,
    34.29724884,
    135.79995728,
    34.26297760,
    135.79847717,
    34.41189957,
    135.79797363,
    34.25428772,
    135.79760742,
    34.30493546,
    135.79077148,
    34.27165985,
    135.78939819,
    34.31282425,
    135.77473450,
    34.45937729,
    135.77145386,
    34.43675232,
    135.77061462,
    34.40594864,
    135.76892090,
    34.41823959,
    135.76821899,
    34.33092880,
    135.76800537,
    34.39233398,
    135.76731873,
    34.33979416,
    135.76509094,
    34.33901596,
    135.76103210,
    34.27935028,
    135.76074219,
    34.46220398,
    135.74896240,
    34.39315033,
    135.74639893,
    34.27863312,
    135.74327087,
    34.37044907,
    135.74317932,
    34.42278671,
    135.74020386,
    34.38416672,
    135.73742676,
    34.38760376,
    135.73634338,
    34.31815338,
    135.73170471,
    34.27264023,
    135.72891235,
    34.41275406,
    135.72660828,
    34.36019516,
    135.71536255,
    34.34933853,
    135.71527100,
    34.35440445,
    135.71296692,
    34.38951492,
    135.71118164,
    34.36217117,
    135.71046448,
    34.35951996,
    135.71041870,
    34.29297638,
    135.70826721,
    34.37225342,
    135.70826721,
    34.26868439,
    135.70388794,
    34.41787720,
    135.70075989,
    34.47105789,
    135.69830322,
    34.33304977,
    135.68984985,
    34.38727188,
    135.68881226,
    34.26796341,
    135.68650818,
    34.32481003,
    135.68591309,
    34.36791611,
    135.68466187,
    34.25994110,
    135.68405151,
    34.34471130,
    135.68084717,
    34.43779755,
    135.67460632,
    34.41091919,
    135.67254639,
    34.40884781,
    135.66752625,
    34.32962036,
    135.66487122,
    34.42524338,
    135.66201782,
    34.37860870,
    135.65805054,
    34.29612732,
    135.65704346,
    34.44551849,
    135.65676880,
    34.40697479,
    135.65438843,
    34.36631012,
    135.65388489,
    34.25082397,
    135.64648438,
    34.29161072,
    135.64344788,
    34.25370026,
    135.62927246,
    34.33645630,
    135.62741089,
    34.27054977,
    135.62643433,
    34.35960388,
    135.62356567,
    34.34106827,
    135.61895752,
    34.39946365,
]


@pytest.fixture(scope="session", autouse=True)
def wcs_instance(request):
    allprops = [cpl.core.Property("EXPTIME", cpl.core.Type.DOUBLE, 0.0)]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.STRING, val)
        for key, val in zip(skeys, svals)
    ]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.DOUBLE, val)
        for key, val in zip(dkeys, dvals)
    ]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.INT, val) for key, val in zip(ikeys, ivals)
    ]
    proplist = cpl.core.PropertyList(allprops)
    wcs = cpl.drs.WCS(proplist)
    return wcs


@pytest.fixture(scope="session", autouse=True)
def platesol_proplist(request):
    allprops = [cpl.core.Property("EXPTIME", cpl.core.Type.DOUBLE, 0.0)]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.DOUBLE, val)
        for key, val in zip(dkeys, dvals2)
    ]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.STRING, val)
        for key, val in zip(skeys, svals2)
    ]
    allprops += [
        cpl.core.Property(key, cpl.core.Type.INT, val)
        for key, val in zip(ikeys, ivals2)
    ]
    proplist = cpl.core.PropertyList(allprops)
    return proplist


@pytest.fixture(scope="session", autouse=True)
def xymat(request):
    return cpl.core.Matrix(xy, 94)


@pytest.fixture(scope="session", autouse=True)
def rdmat(request):
    radec = [
        135.88677979,
        34.28698730,
        135.88664246,
        34.24613953,
        135.88479614,
        34.41748047,
        135.88023376,
        34.33363724,
        135.87858582,
        34.27736664,
        135.87304688,
        34.43286514,
        135.86373901,
        34.37126541,
        135.86285400,
        34.34273529,
        135.85929871,
        34.38956833,
        135.85894775,
        34.45923233,
        135.85778809,
        34.26112366,
        135.85470581,
        34.29931259,
        135.85314941,
        34.35920334,
        135.85084534,
        34.36973190,
        135.84997559,
        34.33263397,
        135.84710693,
        34.42679977,
        135.84652710,
        34.24911499,
        135.84542847,
        34.26911163,
        135.84413147,
        34.31375504,
        135.84321594,
        34.46398163,
        135.83653259,
        34.34495163,
        135.83547974,
        34.35073853,
        135.83410645,
        34.38834763,
        135.83352661,
        34.31077576,
        135.83146667,
        34.25836945,
        135.82904053,
        34.41551590,
        135.82885742,
        34.40904236,
        135.82754517,
        34.32785416,
        135.82716370,
        34.29343414,
        135.82658386,
        34.39306641,
        135.81916809,
        34.31809998,
        135.81260681,
        34.38336945,
        135.80287170,
        34.29724884,
        135.79995728,
        34.26297760,
        135.79847717,
        34.41189957,
        135.79797363,
        34.25428772,
        135.79760742,
        34.30493546,
        135.79077148,
        34.27165985,
        135.78939819,
        34.31282425,
        135.77473450,
        34.45937729,
        135.77145386,
        34.43675232,
        135.77061462,
        34.40594864,
        135.76892090,
        34.41823959,
        135.76821899,
        34.33092880,
        135.76800537,
        34.39233398,
        135.76731873,
        34.33979416,
        135.76509094,
        34.33901596,
        135.76103210,
        34.27935028,
        135.76074219,
        34.46220398,
        135.74896240,
        34.39315033,
        135.74639893,
        34.27863312,
        135.74327087,
        34.37044907,
        135.74317932,
        34.42278671,
        135.74020386,
        34.38416672,
        135.73742676,
        34.38760376,
        135.73634338,
        34.31815338,
        135.73170471,
        34.27264023,
        135.72891235,
        34.41275406,
        135.72660828,
        34.36019516,
        135.71536255,
        34.34933853,
        135.71527100,
        34.35440445,
        135.71296692,
        34.38951492,
        135.71118164,
        34.36217117,
        135.71046448,
        34.35951996,
        135.71041870,
        34.29297638,
        135.70826721,
        34.37225342,
        135.70826721,
        34.26868439,
        135.70388794,
        34.41787720,
        135.70075989,
        34.47105789,
        135.69830322,
        34.33304977,
        135.68984985,
        34.38727188,
        135.68881226,
        34.26796341,
        135.68650818,
        34.32481003,
        135.68591309,
        34.36791611,
        135.68466187,
        34.25994110,
        135.68405151,
        34.34471130,
        135.68084717,
        34.43779755,
        135.67460632,
        34.41091919,
        135.67254639,
        34.40884781,
        135.66752625,
        34.32962036,
        135.66487122,
        34.42524338,
        135.66201782,
        34.37860870,
        135.65805054,
        34.29612732,
        135.65704346,
        34.44551849,
        135.65676880,
        34.40697479,
        135.65438843,
        34.36631012,
        135.65388489,
        34.25082397,
        135.64648438,
        34.29161072,
        135.64344788,
        34.25370026,
        135.62927246,
        34.33645630,
        135.62741089,
        34.27054977,
        135.62643433,
        34.35960388,
        135.62356567,
        34.34106827,
        135.61895752,
        34.39946365,
    ]
    return cpl.core.Matrix(radec, 94)


class TestWCS:
    def test_image_naxis(self, wcs_instance):
        assert wcs_instance.image_naxis == 2

    def test_image_dims(self, wcs_instance):
        assert len(wcs_instance.image_dims) == 2

    def test_crval(self, wcs_instance):
        assert len(wcs_instance.crval) == 2

    def test_crpix(self, wcs_instance):
        assert len(wcs_instance.crpix) == 2

    def test_cd(self, wcs_instance):
        assert wcs_instance.cd.height == 2
        assert wcs_instance.cd.width == 2

    def test_convert_physical_to_world(self, wcs_instance):
        # Transform physical -> world (2 points)

        from_matrix = cpl.core.Matrix(physin, 2)
        to_matrix = wcs_instance.convert(from_matrix, wcs_instance.PHYS2WORLD)

        assert np.isclose(to_matrix[0][0], worldout[0], np.finfo(np.float32).eps)
        assert np.isclose(to_matrix[0][1], worldout[1], np.finfo(np.float32).eps)
        assert np.isclose(to_matrix[1][0], worldout[2], np.finfo(np.float32).eps)
        assert np.isclose(to_matrix[1][1], worldout[3], np.finfo(np.float32).eps)

    def test_convert_world_to_physical(self, wcs_instance):
        # Do world to physical conversion

        from_matrix = cpl.core.Matrix(worldin, 1)
        try:
            to_matrix = wcs_instance.convert(from_matrix, wcs_instance.WORLD2PHYS)
        except cpl.drs.WCSLibError as e:
            print(e.error_list)

        # Status should all be 0, to indicate there is no issues

        assert np.isclose(to_matrix[0][0], physout[0], np.finfo(np.float32).eps)
        assert np.isclose(to_matrix[0][1], physout[1], np.finfo(np.float32).eps)

    def test_convert_physical_to_standard(self, wcs_instance):
        # Do physical to standard

        from_matrix = cpl.core.Matrix(
            physin[:2], 1
        )  # Apparently this is left unchanged and I don't see how it could without throwing an error
        to_matrix = wcs_instance.convert(from_matrix, wcs_instance.PHYS2STD)
        # Status should all be 0, to indicate there is no issues

        assert np.isclose(to_matrix[0][0], stdcout[0], 1.7e-9)
        assert np.isclose(to_matrix[0][1], stdcout[1], 1.7e-9)

    def test_ctype(self, wcs_instance):
        assert np.array_equal(wcs_instance.ctype, ["RA---ZPN", "DEC--ZPN"])

    def test_cunit(self, wcs_instance):
        assert np.array_equal(wcs_instance.cunit, ["deg", "deg"])

    def test_platesol_illegal_input(self, platesol_proplist, rdmat, xymat):
        with pytest.raises(cpl.core.IllegalInputError):
            cpl.drs.WCS.platesol(
                platesol_proplist,
                rdmat,
                xymat,
                0,
                3.0,
                cpl.drs.WCS.PLATESOL_6,
                cpl.drs.WCS.MV_CRVAL,
            )

    def test_platesol_data_not_found(self, platesol_proplist, rdmat, xymat):
        with pytest.raises(cpl.core.DataNotFoundError):
            cpl.drs.WCS.platesol(
                platesol_proplist,
                rdmat,
                xymat,
                3,
                0.0,
                cpl.drs.WCS.PLATESOL_6,
                cpl.drs.WCS.MV_CRVAL,
            )

    def test_platesol_6_constant(self, platesol_proplist, rdmat, xymat):
        opl = cpl.drs.WCS.platesol(
            platesol_proplist,
            rdmat,
            xymat,
            3,
            3.0,
            cpl.drs.WCS.PLATESOL_6,
            cpl.drs.WCS.MV_CRVAL,
        )
        wcs = cpl.drs.WCS(opl)
        arcrval = wcs.crval
        arcrpix = wcs.crpix
        mtcd = wcs.cd
        assert wcs.image_naxis == 0
        assert len(arcrval) == 2
        assert len(arcrpix) == 2
        assert mtcd.shape == (2, 2)
        d1 = opl["CSYER1"].value
        d2 = opl["CSYER2"].value
        # Check if error is less than threshold
        assert d1 <= 3.8e-5
        assert d2 <= 3.8e-5

    def test_platesol_6_constant_crpix(self, platesol_proplist, rdmat, xymat):
        opl = cpl.drs.WCS.platesol(
            platesol_proplist,
            rdmat,
            xymat,
            3,
            3.0,
            cpl.drs.WCS.PLATESOL_6,
            cpl.drs.WCS.MV_CRPIX,
        )
        wcs = cpl.drs.WCS(opl)
        arcrval = wcs.crval
        arcrpix = wcs.crpix
        mtcd = wcs.cd
        assert wcs.image_naxis == 0
        assert len(arcrval) == 2
        assert len(arcrpix) == 2
        assert mtcd.shape == (2, 2)
        d1 = opl["CSYER1"].value
        d2 = opl["CSYER2"].value
        # Check if error is less than threshold
        assert d1 <= 3.8e-5
        assert d2 <= 3.8e-5

    def test_platesol_4_constant(self, platesol_proplist, rdmat, xymat):
        opl = cpl.drs.WCS.platesol(
            platesol_proplist,
            rdmat,
            xymat,
            3,
            3.0,
            cpl.drs.WCS.PLATESOL_4,
            cpl.drs.WCS.MV_CRVAL,
        )
        wcs = cpl.drs.WCS(opl)
        arcrval = wcs.crval
        arcrpix = wcs.crpix
        mtcd = wcs.cd
        assert wcs.image_naxis == 0
        assert len(arcrval) == 2
        assert len(arcrpix) == 2
        assert mtcd.shape == (2, 2)
        d1 = opl["CSYER1"].value
        d2 = opl["CSYER2"].value
        # Check if error is less than threshold
        assert d1 <= 1.05e-4
        assert d2 <= 7.3e-5

    def test_platesol_4_constant_crpix(self, platesol_proplist, rdmat, xymat):
        opl = cpl.drs.WCS.platesol(
            platesol_proplist,
            rdmat,
            xymat,
            3,
            3.0,
            cpl.drs.WCS.PLATESOL_4,
            cpl.drs.WCS.MV_CRPIX,
        )
        wcs = cpl.drs.WCS(opl)
        arcrval = wcs.crval
        arcrpix = wcs.crpix
        mtcd = wcs.cd
        assert wcs.image_naxis == 0
        assert len(arcrval) == 2
        assert len(arcrpix) == 2
        assert mtcd.shape == (2, 2)
        d1 = opl["CSYER1"].value
        d2 = opl["CSYER2"].value
        # Check if error is less than threshold
        assert d1 <= 1.05e-4
        assert d2 <= 7.3e-5
