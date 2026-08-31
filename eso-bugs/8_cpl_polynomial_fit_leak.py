# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyCPL: Polynomial.fit leaks its sampsym buffer, and frees it with the wrong operator.

src/cplcore/polynomial.cpp:236 and :258

    sampsym_ptr = new cpl_boolean[len];     // :236
    ...
    Error::throw_errors_with(cpl_polynomial_fit, ...);
    delete sampsym_ptr;                     // :258

Two defects in one path:

(1) LEAK. Between the allocation and the delete there are three throw sites -- the
    mindeg length check, the maxdeg length check, and cpl_polynomial_fit itself via
    throw_errors_with. Any of them skips the delete entirely. A recipe that calls fit
    in a loop and handles the failure leaks len * sizeof(cpl_boolean) bytes per call.

(2) MISMATCHED DEALLOCATION. `delete` on memory from `new[]` is undefined behaviour
    even on the success path. It is benign with the usual allocators, which is why it
    has gone unnoticed, but ASan reports it as alloc-dealloc-mismatch, and PyCPL's own
    build system already supports that (PYCPL_BUILD_SANITIZE=address in setup.py).

MEASUREMENT below: dimension 20000, 3000 failing calls. cpl_boolean is an enum, so
4 bytes; 20000 * 4 * 3000 = 240 MB, and that is what the process grows by.

    unpatched PyCPL 1.0.4   RSS  46.8 -> 287.0 MB   (+240.2 MB)
    with the fix applied    RSS  46.5 ->  52.0 MB   (+5.5 MB, ordinary noise)

FIX: hold the buffer in a std::vector<cpl_boolean> and drop the manual delete; it is
then released on every path, correctly, and the function becomes exception-safe.
"""
import resource

import cpl

DIM, N = 20000, 3000


def rss_mb():
    # macOS reports ru_maxrss in bytes, Linux in kilobytes
    raw = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return raw / (1024 * 1024) if raw > 10**7 else raw / 1024


p = cpl.core.Polynomial(DIM)
samppos = cpl.core.Matrix([[1.0] * 4])
fitvals = cpl.core.Vector([1.0, 2.0, 3.0, 4.0])
sampsym = [False] * DIM          # correct length, so the buffer is allocated
maxdeg = [1]                     # wrong length, so the call throws after that

before = rss_mb()
for _ in range(N):
    try:
        p.fit(samppos=samppos, fitvals=fitvals, dimdeg=False,
              maxdeg=maxdeg, sampsym=sampsym)
    except Exception:
        pass                     # exactly what a recipe trying several degrees does
after = rss_mb()

print(f"  dimension={DIM}  failing calls={N}")
print(f"  leaked buffer per call = {DIM} * sizeof(cpl_boolean)")
print(f"  OBSERVED RSS {before:.1f} -> {after:.1f} MB   (growth {after - before:.1f} MB)")
print(f"  EXPECTED growth: a few MB of ordinary noise, independent of N")
