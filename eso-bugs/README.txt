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

Scripts
-------
1_hdrl_image_subscript.py            hdrl.core.Image.__getitem__/__setitem__ unreachable
2_hdrl_image_buffer_protocol.py      buffer protocol advertised but undefined
3_hdrl_value_unpicklable.py          Value namedtuple unpicklable -> breaks multiprocessing
4_hdrl_imagelist_indexing.py         no negative indexing + off-by-one bound check
5_hdrl_image_asarray_silent.py       np.asarray() returns a 0-d object array, silently
6_cpl_image_maskedarray_silent_nan.py  Image(MaskedArray) discards the mask as NaN (PyCPL)
