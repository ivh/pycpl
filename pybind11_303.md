# pybind11 3.0.3 Build Failure

## Summary

pybind11 3.0.3 (via [pybind/pybind11#5992](https://github.com/pybind/pybind11/pull/5992))
introduces stricter `static_assert` checks on member function pointers, rejecting
pointers that don't belong to the class being bound. This breaks PyCPL and PyHDRL
in two places.

## Issue 1: `CRecipe` using `Recipe::` member pointers (PyCPL)

In `src/cplui/plugin_bindings.cpp`, `CRecipe` is bound without registering its
parent class `Recipe` with pybind11. All property bindings use `Recipe::` member
pointers directly:

```cpp
py::class_<cpl::ui::CRecipe, std::shared_ptr<cpl::ui::CRecipe>> crecipe(m, "CRecipe");
// ...
.def_property("author", &cpl::ui::Recipe::get_author, &cpl::ui::Recipe::set_author, ...)
.def_property("copyright", &cpl::ui::Recipe::get_copyright, ...)
// ... same for description, email, synopsis, version, name
```

**Fix:** wrap in lambdas that call through the derived class:

```cpp
.def_property("author",
    [](cpl::ui::CRecipe& self) { return self.get_author(); },
    [](cpl::ui::CRecipe& self, const std::string& v) { self.set_author(v); }, ...)
```

Alternatively, register `Recipe` as a pybind11 base class of `CRecipe`.

## Issue 2: `Error` using `ErrorFrame::operator==` (PyCPL + PyHDRL)

In both `src/cplcore/error_bindings.cpp` and `src/hdrlcore/error_bindings.cpp`,
the `__eq__` binding for `Error` uses `ErrorFrame::operator==`:

```cpp
// binding for Error class:
.def("__eq__", &cpl::core::ErrorFrame::operator==)   // wrong: ErrorFrame is not a base of Error
```

This is a bug independent of pybind11 version: `Error` does not inherit from
`ErrorFrame`, so this was never calling the right function. It only compiled
because older pybind11 didn't check member pointer origin.

Both `cpl::core::Error` and `hdrl::core::Error` have their own `operator==`:

```cpp
// error.hpp:188
bool operator==(Error& other) const noexcept;
```

**Fix:** use the correct class's operator:

```cpp
.def("__eq__", &cpl::core::Error::operator==)    // in src/cplcore/error_bindings.cpp
.def("__eq__", &hdrl::core::Error::operator==)    // in src/hdrlcore/error_bindings.cpp
```

## Files requiring changes

| File | Change |
|------|--------|
| `src/cplui/plugin_bindings.cpp` | Wrap 7 `Recipe::` property bindings in lambdas |
| `src/cplcore/error_bindings.cpp` | `ErrorFrame::operator==` -> `Error::operator==` |
| `src/hdrlcore/error_bindings.cpp` | `ErrorFrame::operator==` -> `Error::operator==` |
