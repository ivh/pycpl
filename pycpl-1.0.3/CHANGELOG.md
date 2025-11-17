# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## 1.0.3

### Fixed
- Fixed passing error information from the child process to the parent to get the correct, most recent error information displayed. See PIPE-12292. (!211)
- Check recipe return status and raise an exception on a non-zero return code, even if no previous CPL error was recoreded. The CPL plugin interface specification states that a non zero return code is sufficient to consider the recipe execution as failed. (!211)  

### Changed
- The method `cpl.ui.CRecipe.run()` assumes now that all parameters passed as the second argument were set by an explicit action. Thus, when paramters are propagated to the recipe implementation on execution, the parameter presence flag is set before a parameter is passed to the recipe implementation. This is to allow recipes relying on that flag to work properly. See PIPE-12262. (!212)


## 1.0.2

### Fixed
- Fix catching exceptions from the child process on recipe failure, allowing a calling instance (e.g. *PyEsoRex*) to react properly. See PIPE-12158. (!207)


## 1.0.1

### Added
- Added capability to run `Pyesorex` from a multiprocessing pool. See PIPE-11208. (!190)

### Changed
- Change format of `__str__` and `__repr__` methods for `Frame` and `FrameSet` classes. See PIPE-10967. (!200)
- Make `cpl::ui::CRecipe::set_recipe_dir()` follow symbolic links. (!196)
### Fixed
- Fixed segmentation fault when attempting an unsupported assignement of a scalar to an entire table column. (!199)
- Test suite failures on macOS (arm64) due to numerical noise (!198, !201)
- Cleanup of include directives and most compiler warnings. (!194) 
- Fixed test suite failures with NumPy 2.x (!192)
- Fixed building with GCC 14 and changed policy for including the header `<algorithm>`. (!191)
- Fixed the display of the dummy values of `cpl::core::Window::All`. (!189) 


## 1.0.0

### Added
- Added test cases for all functions in `cpl.core.Image` and `cpl.core.ImageList`. (!176)
- Added package version attribute `cpl.__version__`. This is taken from the package meta data in `setup.cfg` and propagated to the extension. (!166)
- Added environment variables `PYCPL_BUILD_DEBUG`, `PYCPL_BUILD_SANITIZE` and `PYCPL_BUILD_VERBOSE`. These are intended for maintainers and set up the build system for debugging, use with (address) sanitizer, and verbose compiler output. The respective compiler flags are taken from internal presets. (!166)
- Added numpy masked_array support plus tests for `cpl.core.Table` column output (normal and cpl.core.Type.ARRAY columns) where invalid data are masked. Added `cpl.core.Table` as_array attribute. (!174)
- Added test cases for testing dump functions. (!165)
- Finished docstrings for `cpl.core.ImageList`. (!152)
- Added `cpl.core.Image.as_array()` method to convert Images to numpy arrays. (!164)
- Python bindings for `cpl_image_reject_value` added as `cpl.core.Image.reject_value()` (!168) 
- Added ability to retrieve rows from `cpl.core.Table` objects as Python dictionaries. (!167)
- `sorted` methods for `cpl.core.Bivector` and `cpl.core.Vector` to return a sorted copy
of the object while leaving the original unmodified.

