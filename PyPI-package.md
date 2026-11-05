# Publishing on PyPI — on hold

Goal: an ESO pipeline (pycpl + pyesorex + edps + recipes) fully `pip install`-able for
colleagues. Status as of 2026-11-05: **parked**. Nothing in the repo has been changed for it.

## Why it's parked

ESO's `pyesorex` (1.0.4, `setup.cfg`: `install_requires = pycpl`) depends on a
distribution named **`pycpl`**, and edps presumably does too. Python packaging matches
dependencies by distribution name only, and there is no "provides"/"obsoletes", so a
`pycpl-standalone` on PyPI can never satisfy that requirement. Neither `pyesorex` nor
`edps` is on PyPI, so a PyPI-only install of the pipeline is impossible anyway, and a
user who mixes indexes gets ESO's `pycpl` installed over ours (both write `cpl/`).

PyPI only pays off if pyesorex and edps are on PyPI too, and republishing them
ourselves is not reasonable. Ways forward, should this come back:

1. ESO publishes pyesorex/edps on PyPI, or makes their `pycpl` requirement satisfiable
   (e.g. optional or behind an extra). Their call, to be raised with them.
2. Publish the same wheels under two names: `pycpl-standalone` on PyPI for people who
   only want the bindings, and `pycpl` on the Pages index for the pipeline. CI would
   rename the dist-info (`METADATA` Name, dist-info directory, `RECORD`) instead of
   building twice. Downside: installing both gives two unrelated distributions owning
   the same files.
3. Users who want the PyPI name together with pyesorex use a uv override,
   `override-dependencies = ["pycpl; sys_platform == 'never'"]`, or pip `--no-deps`.
   Too clunky to be the default.

## Name

- `pycpl` is not taken on PyPI, and ESO does not publish there. It is blocked because
  PyPI's confusable-name check treats `l`/`i`/`1` as the same letter, so `pycpl`
  collides with the unrelated **`pycpi`**. The reason given in CLAUDE.md ("too similar to
  ESO's pycpl") is wrong.
- Chosen name: **`pycpl-standalone`**. It follows HDRL's own `--enable-standalone`
  wording and says no system CPL is needed. Runner-up: `pycpl-selfcontained`.
- All free on 2026-11-05: pycpl-standalone, -selfcontained, -bundled, -portable,
  -prebuilt, -vendored, -unofficial, -wheels, -bin. Rejected: `eso-cpl` (implies ESO
  publishes it), `-static` (the bundled libraries are shared, not static), `-full`/`-complete`
  (suggests ESO's version lacks something).
- `cpl` on PyPI is an unrelated "copy-paste loop" package.
- The import names stay `cpl` and `hdrl` whatever the distribution is called.

## Licensing: no obstacle

| component | licence |
|---|---|
| PyCPL/PyHDRL bindings (`src/`) | GPL-3.0-or-later |
| CPL 7.4, HDRL 1.6.0, FFTW 3.3.10 | GPL-2.0-or-later |
| GSL 2.8 | GPL-3.0-or-later |
| WCSLIB 8.2.2 | LGPL-3.0-or-later |
| CFITSIO | NASA permissive (notice "in all copies") |
| ERFA | BSD-3-Clause (notice required in binary redistributions) |
| pybind11 (compiled in) | BSD-3-Clause |

The combined work is GPL-3.0-or-later, matching `pyproject.toml`. Redistributing it needs
no permission from ESO. Do not imply that ESO endorses it.

## Source and licence notices

- **Licence notices**: fixed in 1.0.4.post10. `license-files` now lists every vendored
  licence, plus `licenses/pybind11-LICENSE`.
- **Corresponding source**: for the Pages wheels, the tagged GitHub repo covers this. For
  PyPI, An sdist (it carries `vendor/`, `src/` and `patches/` via
  `MANIFEST.in`) would meet GPLv3 §6. `vendor/` is 84 MB unpacked, so the compressed sdist
  should fit PyPI's 100 MB file limit. It must be built from a clean checkout, because of
  `global-include *.dylib`.

## If it is resumed: remaining steps

1. README/CHANGELOG/CLAUDE.md: the new install line, a "not affiliated with ESO" note,
   and where to report which bugs (CPL/HDRL bugs to ESO, packaging bugs here).
2. An sdist job, as above.
3. `name` in `pyproject.toml`, a PyPI job using Trusted Publishing (the pending publisher
   must be registered on pypi.org by hand first), and a decision on what
   `update-index.sh`/`simple/pycpl/` serves (pip ignores wheels whose filename doesn't
   match the project name).
4. Release the next `.postN` as usual and check that it appears on PyPI.
