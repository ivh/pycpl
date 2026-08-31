# Change Log

Changes to this repackaging of ESO's PyCPL. ESO's own change log for the upstream
sources is kept verbatim as [CHANGELOG_orig.md](CHANGELOG_orig.md).

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versions follow ESO's, with a `.postN` suffix for this packaging. Releases before
1.0.4.post6 are documented in the git tags and in the notes at the top of the README.


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
