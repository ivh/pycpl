# PyCPL with batteries included

**This is an unofficial re-packaging of ESO's PyCPL**

In contrast to the ESO's own package, which is available from [their own index](https://ftp.eso.org/pub/dfs/pipelines/libraries/) but not from PyPI, this one comes with the necessary C-libraries (CPL, cfitsio, wcslib, fftw) included, so they don't have to be installed separately. It is served from [our own index](https://ivh.github.io/pycpl/simple/) — see below on how to use it.

As of January 2026 this package also contains
[pyHDRL](https://www.eso.org/sci/software/pyhdrl/pyhdrl-site/index.html),
including pre-built HDRL and its dependencies.

A few things to note:
* This a quick afternoon-project and there are no guarantees on how well it works. Pull requests welcome. All credit goes to the original library authors and to ClaudeCode for figuring out how to put together this package.
* There is a GitHub workflow that builds pre-compiled wheels for Python 3.12 to 3.14 on Linux and MacOS, so installation should be very quick. Lower Python versions might work, but need build tools (cmake, autoconf, automake, libtool) and will trigger lengthy compilation when installing. No Windows support at all.
* For technical details on the build system, see [CLAUDE.md](CLAUDE.md).
* I recommend to use [uv](https://docs.astral.sh/uv/) for this package (and for everything else). But it should work just as well with `pip` or other Python tools.
* To see if this package works on your machine run `uv run -p 3.14 --index https://ivh.github.io/pycpl/simple/ --with pycpl python -c "import cpl; import hdrl; print('Yay')"`
* For local development, use `uv sync --no-install-project` since editable installs don't work with the complex native build.
* There is also the script [`pyhdrl_demo.py`](https://raw.githubusercontent.com/ivh/pycpl/refs/heads/master/pyhdrl_demo.py) that you can download alone, and simply run with `uv run pyhdrl_demo.py`. It should find this package and use it.
* I chose the package version number the same as ESO's, but appending *.postNN* which means it's higher and takes precedence but will not interfere with their future versioning. If you want original pycpl from ESO, use only their index and/or install the fixed version number like *pycpl==1.0.3* .
* **Version 1.0.4.post2 upgrades PyHDRL from 0.2.0 to 1.0.0, which changes the HDRL API.** `hdrl.core.Parameter` was removed, `hdrl.core.Spectrum1D`/`Spectrum1DList` were refactored, and `hdrl.func.Efficiency`/`Response` now take specific parameter types (`EfficiencyParameter`, `ResponseFitParameter`, ...) instead of the opaque `Parameter`. See ESO's [PyHDRL changelog](https://ftp.eso.org/pub/dfs/pipelines/libraries/pyhdrl/). This release also moves the bundled C libraries to CPL 7.4 and HDRL 1.6.0a5, as required by PyCPL 1.0.4.
* **macOS: if you have ESO pipelines installed, unset `DYLD_LIBRARY_PATH` when using this package.** An import failing with e.g. `Symbol not found: _cpl_wcs_duplicate ... Expected in: <your CPL>/lib/libcpldrs.26.dylib` means macOS loaded your *system* CPL instead of the bundled one: `DYLD_LIBRARY_PATH` (set by the usual `CPLDIR` setup for `esorex`) is searched by library filename before the wheel's own `@rpath`, so the bundled libraries get shadowed. Run the command with the variable cleared:

```
env -u DYLD_LIBRARY_PATH uv run yourscript.py
```

  Linux is unaffected — the wheels use `DT_RPATH`, which wins over `LD_LIBRARY_PATH`.
* The installation instructions in ESO's own README (kept verbatim as [README_orig.md](README_orig.md)) do not apply to this package. Instead do `(uv) pip install pycpl --extra-index-url https://ivh.github.io/pycpl/simple/` or add the URL to your *pyproject.toml*. Like this, `uv sync` will install *pycpl* from here and *pyesorex* and *edps* from ESO, having them use the bundled pycpl:

```
[project]
# ...
dependencies = ['pycpl','pyesorex','edps']

[tool.uv.sources]
pycpl = { index = "pycpl" }

[[tool.uv.index]]
name = "pycpl"
url = "https://ivh.github.io/pycpl/simple/"

[[tool.uv.index]]
name = "eso"
url = "https://ftp.eso.org/pub/dfs/pipelines/libraries/"
```

## Bug fixes carried on top of ESO's sources

These wheels are not a plain repackaging: the C++ binding sources in `src/` are pristine
upstream, but `setup.py` applies the patches in [`patches/`](patches/) before building.
Each one fixes a defect reported to ESO, with a reproducer in [`eso-bugs/`](eso-bugs/),
and is dropped again as soon as an upstream release contains the fix.

| fix | effect |
|---|---|
| `zone_def` lifetime | `cpl.drs.detector.get_noise_window`/`get_bias_window` accept a window at all. Upstream passes CPL a pointer to a dead stack array, so every `zone_def` raises `IllegalInputError` — and being undefined behaviour, another build could instead measure an arbitrary region without saying so. |
| `Value` picklable | HDRL statistics (`get_mean`, `get_median`, ...) can cross a process boundary, so results can be returned from a `multiprocessing` worker. |
| `ImageList` indexing | `hdrl.core.ImageList[-1]` wraps like `cpl.core.ImageList`, and out-of-range raises a real message. |
| `Image` subscript | `hdrl.core.Image[y, x]` works, for reading and writing. Upstream binds the dunders so that no subscript expression can reach them. |
| `np.asarray` refusal | `np.asarray(hdrl_image)` raises instead of silently returning a 0-d `dtype=object` array. It deliberately does not *implement* the conversion — which plane that should return is ESO's call. |
| memory handling | Mismatched `new[]`/`delete` in `Polynomial.fit` and `Vector`, and leaks in `Polynomial.fit` and `get_noise_ring`. No behaviour change. |

The rule these follow: **code that works against ESO's wheels keeps working against
these.** No patch changes the result of a call that already succeeds upstream — the
reverse is not true, so a script that relies on `img[y, x]` or a windowed `get_noise_window`
will need ESO's fix before it runs on their build. `patches/README.md` has the details.
