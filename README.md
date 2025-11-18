# PyCPL with batteries included

**This is an unofficial re-packaging of ESO's PyCPL**

In contrast to the ESO's own package, which is available from [their own index](https://ftp.eso.org/pub/dfs/pipelines/libraries/) but not from PyPI, this one comes with the necessary C-libraries (CPL, cfitsio, wcsloib, fftw) included, so they won't have to be installed separately and made to be found by the package.

Things to note:
* I chose the package version number the same as ESO's, but appending *post1* which means it's higher and takes precedence but will not interfere with their future versioning. If you want original pycpl from ESO, install fixed version number like pycpl==1.0.3 .
* For technical details on the build system, see [CLAUDE.md](CLAUDE.md).
* There is a GitHub workflow that build pre-compiled wheels for Python 3.11 to 3.14 on Linux and MacOS, so installation should be very quick. No Windows support.
* This a quick afternoon-project and there are no guarantees on how well it works. Pull requests welcome. All credit goes to the original library authors and to ClaudeCode for figuring out how to put together this package.
* The installation instructions below do not apply to this package. Just use it like any other Python package with *uv*, *pip*, etc.

Original ESO README from here on:

# PyCPL &mdash; Python 3 bindings for the ESO Common Pipeline Library

PyCPL provides Python 3 bindings for the ESO Common Pipeline Library (CPL) via Pybind11. It allows for using the ESO Common Pipeline Library in Python scripts, or the Python interpreter, and thus allows for implementing instrument individual pipeline recipes or entire instrument pipelines in Python. In addition to providing an interface to implement pipeline recipes in Python, it also provides the necessary Python bindings to execute the recipes of traditional instrument pipelines implemented directly in C/C++ using the ESO Common Pipeline Library (CPL). Together with the command line tool PyEsoRex, a drop-in replacement for the EsoRex command line tool, any combination of instrument pipeline recipes implemented in Python or C/C++ can be executed transparently from the shell prompt with the same user experience as provided by the EsoRex tool.

## What PyCPL consists of

PyCPL is organized into four Python sub-modules, one for each of the four main CPL component libraries: _libcplcore_, _libcplui_, _libcpldfs_, and _libcpldrs_. The corresponding PyCPL modules are:

- **cpl.core** which provides access to the core PyCPL/CPL classes like _Image_, _ImageList_, _Table_, _Property_ and _PropertyList_, _Vector_, _Matrix_, the functions to manipulate them, and provisions for error and exception handling,
- **cpl.ui** which provides the means to interface with Python and native recipe implementation. It provides the classes _Parameter_ and its subclasses, _ParameterList_, _Frame_, _FrameSet_, and the interfaces _PyRecipe_ and _CRecipe_,
- **cpl.dfs** which provides the functions to write data products complying to the ESO data standards,
- **cpl.drs** which provides selected, higher level data reduction functions, including sub-modules for point pattern matching and fast fourier transformation 

