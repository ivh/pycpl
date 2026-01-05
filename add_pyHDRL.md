# Adding PyHDRL to the PyCPL Package

## Summary

This document captures the analysis and plan for integrating HDRL and PyHDRL into the existing PyCPL "batteries included" package at `/Users/tom/pycpl.git/`.

**Goal**: One wheel, one install:
```bash
pip install pycpl --extra-index-url https://ivh.github.io/pycpl/simple/
```
```python
import cpl
from cpl import hdrl  # hdrl is a submodule of cpl
# or: import cpl.hdrl as hdrl
```

The package keeps the name `pycpl` since CPL is the foundation that HDRL builds on.

## Current Status

### DONE:
- [x] Analyzed dependencies - libcurl NOT needed
- [x] Unpacked and cleaned vendor sources (removed docs, tests, changelogs)
- [x] Copied `src/hdrlcore/`, `src/hdrlfunc/`, `src/hdrldebug/` to `/Users/tom/pycpl.git/src/`
- [x] Renamed conflicting functions (`bind_errors` → `bind_hdrl_errors`, etc.)
- [x] Modified `pycpl.cpp` to add `cpl.hdrl` submodule with core/func/debug

### TODO:
- [ ] Move vendor sources to `/Users/tom/pycpl.git/vendor/`:
  - `gsl-2.8/` (18MB cleaned)
  - `erfa-2.0.1/` (2.7MB)
  - `hdrl-1.6.0a/` (4MB cleaned)
- [ ] Update `setup.py` - add GSL, ERFA, HDRL build phases
- [ ] Update `CMakeLists.txt` - add hdrl source files and link against HDRL
- [ ] Patch HDRL to make libcurl optional (or stub out hdrl_download.c)
- [ ] Test build

## Background: Library Relationships

```
Python layer:
  cpl.hdrl module  ──uses──>  cpl module
       │                           │
       │ pybind11                  │ pybind11
       ▼                           ▼
C layer:
  HDRL library  ───depends on───>  CPL library
       │                                │
       │                                ├── cfitsio
       │                                ├── fftw
       │                                └── wcslib
       │
       ├── GSL (GNU Scientific Library)
       └── ERFA (Essential Routines for Fundamental Astronomy)
```

### How PyHDRL uses PyCPL

PyHDRL independently wraps HDRL using pybind11. At the Python API level, functions accept and return PyCPL types (`cpl.core.Image`, `cpl.core.Table`, etc.) for seamless interoperability.

Custom pybind11 type casters in `src/hdrlcore/pycpl_*.hpp` convert between:
- Python PyCPL objects (e.g., `cpl.core.Image`)
- C pointers (e.g., `cpl_image*`)

Usage:
```python
import cpl
from cpl import hdrl

cpl_img = cpl.core.Image(numpy_array)
hdrl_img = hdrl.core.Image(cpl_img, cpl_err_img)
result = hdrl.func.some_algorithm(hdrl_img)
```

## Key Discovery: libcurl is NOT Required

HDRL's `hdrl_download.c` provides generic URL download utilities using libcurl, BUT:

1. **No HDRL algorithm uses it internally** - just a utility for pipeline developers
2. **PyHDRL doesn't bind it** - no Python interface exposed
3. **Only used in unit tests** - downloads Earth Orientation Parameters for testing

**Conclusion**: libcurl can be safely excluded. This removes the most problematic dependency.

## Dependencies

### Already bundled in PyCPL (`/Users/tom/pycpl.git/vendor/`):
| Library | Size |
|---------|------|
| cfitsio-4.6.2 | 7.9MB |
| fftw-3.3.10 | 24MB |
| wcslib-8.2.2 | 7.5MB |
| cpl-7.3.2 | 14MB |

### To add (cleaned, in `/Users/tom/pyhdrl.git/`):
| Library | Version | Size | Download |
|---------|---------|------|----------|
| **GSL** | 2.8 | 18MB | https://ftp.gnu.org/gnu/gsl/gsl-2.8.tar.gz |
| **ERFA** | 2.0.1 | 2.7MB | https://github.com/liberfa/erfa/releases/download/v2.0.1/erfa-2.0.1.tar.gz |
| **HDRL** | 1.6.0a | 4MB | (local) |

**Removed from sources**: `gsl-2.8/doc/` (13MB), `hdrl-1.6.0a/tests/` (1.2MB), ChangeLog/NEWS files

### NOT needed:
| Library | Reason |
|---------|--------|
| libcurl | Download module not used by any algorithm, not bound by PyHDRL |