### Changed
- Swapped image args to (y,x) in `cpl.core.Image` and `cpl.core.Mask` to be more consistant with pythonic representation. (!179)
- Corrected docstrings at various places (!175)
- Build system modernized: switch to a PEP-517 compliant build system using `pyproject.toml` together with the setuptools backend to build the extension. Use `setup.cfg` to specify all package meta data and use `setup.py` only for building the extension. (!166)
- Remove custom commandline options passed to the build stage. This only works when `setup.py` is called directly, which is now obsolete. (!166)
- Environment variables recognized by the build system are now namespace protected, e.g. RECIPE_DIR was replaced by PYCPL_RECIPE_DIR. (!166)
- The `cpl.core.Table` __array__ now returns numpy masked arrays. (!174)
- `cpl.core.Table` getter functions now return a tuple with the value and flag indicating if the returned value is invalid (!170)
- Merged cpl.core.Image.get_xyz_window() methods with corresponding cpl.core.Image.get_xyz methods by adding optional window arguments. (!158)
- Changes the functionality of dump functions all over making it homogeneous (!165)
- `cpl.core.ImageList.collapse_sigclip_create` no longer requires a contribution map as input to be modified, it creates and returns a new contribution map. (!152)
- Reworked `cpl.core.Polynomial`, `cpl.core.Table` and `cpl.core.Vector` and constructors. Overloaded constructors have been replaced with factory function (`zeros`, `empty`) similar to numpy classes. (!153)
- Empty `cpl.core.Bivector` and `cpl.core.Vector`s now constructed using `zeros` static method similar to other classes (!177)
- `cpl.core.Bivector.interpolate_linear`, `cpl.core.Image.filter`, `cpl.core.Image.filter_mask`, `cpl.core.Image.hypot`, `cpl.core.Mask.threshold_image` and `cpl.core.Vector.correlate` now return their results in a new PyCPL object instead of requiring a pre-allocated object to store the result in. (!177)
- `cpl.core.Image.fill_gaussian` changed to `create_guassian` and `create_gaussian_like`, `cpl.core.Image.fill_jacobian` changed to `create_jacobian`, `cpl.core.Image.fill_jacobian_polynomial` changed to `create_jacobian_polynomial` and `create_jacobian_polynomial_like`, and `cpl.core.fill_kernel_profile` changed to `kernel_profile`. In each case the new methods now return their results in new PyCPL objects instead of requiring pre-allocated objects to store the result in. (!177)
- `cpl.core.Bivector.sort` and `cpl.core.Vector.sort` now use an optional `reverse` boolean keyword to specify sort direction. (!177)
- `cpl.core.Table` now returns an empty numpy array as the value when an invalid element of an array-valued column is accessed (was a single element numpy array containing `None`). (!182)

### Fixed
- Fixed the functionality of 'cplcore.Image function' set_either which accepts all data types. (!176)
- Fixed handling of optional window to Image, ImageList and Mask dump functions. (!176)
- Fixed support for cplcore.Type.ARRAY type columns of `cpl.core.Table` where the array type are strings. (!174)
- Fixed CPL and pipeline web page links. (!154)
- Fixed a typo in the user documentation. (!155)
- Fixed tests failing due to Vector API change, see PIPE-10547 (!157)
- Skip a test if numpy version < 1.20, as needed for PIPE-9418 (!157)
- Fixed code style in most Python files (!157)
- Fixed inconsistent behaviour of test_filter for PIPE-10547. (!164)
- Fixed iterating on a cpl.core.Table raising TypeError for PIPE-10761. (!167)
- Fixed `cpl.core.Image.__array__()` reshape bug, see PIPE-10547 (!177)
- Fixed a file descriptor leak in `test_dfs.py` (see PIPE-10922) and avoid further leaks by adding context managers to all file I/O in the tests. (!181)
- Linted Python code with Flake8. (!182)


## 0.9.0

### Added
- Docstrings and comments added to 'cpl.core.table_bindings' and 'cpl.core.matrix_bindings' (!156)
- Module level docstrings were added to the submodules `cpl.core`, `cpl.dfs`, `cpl.drs`, and `cpl.ui`, summarizing the provided functionality. (!139)
- `equals` function to `cpl.core.Image`, used to compare equivalence of the image to another image.
- `cpl.drs.geometric_transforms` pydocs and unit tests (!111)
- `cpl.drs.detector` pydocs and unit tests (!109)
- `DESCRIPTION` constant accessible via `cpl.DESCRIPTION`. This is a string detailing the libraries linked to CPL (including CPL itself) and their version numbers. (!131)
- large number of `cpl.core.Table` method pydocs added (!137)
- large number of `cpl.core.Matrix` method pydocs added (!137)
- `cpl.core.Image.labelise_create` added (!108)
- `cpl.core.Image` can now be deep copied (!108)
- `cpl.drs.Apertures` is now fully documented with unit tests. (!108)
- `cpl.drs.wlcalib` documented with some unit tests (!107)
- `cpl.core.Image.fft` now bound (!140)

