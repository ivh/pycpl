"""Local pytest configuration for the vendored upstream suites.

`tests/` (PyCPL) and `tests-hdrl/` (PyHDRL) are pristine upstream and must stay that
way, so platform quirks in them are handled from here rather than by editing them.
pytest loads this file because it sits at the rootdir named by `pytest.ini`, even when
the suites are invoked by absolute path from somewhere else, which is how CI runs them.
"""

import platform

import pytest

# tests/cplcore/test_polynomial.py:200 hard-codes evaluation results that depend on the
# width of `long double`, and guards only two of the three cases in the wild: x86_64
# (80-bit extended) and Darwin/arm64 (64-bit, same as double). Linux aarch64 has IEEE
# 128-bit quad and lands on a third value -- -8.180305391403131e-05 where the test wants
# the x86_64 -8.17776e-05 -- so both root evaluations fail there and nowhere else.
# Reported to ESO; drop this once their guard covers aarch64. See eso-bugs/README.txt.
_LONG_DOUBLE_QUAD = platform.system() == "Linux" and platform.machine() == "aarch64"

_QUAD_PRECISION_XFAIL = (
    "tests/cplcore/test_polynomial.py::TestPolynomial::test_eval_2d[-8.17776e-05-xy0]",
    "tests/cplcore/test_polynomial.py::TestPolynomial::test_eval_2d[-8176-xy1]",
)


def pytest_collection_modifyitems(config, items):
    if not _LONG_DOUBLE_QUAD:
        return
    marker = pytest.mark.xfail(
        reason="upstream expects x86_64 long double; aarch64 Linux has 128-bit quad",
        strict=False,
    )
    for item in items:
        if any(item.nodeid.endswith(nodeid) for nodeid in _QUAD_PRECISION_XFAIL):
            item.add_marker(marker)