These submodules provide the bindings for all major CPL APIs. For the full list of provided sub-modules, classes, and functions, refer to the [PyCPL API reference](https://www.eso.org/sci/software/pycpl/pycpl-site/reference.html).

## PyCPL features

Once installed, PyCPL can be imported from python as a module named `cpl`.
```python
import cpl
```

### Numpy array compatible objects
Some CPL objects can be converted to numpy arrays, for example
```python
im=cpl.core.Image([[1,2,3],[4,5,6]])
as_arr = np.array(im)
```
### Error handling
CPL functions may raiseemit CPL specific error codes. PyCPL will throw any detected error codes as a custom exception. For example:
```python
mask = cpl.core.Mask(-3,-3) # Raises IllegalInputError
```

The custom PyCPL exceptions are part of the submodule `cpl.core`. The following is the list of custom PyCPL exceptions and their respective base classes:
- AccessOutOfRangeError(Error, builtins.LookupError)
- AssigningStreamError(Error, builtins.RuntimeError)
- BadFileFormatError(Error, builtins.RuntimeError)
- ContinueError(Error, builtins.RuntimeError)
- DataNotFoundError(Error, builtins.RuntimeError)
- DivisionByZeroError(Error, builtins.RuntimeError)
- DuplicatingStreamError(Error, builtins.RuntimeError)
- EOLError(Error, builtins.RuntimeError)
- ErrorLostError(Error, builtins.RuntimeError)
- FileAlreadyOpenError(Error, builtins.RuntimeError)
- FileIOError(Error, builtins.RuntimeError)
- FileNotCreatedError(Error, builtins.RuntimeError)
- FileNotFoundError(Error, builtins.RuntimeError)
- IllegalInputError(Error, builtins.ValueError)
- IllegalOutputError(Error, builtins.RuntimeError)
- IncompatibleInputError(Error, builtins.ValueError)
- InvalidTypeError(Error, builtins.RuntimeError)

These exceptions can be caught via their PyCPL name or their Python base class name:
```python
try:
    cpl.core.Mask(-3,-3)
except cpl.core.IllegalInputError:
    print("Mask failed to build due to negative indices")
```
or
```python
try:
    cpl.core.Mask(-3,-3)
except ValueError:
    print("Mask failed to build due to negative indices")
```

### Frames & Sets
Frame objects can be created from a filename and metadata:
```python
frameDefaultsToNones = cpl.ui.Frame("existing_file.fits")

 frameWithParameters = cpl.ui.Frame(
    "/path/to/fitsfile.fits", tag="BIAS",
    group=cpl.ui.Frame.FrameGroup.CALIB
)
```
FrameSets can either be created:
```python
fromScratch = cpl.ui.FrameSet()
fromScratch.append(frameWithParameters)
```
Or imported from a .sof (Set of Frames) file:
```python
fromSOF= cpl.ui.FrameSet("/path/to/frameset.sof")
firstFrame = frameSOF[0]
```

### Executing CRecipes

To execute a native recipe, first fetch the recipe object (by name). Then, call its `run` method with the input parameters and
a frameset.
```python
giwcal = cpl.ui.CRecipe("giwavecalibration")
output_frames = giwcal.run(fromSOF, {"giraffe.wcal.rebin": True})
```
A list of available recipes can be obtained by querying the read-only property `cpl.ui.CRecipe.recipes`.

## User Documentation

The documentation for the general user, including download links, installation instructions, tutorial, and the API reference are available on the [PyCPL web site](https://www.eso.org/sci/software/pycpl/). If you are interested in installing and using PyCPL for production use it is recommended that you continue reading there.

## Building and Installing PyCPL

The following instructions are directed towards developers and experienced users. Please note that they are not a step by step tutorial!

### Software prerequisites

* cpl >= 7.2.2
* python >= 3.9
* Python development headers 
    * package python3-devel (On Fedora, Red Hat, CentOS, etc.)
    * package python3-dev (On Ubuntu, Debian, etc.)
* gcc >= 9.5
* pybind11 >= 2.8.0
* cmake >= 3.12

#### Installing pybind11

You can choose either the packaged pybind from your linux vendor if you are not using any virtual environment, e.g. for RPM based distribution (Fedora, RedHat, CentOS, etc.):
```shell
sudo dnf install python3-pybind11
```
**or** you can install pybind11 with `pip`.
```shell
python3 -m pip install pybind11
```
In general the preferred method of installing pybind11 is to use using `pip`.

#### Installing CPL

In order to install PyCPL an installation of CPL, including the development files are needed. A compatible, public version can be obtained from
the ESO repositories as RPM or MacPorts package. To setup the ESO repositories follow the instructions for [RPM packages](https://www.eso.org/sci/software/pipelines/installation/rpm.html) and [MacPorts packages](https://www.eso.org/sci/software/pipelines/installation/macports.html) respectively. CPL can also be installed from the public source packages which can be obtained from the [CPL download page](https://www.eso.org/sci/software/cpl/download.html).

 For developers development versions of CPL are also available from the ESO [GitLab](https://gitlab.eso.org) repositories (**ESO internal access only**). Building CPL from a checked out working copy requires additional (build-time) dependencies, like the GNU autotools (autoconf, automake and libtool) and the libltdl development package, to be installed.

The typical installation process is the following, when installing:

* from the ESO Fedora repositories:
  ```shell
  sudo dnf install cpl-devel
  ```
* from the ESO MacPorts repositories:
  ```shell
  sudo port install cpl
  ```
* from a public release tar archive:
  ```shell
  tar -zxvf cpl-7.3.2.tar.gz
  cd cpl-7.3.2
  ./configure --prefix=/usr/local --with-cfitsio=/usr/local --with-wcslib=/usr/local --with-fftw=/usr/local
  make
  sudo make install
  ``` 
* from an ESO GitLab working copy:
  ```shell
  git clone https://gitlab.eso.org/pipelines/cext.git
  cd cext
  ./autogen.sh
  ./configure --prefix=/usr/local
  make
  sudo make install
  cd ..
  git clone https://gitlab.eso.org/pipelines/cpl.git
  cd cpl
  ./autogen.sh
  ./configure --prefix=/usr/local --with-cfitsio=/usr/local --with-wcslib=/usr/local --with-fftw=/usr/local --with-system-cext --with-cext=/usr/local
  make
  sudo make install
  ```

### PyCPL

For production use, public PyCPL source distribution (sdist) packages are available from the [ESO ftp server](https://ftp.eso.org/pub/dfs/pipelines/libraries/pycpl/). Installing the downloaded PyCPL source distribution can then be done like:

* User-space installation, no venv/conda:
  ```shell
  python3 -m pip install --user -U pycpl-1.0.0.tar.gz
  ```
* Installation inside a Python 3 venv/conda environment:
  ```shell
  python3 -m pip install -U pycpl-1.0.0.tar.gz
  ```

Developers and contributors rather should clone the PyCPL repository from the ESO GitLab (**ESO internal access only!**) to obtain the
PyCPL source code.

Then, for installing PyCPL from within the source tree (from an unpacked source distribution, or a working copy):

* User-space pip install, no venv/conda:
  ```shell
  cd pycpl
  python3 -m pip3 install --user -U .
  ```
* Installation inside a Python 3 venv/conda environment:
  ```shell
  cd pycpl
  python3 -m pip install -U .
  ```
pip will automatically install the required Python dependencies (astropy, numpy, pybind11, setuptools).

Environment variables may be required if your CPL installation is not in its expected location. The CPL installation will be found automatically, if it is installed in the system using RPM or MacPorts packages. For manual installations it is safe to assume that the installation directory of CPL must be set explicitly. Non-default locations can be set with the environment variables `CPL_ROOT` **or** `CPLDIR`. If both variables are set, `CPL_ROOT` takes precedence.

When environment variables like `CPLDIR` need to be used the installation commands become
* User-space pip install, no venv/conda:
  ```shell
  cd pycpl
  CPLDIR=/Users/Username/ESO python3 -m pip3 install --user -U .
  ```
* Installation inside a Python 3 venv/conda environment:
  ```shell
  cd pycpl
  CPLDIR=/Users/Username/ESO python3 -m pip install -U .
  ```
This also applies to the other environment variables which are recognized by the PyCPL build process:

- `PYCPL_BUILD_DEBUG`
- `PYCPL_BUILD_SANITIZE`
- `PYCPL_BUILD_VERBOSE`
- `PYCPL_RECIPE_DIR`

The first three are boolean variables which may be set to 0 or 1 to deactivate or activate the respective configuration. The default for all these variables is 0, i.e. inactive. The variables can be used to build PyCPL in debug mode, with support for address sanitizer, and with verbose output.

The fourth environment variable can be used to change the default, built-in search path for pipeline recipes of PyCPL. Note that this default can
always be overridden by parameters or command line options when using PyCPL or PyEsoRex at run time. The default recipe search path of PyCPL is `$CPLDIR/lib/esopipe-plugins`.

### Documentation

The PyCPL source tree also contains the source for the PyCPL Reference Manual.

#### Prerequisites

Building this manual requires the Sphinx Python library and its dependencies. These can be installed using `pip` by specifying the `[doc]` extra rquirements when installing PyCPL, e.g.
```shell
cd pycpl
python3 -m pip install -U '.[doc]'
```
#### Building the documentation

First change to the docs subdirectory of the PyCPL sources, e.g. `cd pycpl/doc`. The documentation can then be built using `make`, with different make targets corresponding to different output formats. For example:

* HTML output: `make html`
* PDF output (requires LaTeX): `make latexpdf`

To list all of the available targets/formats just run `make` without a target in the `docs` directory.

After building the documentation it will be in the `pycpl/docs/_build`

### Testing

The PyCPL source tree includes a comprehensive set of unit, validation and regression tests.

#### Prerequisites

Running the PyCPL tests requires the pytest Python library and its dependencies. These can be installed using `pip`
by specifying the `[test]` extra requirements when installing PyCPL, e.g.
```shell
cd pycpl
python3 -m pip install -U .[test]
```

#### Unit tests

To run the unit tests simply run `pytest` in the top level directory of the PyCPL source tree, e.g.
```shell
cd pycpl
python3 -m pytest -r eFsx --log-file=pycpl_unit_tests.log
```

There are many options to configure the output of pytest and select which tests are run, see the pytest documentation for details.

Three of the unit tests require the _pandas_ Python library. If this is not installed the tests will be skipped.

#### Validation tests

The validation tests run a series of CPL recipes on sample data files using both EsoRex and PyCPL and compare the results to ensure that they are identical. The recipes used are from the GIRAFFE instrument pipeline and the sample data is from the GIRAFFE pipeline regression tests so both of these must be installed in order to run the PyCPL validation tests.

* GIRAFFE pipeline: available from the [ESO Instrument Pipeline web site](https://www.eso.org/sci/software/pipelines/giraffe/)
* GIRAFFE regression test data: **not publicly available**

The GIRAFFE pipeline recipes must either be installed in the CPL recipe search path (e.g. under `$CPLDIR/lib/esopipes-plugins`) or the `$PYCPL_RECIPE_DIR` environment variable should be set to a path or paths that contain the GIRAFFE recipes.

In order to run the validation tests the environment variable `$REGTESTS` must be set to the directory containing the `sof` directory of the regression test data, for instance
```shell
export REGTESTS=$HOME/CPL/giraffe/pipedata/regression/giraffe/dfs
```

If `$REGTESTS` is set then pytest will run the validation test in addition to the unit tests when it
is run in PyCPL source tree, e.g.
```shell
cd pycpl
python3 -m pytest -r eFsx --log-file=pycpl_all.log
```

In order to run just the validation tests specify the file containing
the validation tests, e.g.
```shell
cd pycpl
python3 -m pytest -r eFsx --log-file=pycpl_validation.log tests/test_validation.py
```

**WARNING:** The validation tests do take a long time to run.

To disable the validation tests either `unset` the `$REGTESTS` environment variable or use the pytest `ignore` option, e.g.
```shell
cd pycpl
python3 -m pytest -r eFsx --ignore tests/test_validation.py --log-file=pycpl_unit_tests.log
```

#### Regression tests

Todo.

## Technical Details

Technical explainations on some design decisions and how the python bindings function during runtime.

### Link between python and C++
Before library installation, setup.py will build the code using cmake and in turn create a CPython binary module (e.g. cpl.cpython-39m-x86_64-linux-gnu.so). This module will be the target to import for use in python.

### Memory deallocation
As C++ structures are converted to bindings some questions have been raised on the ownership of memory. Generally memory allocation/deallocation is in the hands of the C++ objects. During methods all memory allocated/deallocated is controlled via the appropriate alloc/dealloc/new/delete functions. Generally allocation is called on class object constructor methods, while deallocation is called on class destructor methods. Python does however provide some automation to this in regards to the bound classes by maintaining reference counts from the python interpreter to the C++ objects: once a count reaches zero, the destructors of the object is called.

This serves as an built-in shared pointer, and in fact for some of the objects that may be contained within other classes (such as frames) are also bound as shared pointers: this ensures that both the python references and references from C++ objects are counted for determining if an object should be destroyed. For example theres a python reference to a frame, and a frameset contains that frame: the total reference count will be 2. Once the python reference is gone, the reference count is still 1 and the frame will not be destroyed as it is contained within the frameset. Only when the frameset is dereferenced, will the frameset be destroyed, and thus dereferencing the frame, and as such the frame will be destroyed.

### Using std::variant for parameters
In binding `cpl_parameter` a few issues arose regarding the creation of parameters of specific datatypes. The initial plan was to use template classes as they were thought to automatically use the datatype of the template variable during construction. However this is not the case: as templates are meant to be placeholders for type to use at compile time: and as such the compiler needs to know what to replace it with at compile time. Template classes can technically be used but are cumbersome and are ultimately pointless, as each template type for the class needs to not only be declared for binding, but also must be under a different class name. For example:
```c++
template<typename T>
void declare_array(py::module &m, std::string &typestr) {
    using Class = Array2D<T>;
    std::string pyclass_name = std::string("Array2D") + typestr;
    py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())
    .def(py::init<>())
    .def(py::init<Class::xy_t, Class::xy_t, T>())
    .def("size",      &Class::size)
    .def("width",     &Class::width)
    .def("height",    &Class::height);
}

declare_array<float>(m, "float"); // Would declare class name Array2Dfloat
declare_array<int>(m, "int");     // Would declare class name Array2Dint
```
As we would prefer the underlying data types of the parameters to be opaque, where the user just provides a value and the datatype is automatically selected, this is

Further complicating this is the need to use different cpl methods for retrieving and setting different kinds of parameters (for example `cpl_parameter_get_int` and `cpl_parameter_set_int` for integers, `cpl_parameter_get_string` and `cpl_parameter_set_string` for strings).

Hence to ensure a single method is used for getting and setting parameters, `std::variant` is used instead. This allows any specified type that is part of the variant to be returned, and the on setting values, the variant can be checked for a specific type to ensure the type passed is compatible.