### Changed
- parameter `zone_def` for all `cpl.drs.detector` functions is now a tuple of 4 values (data types dependent on function). The parameter is also now made optional. (!109)
- `cpl.core.ImageList.load` now loads data as CPL_TYPE_DOUBLE by default while Image.load() loads data as it is in the file. (!127)
- `cpl.core.Image` constructor simplified and better documented. Zero initialised images now constructed with `cpl.core.Image.zeros` (!134)
- `cpl.core.Image.fill_abs_arg` has been renamed to `cpl.core.Image.split_abs_arg` and `cpl.core.Image.fill_abs_arg` has been renamed to `cpl.core.Image.fill_real_imag` to be more indicative of the function's purpse. (!134)
- `cpl.core.Image.as_type` added as an alternative interface to `cpl.core.Image.cast`. (!134)
- `cpl.core.Image.fill_gaussian` now uses 0 indexed values for xcen and ycen to be more consistent with the rest of the methods (!134)
- `cpl.core.dump_window` removed, now `cpl.core.dump` supports optional window arg to provide the same functionality. (!134)
- `cpl.core.get_median_dev` now returns a tuple of two values instead of just one, as it also returns the mean absolution median devation (!134)
- FITS keyword names are now rendered as code in the documentation of `cpl.dfs`. (!139)
- `cpl.core.Matrix.solve_lu` returns a new matrix based on the solution applied to `rhs` instead of modifying `rhs` (!136)
- `cpl.core.Matrix.sort_rows` and `cpl.core.Matrix.sort_columns` now take a boolean arg instead of an integer mode to decide to sort by absolute or value (!136)
- `cpl.core.Matrix` has new init structure using optional arguments. Initialising an empty matrix now done via `cpl.core.Matrix.zeros` (!136)
- `cpl.core.Matrix.threshold_small`, `cpl.core.Matrix.is_zero`, `cpl.core.Matrix.is_diagonal` and `cpl.core.Matrix.is_identity` now have `None` as the default value for `threshold` instead of -1.0, but has the same effect. (!136)
- `cpl.core.Matrix.copy` renamed to `cpl.core.Matrix.copy_values_from` (!136)

### Fixed
- Critical warnings from Sphinx about unexpected section titles in the documentation of overloaded functions were fixed by manually formatting these sections. (!139)
- clang compiler warnings should no longer be present, making clang fully supported
- Symbols and constants from the CPL C API appearing in the documentation of `cpl.dfs` were replaced by their PyCPL counterparts. (!139)
- C++ types in the docs have been corrected to their corresponding python types
- CPL path set by environment variables CPL_ROOT now takes priority over system directories to be linked during installation
- Unit test for eval test adjusted for m1 macs due to reduced double precision
- `cpl.core.Image` equality works for images of all data types now
- `cpl.core.Matrix.is_zero`, `cpl.core.Matrix.is_diagonal` and `cpl.core.Matrix.is_identity` now return booleans (!136)
- `cpl.core.Matrix.is_diagonal` and `cpl.core.Matrix.is_identity` throws `cpl.core.IllegalInputError` rather than simply returning `False` when called on a square matrix (!136)
- `cpl.core.Table.is_selected` returns `bool` as intentded (!137)
- `cpl.core.Image` equality works for images of all data types now (!127)
- `cpl.core.Image.load` now allows specifying the image target and target window type when loading a file as it should have.  (!127)
- `cpl.drs.wlcalib.SlitModel` could not previously be initialised. This has now been fixed. (!142)
- `cpl.drs.wlcalib.SlitModel.fit_best_1d` was a typo. Corrected to `cpl.drs.wlcalib.SlitModel.find_best_1d` (!142)
- `cpl.drs.wlcalib.SlitModel.find_best_1d` returned NamedTuple previously used the wrong field names. This has now been corrected. (!142)
- `cpl.drs.fit.lvmq` no longer throws bad_function_call when PyCPL is compiled with >=gcc11 on MacOS (!141)
- `cpl.drs.geometric_transforms.img_offset_combine` warning when compiling with clang now fixed. (!145)
- `cpl.core.Table.load_window`'s ordering of arguments was previously incorrect. Now fixed. (!145)
- `cpl.drs.detector.get_noise_ring` now returns correct values on gcc (!145)
- `cpl.drs.geometric_transforms.img_offset_combine` warning when compiling with clang now fixed.
- `cpl.core.Matrix.solve_lu` copies `rhs` and returns the modification rather than modifies `rhs` inplace (!136)


## 0.4.0

### Added

- New PyEsoRex options/parameters:
  * `--output-dir` command line option, `output_dir` parameter and Pyesorex property, with input validation. (!119)
  * `--output-mkdir` command line option & parameter. (!119)
  * `--output-prefix` and `--suppress-prefix` command line options & parameters. (!119)
- Added 2 new `append` interfaces for `cpl.core.PropertyList`, including appending another propertylist, or appending a new property without needing to create the property first (by value) (!121)

### Changed

