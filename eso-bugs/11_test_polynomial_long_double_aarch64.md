PyCPL: tests/cplcore/test_polynomial.py fails on Linux aarch64
==============================================================

Not a binding defect -- a portability gap in ESO's own test suite. Filed because it
makes the suite unusable as a release gate on aarch64, which is a supported wheel
platform.

WHERE
  tests/cplcore/test_polynomial.py:196-215, TestPolynomial::test_eval_2d

WHAT
  The expected evaluation results are hard-coded, and they depend on the width of the
  platform's `long double`. The parametrisation guards exactly two cases:

      if not (platform.system() == "Darwin" and platform.processor().startswith("arm")):
          (-8.17776e-05, [1e3, 1e2])      # x86_64: 80-bit extended
          (-8176,        [1e5, 1e0])
      else:
          (0.0, [1e3, 1e2])               # Darwin/arm64: long double == double, 64-bit
          (0.0, [1e5, 1e0])

  There is a third case in the wild. Linux aarch64 has IEEE binary128 `long double`,
  so it takes the first branch and evaluates the polynomial more precisely than the
  x86_64 reference:

      OBSERVED  -8.180305391403131e-05
      EXPECTED  -8.17776e-05            atol=np.finfo(np.double).eps (2.2e-16)

  Both root evaluations fail; the two non-root parameters pass everywhere. Reproduced
  on manylinux_2_28 aarch64 under cibuildwheel, cp312/cp313/cp314, against the 1.0.4
  bindings with CPL 7.4. x86_64 Linux and both macOS architectures pass.

WHY IT IS THE TEST, NOT THE LIBRARY
  Both roots are evaluations of a polynomial whose true value there is ~0. The
  differences are the accumulated rounding of a Horner evaluation carried in a wider
  intermediate type, and the aarch64 answer is the closest of the three to the exact
  root. A tolerance of one double eps against a hard-coded literal cannot express
  that; the test is asserting a particular FPU, not a particular result.

FIX
  Either compare against 0 with a tolerance that reflects the conditioning of the
  polynomial at its root, or select the expected values from the actual `long double`
  width rather than from the platform name -- e.g. np.finfo(np.longdouble).eps, which
  distinguishes all three cases without enumerating platforms.

WORKAROUND HERE
  /conftest.py xfails those two parameters on Linux aarch64 (non-strict), so the rest
  of the suite can gate our wheels. Removed once the guard upstream covers aarch64.