## Source Integration (DONE)

### Files copied to `/Users/tom/pycpl.git/src/`:
```
src/
  cplcore/          ← existing (from PyCPL)
  cpldfs/           ← existing
  cpldrs/           ← existing
  cplui/            ← existing
  hdrlcore/         ← NEW (from PyHDRL)
  hdrlfunc/         ← NEW (from PyHDRL)
  hdrldebug/        ← NEW (from PyHDRL)
  dump_handler.*    ← shared (identical in both)
  path_conversion.hpp ← shared (identical in both)
  pycpl.cpp         ← MODIFIED (added hdrl init)
```

### Function renames (to avoid linker conflicts):
| Original | Renamed |
|----------|---------|
| `bind_errors` (hdrlcore) | `bind_hdrl_errors` |
| `bind_image` (hdrlcore) | `bind_hdrl_image` |
| `bind_imagelist` (hdrlcore) | `bind_hdrl_imagelist` |
| `bind_types` (hdrldebug) | `bind_hdrl_types` |

### Changes to `pycpl.cpp`:
- Added includes for all `hdrlcore/`, `hdrlfunc/`, `hdrldebug/` bindings
- Added `cpl.hdrl` submodule with `core`, `func`, `debug` sub-submodules
- Calls renamed `bind_hdrl_*` functions

## Build Process (Proposed)

**Phase 1** (parallel): cfitsio + fftw + **GSL** + **ERFA**
**Phase 2**: wcslib (depends on cfitsio)
**Phase 3**: CPL (depends on cfitsio, fftw, wcslib)
**Phase 4**: **HDRL** (depends on CPL, GSL, ERFA) - configure with `--enable-standalone`
**Phase 5**: Build pybind11 extension (now includes hdrl bindings)
**Phase 6**: Copy all .so/.dylib to wheel

### HDRL Build Configuration
```bash
./configure \
  --prefix=$INSTALL_DIR \
  --enable-standalone \
  --with-cpl=$INSTALL_DIR \
  --with-gsl=$INSTALL_DIR \
  --with-erfa=$INSTALL_DIR
```

Need to either:
- Use `--without-curl` if supported
- Or patch `configure.ac` / stub out `hdrl_download.c`

## Files Still To Modify

### `/Users/tom/pycpl.git/setup.py`:
- Add `_build_gsl()` function (CMake or autotools)
- Add `_build_erfa()` function (autotools)
- Add `_build_hdrl()` function (autotools with `--enable-standalone`)
- Update `_copy_vendored_libraries()` for GSL, ERFA, HDRL libs

### `/Users/tom/pycpl.git/CMakeLists.txt`:
- Add hdrl source files to compilation
- Add FindHDRL.cmake or direct detection
- Link against `HDRL::hdrl` (or `${HDRL_LIBRARIES}`)

### `/Users/tom/pycpl.git/pyproject.toml`:
- Update package metadata/description

## Upgrade Process (Future)

When ESO releases new versions:
- **New PyCPL**: Replace `src/cplcore/`, `src/cpldfs/`, `src/cpldrs/`, `src/cplui/`
- **New PyHDRL**: Replace `src/hdrlcore/`, `src/hdrlfunc/`, `src/hdrldebug/`
  - Re-apply function renames (`bind_errors` → `bind_hdrl_errors`, etc.)
- **pycpl.cpp**: Small file, manual merge if needed (just init code)

## Wheel Size Estimate

Current PyCPL wheel: ~12MB

Additional compiled libs:
- GSL: ~2.5MB
- ERFA: ~200KB
- HDRL: ~1MB

**Estimated total: ~16MB**

## Open Questions

1. ~~Which GSL version?~~ → **2.8** (latest stable)
2. ~~Which ERFA version?~~ → **2.0.1** (latest stable)
3. Does HDRL's configure support `--without-curl`? If not, need to patch
4. Keep PyHDRL tests separate or merge into PyCPL test suite?
5. Version numbering for combined package?

## References

- PyCPL repo: `/Users/tom/pycpl.git/`
- PyHDRL source: `/Users/tom/pyhdrl.git/pyhdrl-0.1.0/`
- HDRL source: `/Users/tom/pyhdrl.git/hdrl-1.6.0a/`
- Vendor sources (cleaned): `/Users/tom/pyhdrl.git/gsl-2.8/`, `erfa-2.0.1/`, `hdrl-1.6.0a/`
- PyHDRL user guide: https://www.eso.org/sci/software/pycpl/pyhdrl-site/userguide.html