- Validation tests now use PyEsoRex instead of the `cpl.ui` plugin interface directly. (!119)
- Replaced test in `tests/cplui/test_recipe_param.py` with a more rigorous one. (!119)
- `cpl.core.Polynomial.fit` parameters `fitsigm` and `cpl.core.Polynomial.fit_residual` parameters `fitvals` and `fitsigm` no longer support iterable types and require cpl.core.Vector for consistency and functionality with clang. (!120)
- `cpl.core.Bivector`,`cpl.core.PropertyList`, `cpl.core.Property` and `cpl.core.Table` no longer have functions that return `self` when called as these are functions that just perform an inplace modification. These functions now return `None` (!121)

### Fixed
- `cpl.core.Polynomial.fit` previous unusable, now usable with `sampsym` and `dimdeg` now takes in a python compatible type (from `cpl_boolean` to `bool`). (!120)
- Segfaults with clang-compiled PyCPL when using `cpl.core.Polynomial.fit`, `cpl.core.Polynomial.eval` or `cpl.core.Polynomial.fit_residual` no longer present.  (!120)
- DFS test changes cwd to only include the filename in the PIPEFILE headers. Tests should now pass on Mac. (!118)

### Removed

- PyCPL no longer redirects recipe output files from current working directory to `./products/<recipename>NNN` (!119)


## 0.3.1

### Changed

- Updated outdated PyEsoRex help text (see https://jira.eso.org/browse/PIPE-10045, !115)

### Fixed

- Hotfix for Macs compiling with XCode clang no longer fails (!114) although issues are still present and thus not yet considered compatible.
- Warning for use of deprecated `std::iterator` no longer present.
- `cpl.core.Polynomial.eval_2d` and `cpl.core.Polynomial.eval_3d` segfault errors fixed.
- Apertures test corrected.
- Fixed critical PyEsoRex bugs
  * PyEsoRex would raise a `FileNotFoundError` exception if started when no config file existed at the default location `~/.pyesorex/pyesorex.rc` (see https://jira.eso.org/browse/PIPE-10045, !115)
  * It was not possible to change any PyEsoRex parameters from their default values due to a bug in the `pyesorex.parameter.Parameter` classes (!115)
  * It was not possible to create config files with PyEsoRex because the `CreateConfigAction` and `Pyesorex.write_config()` method had not been fully updated for the refactored parameter/command line argument handling (!115)
  * Trying to read recipe config files with a specified name resulted in an `UnboundLocalError`
  (see https://jira.eso.org/browse/PIPE-10045, !115)


## 0.3.0

### Added
- Bindings have been extended for adding in all modules of cpl.drs. Note that some of these are still work in progress, and have yet to be properly tested and documented:
  * cpl.drs.wlcalib
  * cpl.drs.geometric_transforms (cpl_geom_img)
  * cpl.drs.photom
  * cpl_detector
  * cpl.wcs
- Addition of the new 7.2 functions including:
  * cpl.core.Polynomial.eval_2d() added
  * cpl.core.Polynomial.eval_3d() added
- Documentation completeness: many functions that did not have pydocs are now present and now much readable using the sphinx generated reference manual.
- 7.2 function expansions:
  * cpl.core.Matrix.solve_svd() now supports thresholding
  * cpl.core.Polynomial.fit() now supports fitsigm as an input parameter

### Changed
- Refactoring of PyEsoRex parameter & command line argument handling.
- Buildsystem updates for ease of installation:
  * The build system is updated to locate CPL via find package.
  * When building the project the setup.py has been extended to allow setting the CPL location, and the default built-in recipe search directory via command line options.
  * Allows locating CPL by setting CPL_ROOT env variable to the cpl installation directory
- Unit test restructuring for better organisation

### Fixed
- Fixed issues with some PyEsoRex command line arguments being ignored under certain circumstances.
- Fedora build no longer generates compiler warnings
- gcc requirements reduced to 9.3.1


## 0.2.0

### Added
- Python bindings of CPL fitting functions:
  - Binding for `cpl_fit` now accessible under `cpl.drs.fit`
  - Binding for function `cpl_vector_fit_gaussian` as `cpl.core.Vector.fit_gaussian`
- Addition of the parameter presence flag as a property of `cpl.ui.Parameter` classes. This can be set either manually or automatically upon setting the value of the parameter.
- API Reference Manual for both PyCPL and PyEsoRex, generated from docstrings using Sphinx. See README.md for build instructions.
- Installation options to install the requirements for building docs or running tests. See README.md for details.
- PyEsoRex now signs data products returned by recipes. Options have been added to disable either the checksums or data MD5 sums in the recipe products.
- Change log.

### Fixed
- Compiler warning on some systems being raised on a custom type caster as detailed in https://jira.eso.org/browse/PIPE-9848. This has now been fixed.
