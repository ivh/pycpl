Reproducers for PyCPL / PyHDRL binding defects
==============================================

Each script is standalone, needs only numpy plus an importable pycpl/pyhdrl, and
prints OBSERVED vs EXPECTED. Run with:

    uv run <script>.py                      # if pycpl is already on the path
    python <script>.py

Environment used for the runs quoted in the tickets
---------------------------------------------------
PyCPL 1.0.4 / PyHDRL 1.0.0 binding sources (as shipped in the pycpl 1.0.4.post4
third-party repackage at https://ivh.github.io/pycpl/simple/, whose src/ tree is
pristine upstream apart from four bind_* symbol renames), C libraries CPL 7.4 and
HDRL 1.6.0, numpy 2.5.2, Python 3.13.12, macOS 26.6 arm64.

NOTE: all six defects are in the Python binding layer, not in the vendored C
libraries, so they should reproduce identically against ESO's own PyCPL/PyHDRL
wheels. Worth re-running there before filing, to preempt the question.

NOTE: several of these no longer reproduce against our own wheels, which carry
local fixes in patches/. Run them against ESO's PyCPL/PyHDRL to see the defect.

Scripts
-------
1_hdrl_image_subscript.py            hdrl.core.Image.__getitem__/__setitem__ unreachable
2_hdrl_image_buffer_protocol.py      buffer protocol advertised but undefined
3_hdrl_value_unpicklable.py          Value namedtuple unpicklable -> breaks multiprocessing
4_hdrl_imagelist_indexing.py         no negative indexing + off-by-one bound check
5_hdrl_image_asarray_silent.py       np.asarray() returns a 0-d object array, silently
6_cpl_image_maskedarray_silent_nan.py  Image(MaskedArray) discards the mask as NaN (PyCPL)
7_cpl_detector_zone_def_dangling.py  zone_def passed as a dangling pointer (PyCPL) -- file first
8_cpl_polynomial_fit_leak.py         Polynomial.fit leaks sampsym; delete on new[] (PyCPL)
9_cpl_vector_wrap_allocator.md       new[] buffer freed by CPL with free() (PyCPL, inspection only)
10_pybind11_31_variant_resolution.py  pybind11 >= 3.1 breaks every variant-typed argument (PyCPL) -- file first

NOTE: 10 is build-environment dependent, unlike the rest. It needs a PyCPL built
against pybind11 >= 3.1.0; a build with 3.0.x shows none of it. The defect is in
PyCPL's variant declarations, not in pybind11.

Filing: 10 first and on its own -- it is a release blocker, it silently corrupts FITS
headers, and pyproject.toml has no upper bound so every source build from 2026-08-06
onwards is affected. 7 on its own (user-visible, has a workaround people need). 8 and 9 together
with the get_noise_ring leak (detector.cpp:105) as one memory-management ticket --
same defect class, same fix pass, none with a symptom on a normal build.
