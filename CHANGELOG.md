# Change Log

Changes to this repackaging of ESO's PyCPL. ESO's own change log for the upstream
sources is kept verbatim as [CHANGELOG_orig.md](CHANGELOG_orig.md).

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versions follow ESO's, with a `.postN` suffix for this packaging. Releases before
1.0.4.post6 are documented in the git tags and in the notes at the top of the README.


## Unreleased

### Added

- CI now runs ESO's own test suites against every wheel before publishing it, via
  cibuildwheel's `test-command`. PyHDRL's suite is vendored as `tests-hdrl/` (PyCPL's
  was already in `tests/`, unrun). 1170 + 330 tests, ~30 s per wheel; all pass with the
  eight local patches applied. `patches/README.md` records what to do when a patch and
  an upstream test disagree.

### Changed

- `license` is now a PEP 639 SPDX expression rather than the deprecated TOML table, so
  the metadata carries `License-Expression: GPL-3.0-or-later` and its version moves from
  2.2 to 2.4. Requires setuptools >= 77 to build; pip < 24.2 may not read the new
  metadata version.
- Local builds and `.python-version` default to Python 3.14. Verified there, and on
  3.15.0rc2, that both suites pass and the round-trip smoke test holds; published wheels
  are unchanged at cp312/cp313/cp314.


## 1.0.4.post7

### Fixed

The build now pins `pybind11>=2.8,<3.1`. pybind11 3.1.0 (2026-08-06) changed the
`std::variant` caster so that a Python `int` or `bool` fills the `double` slot, and
PyCPL's variants declare `double`/`complex` first. Wheels built with it are badly
broken:

- every `int` and `bool` `cpl.ui.Parameter` raises "A parameter of type int does not
  match the received type" on assignment, which takes PyEsoRex down in `load_recipe`
  before any recipe can run;
- every numeric `cpl.core.Property` silently became `DOUBLE_COMPLEX` — `Property("K", 3)`
  held `(3+0j)`, `Property("K", 1.5)` held `(1.5+0j)`, `Property("K", True)` held
  `(1+0j)` — and was written to FITS headers as a complex keyword.

**v1.0.4.post5 and v1.0.4.post6 are affected and have been withdrawn from the package
index**; post4 and earlier are not. Reported upstream, reproducer in
`eso-bugs/10_pybind11_31_variant_resolution.py`. The pin is the stopgap: the real fix
is to stop resolving these variants by declaration order, which has to happen in ESO's
sources.


## 1.0.4.post6

### Fixed

The wheels now carry local fixes for defects in ESO's binding sources, applied from
`patches/` at build time. All are reported upstream, with reproducers in `eso-bugs/`.
None changes the result of a call that already succeeds against ESO's own wheels.

- `cpl.drs.detector.get_noise_window` and `get_bias_window` accept a `zone_def` window
  at all. Upstream hands CPL a pointer to a block-scoped array, so every window raised
  `IllegalInputError` from inside CPL — and being undefined behaviour, another build
  could instead have measured an arbitrary region silently.
- HDRL statistics (`get_mean`, `get_median`, ...) return a `Value` that can be pickled,
  so results can cross a process boundary and be returned from a `multiprocessing`
  worker.
- `hdrl.core.ImageList[-1]` wraps like `cpl.core.ImageList` instead of raising, and an
  out-of-range index reports its own message rather than one leaked from `std::vector`.
- `hdrl.core.Image[y, x]` works, for reading and writing. Upstream binds the dunders
  with two positional arguments, which no subscript expression can reach.
- `np.asarray(hdrl_image)` raises instead of silently returning a 0-d `dtype=object`
  array. It deliberately does not implement the conversion: which plane that should
  return is ESO's decision.
- Memory handling: mismatched `new[]`/`delete` in `Polynomial.fit` and in `Vector`,
  and leaks in `Polynomial.fit` (240 MB over 3000 failing calls at dimension 20000)
  and in `get_noise_ring`. No behaviour change.

### Changed

- ESO's `README.md` and `CHANGELOG.md` are kept verbatim as `README_orig.md` and
  `CHANGELOG_orig.md`, so they still diff cleanly against a new upstream tarball. The
  top-level files are now ours.
