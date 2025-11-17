// This file is part of PyCPL the ESO CPL Python language bindings
// Copyright (C) 2020-2024 European Southern Observatory
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "cplui/plugin_bindings.hpp"

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cpl_error.h>
#include <pybind11/eval.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "cplcore/error.hpp"
#include "cplcore/errorframe.hpp"
#include "cplui/frameset.hpp"
#include "cplui/parameter.hpp"
#include "cplui/plugin.hpp"

// Binding method for plugins/recipes: classes contained within plugin.hpp and
// plugin.hpp, to be called by the main bindings file (bindings.cpp)
void
bind_plugin(py::module& m)
{
  py::register_exception<cpl::ui::RecipeNotFoundException>(
      m, "RecipeNotFoundException");

  try {
    // The default recipe dir is expected to be provided through the
    // preprocessor symbol PYCPL_RECIPE_DIR, configured in the build system
    std::vector<std::string> default_dir = {std::string(PYCPL_RECIPE_DIR)};
    cpl::ui::CRecipe::set_recipe_dir(default_dir);
  }
  catch (const std::filesystem::filesystem_error& bad_recipe_dir) {
    py::module::import("warnings")
        .attr("warn")(
            std::string("An error occurred trying to read the esopipes-plugin "
                        "directory '" PYCPL_RECIPE_DIR "': ") +
            bad_recipe_dir.what());
  }

  py::class_<cpl::ui::CRecipe, std::shared_ptr<cpl::ui::CRecipe>> crecipe(
      m, "CRecipe");

  crecipe.doc() = R"pydoc(
      Interface for initialising and executing compiled CPL recipes written in C. The default recipe dir is set
      on installation as the esopipes-plugin directory in the configured CPLDIR/lib directory. 

      Modifying any parameters stored in the the recipe's `parameters` property will not have any effect on execution. 
      Any settings from the default parameters must be passed to the settings parameter in the run method.
    )pydoc";

  crecipe
      .def(py::init<std::string>(), py::arg("name"),
           "Initialise the recipe with a given name.")
      .def_property_readonly_static(
          "recipes",
          [](py::object /* self */) -> std::vector<std::string> {
            return cpl::ui::CRecipe::list();
          },
          "List of recipes found in dirs defined by the `recipe_dir` property")
      .def_property_readonly("parameters", &cpl::ui::CRecipe::get_parameters,
                             "A list of recipe parameters and their defaults. "
                             "This is a copy, not a reference.")
      .def_property("author", &cpl::ui::Recipe::get_author,
                    &cpl::ui::Recipe::set_author, "Name of the recipe's author")
      .def_property("copyright", &cpl::ui::Recipe::get_copyright,
                    &cpl::ui::Recipe::set_copyright,
                    "Recipe's license and copyright information. Must be "
                    "compatible with that of CPL.")
      .def_property("description", &cpl::ui::Recipe::get_description,
                    &cpl::ui::Recipe::set_description)
      .def_property("email", &cpl::ui::Recipe::get_email,
                    &cpl::ui::Recipe::set_email, "Author's contact information")
      .def_property("synopsis", &cpl::ui::Recipe::get_synopsis,
                    &cpl::ui::Recipe::set_synopsis,
                    "Detailed description of a plugin.")
      .def_property(
          "version", &cpl::ui::Recipe::get_version,
          py::overload_cast<const std::string&>(&cpl::ui::Recipe::set_version),
          "Recipe version number")
      .def_property("name", &cpl::ui::Recipe::get_name,
                    &cpl::ui::Recipe::set_name, "Unique name of the Recipe")
      .def_property_static(
          "recipe_dir",
          [](py::object /* self */) -> std::vector<std::string> {
            return cpl::ui::CRecipe::get_recipe_dir();
          },
          [](py::object /* self */, py::object setting) -> void {
            std::vector<std::string> dirs;
            try {
              dirs = setting.cast<std::vector<std::string>>();
            }
            catch (const py::cast_error& /* unused */) {
              try {
                dirs = {setting.cast<std::string>()};
              }
              catch (const py::cast_error& /* unused */) {
                throw cpl::core::IllegalInputError(
                    PYCPL_ERROR_LOCATION,
                    "Directory must either be a string or a list of strings");
              }
            }
            cpl::ui::CRecipe::set_recipe_dir(dirs);
          },
          "Path where recipes are installed. Is defaulted to "
          "$CPL_ROOT/lib/esopipes_pipelines")
      // A more private run alternative to run recipe in the same process
      // This allows multiple Pyesorex instances to be launched from a
      // multiprocessing pool to give more flexibility in the usage of
      // PyCPL/Pyesorex. See PIPE-11208 for more details.
      .def(
          "_run_in_process",
          [](cpl::ui::CRecipe& r,
             std::shared_ptr<cpl::ui::FrameSet> input_frames,
             std::map<std::string, cpl::ui::value_type> settings)
              -> std::shared_ptr<cpl::ui::FrameSet> {
            // It is assumed that the provided settings are only the settings
            // which were set explicitly, i.e. settings is the subset of
            // modified recipe parameters. This is made explicit here for
            // passing this information downstream.
            std::map<std::string, std::pair<cpl::ui::value_type, bool>>
                _settings;
            for (auto& [key, value] : settings) {
              _settings.emplace(
                  std::make_pair(key, std::make_pair(value, true)));
            }

            std::shared_ptr<cpl::ui::FrameSet> result =
                std::make_shared<cpl::ui::FrameSet>();
            try {
              result = r.run(input_frames, _settings);
            }
            catch (const cpl::core::Error& e) {
              // An exception could be raised here if desired.
              std::cout << "ERROR " << std::endl;
            }
            return result;
          },
          py::arg("input_frames"),
          py::arg("settings") = std::map<std::string, cpl::ui::value_type>(),
          R"pydoc(
      Execute a recipe within the current process.
      Allows for multiple instances of Pyesorex to be launched
      from a Python multiprocessing pool (see Example).

      Intended for expert developers, rather than the casual user.
      Error handling is less robust than the standard run method.

      As the parameters property is just a copy, custom parameter settings should be set in this method call.

      Parameters
      ----------
      input_frames : cpl.ui.FrameSet
        Input FrameSet for the recipe to use as inputs
      settings : dict
        {parameter_name : value} pairs for configuring recipe parameters for execution.
      
      Returns
      -------
      cpl.ui.FrameSet
        A FrameSet of all frames with cpl.ui.Frame.FrameGroup.PRODUCT, indicating them as the recipe's output

      Raises
      ------
      RuntimeError
        if a recipe error occurs.

      Notes
      -----
      All files generated during execution are saved in ./products/(recipe_name)_(folder_no) relative to the current working directory.
      After running, these files will not be deleted by PyCPL if not deleted by the recipe binaries themselves and remain on disk for
      the user to manage themselves for their own uses.
      
      Example
      -------
      .. code-block:: python

        def run_me(self,path,recipe,param1,param2):
           # change cwd to path
           os.chdir(path)
           # wait for all workers to have switched over to their folders...
           time.sleep(5)
           p = Pyesorex()
           p.recipe = recipe
           
           # setup Pyesorex parameters
           
           # run Pyesorex
           results = p.recipe._run_in_process(sof,p.recipe_parameters.as_dict())
       
           # make sure to return the path to correct the results FrameSet
           return (results, path)
           
        # When collecting the results from the pool:
        # create an empty frameset to store all the results from the pool
        total_results = FrameSet()
        
        # run your pool...
        
        # handle the results
        fset, path = result
        
        # Since fset does not have absolute paths, make sure to set absolute paths for filenames
        # otherwise the returned FrameSet will be invalid and will raise an Exception
        for fr in fset:
            total_results.append(Frame(str(path / fr.file),fr.tag))
    )pydoc")
      /*
      Running of plugins/recipes:

      There is a distinct possibility that a recipe may crash,
      taking the PyCPL C++ library with it, and possibly the Python thread.

      To avoid a recipe  having such dramatic effects, These python bindings
      run the recipes in a separate Python interpreter thread.

      The privately named _execute_in_process is the actual 'binding' of
      recipe.run(), however it also has some modifications:
      a directory (named <recipe_name>_<number>,  not overwriting existing
      directories) is created, and products & intermediate FITS files from the
      recipe end up in said directory.
      */
      .def(
          "run",
          [](cpl::ui::CRecipe& r,
             std::shared_ptr<cpl::ui::FrameSet> input_frames,
             std::map<std::string, cpl::ui::value_type> settings)
              -> std::shared_ptr<cpl::ui::FrameSet> {
            // It is assumed that the provided settings are only the settings
            // which were set explicitly, i.e. settings is the subset of
            // modified recipe parameters. This is made explicit here for
            // passing this information downstream.
            std::map<std::string, std::pair<cpl::ui::value_type, bool>>
                _settings;
            for (auto& [key, value] : settings) {
              _settings.emplace(
                  std::make_pair(key, std::make_pair(value, true)));
            }

            py::object multiprocessing = py::module::import("multiprocessing");
            // Get queue to retrieve frames from after exection
            py::object queue = multiprocessing.attr("Queue")();
            // Cast to python object to make it a target of a new process
            py::object pyInstance = py::cast(r);
            // py::list input;
            // for(int i=0; i<input_frames->size();i++){
            //   input.attr("append")(input_frames->get_at(i));
            // }
            py::object instance = multiprocessing.attr("Process")(
                py::arg("target") = pyInstance.attr("_execute_in_process"),
                py::arg("args") =
                    py::make_tuple(input_frames, _settings, queue));

            instance.attr("start")();  // Execute recipe in Process object
            instance.attr("join")();   // Wait for Recipe to complete

            // Get frames in queue from subprocess
            std::shared_ptr<cpl::ui::FrameSet> result =
                std::make_shared<cpl::ui::FrameSet>();
            while (!queue.attr("empty")().cast<bool>()) {
              py::object queue_elem = queue.attr("get")();
              try {
                // Throw an error if exited abnormally
                auto frame_ptr =
                    queue_elem.cast<std::shared_ptr<cpl::ui::Frame>>();
                result->append(frame_ptr);
              }
              catch (const py::cast_error& /*exception */) {
                // Decode the serialized error information passed through
                // the queue by the child process
                py::tuple errdata = queue_elem.cast<py::tuple>();
                cpl::core::Error* error = cpl::core::Error::make_error(
                    static_cast<cpl_error_code>(errdata[0].cast<uint64_t>()),
                    errdata[1].cast<std::string>(),
                    errdata[2].cast<std::string>(),
                    errdata[3].cast<unsigned int>(),
                    errdata[4].cast<std::string>());
                throw error;
              }
            }
            return result;
          },
          py::arg("input_frames"),
          py::arg("settings") = std::map<std::string, cpl::ui::value_type>(),
          R"pydoc(
      Execute a recipe. As the parameters property is just a copy, custom parameter settings should be set in this method call.  

      Parameters
      ----------
      input_frames : cpl.ui.FrameSet
        Input FrameSet for the recipe to use as inputs
      settings : dict
        {parameter_name : value} pairs for configuring recipe parameters for execution. 
      
      Returns
      -------
      cpl.ui.FrameSet
        A FrameSet of all frames with cpl.ui.Frame.FrameGroup.PRODUCT, indicating them as the recipe's output

      Raises
      ------
      RuntimeError
        if a recipe error occurs.

      Notes
      -----
      All files generated during execution are saved in ./products/(recipe_name)_(folder_no) relative to the current working directory.
      After running, these files will not be deleted by PyCPL if not deleted by the recipe binaries themselves and remain on disk for 
      the user to manage themselves for their own uses.
    )pydoc")
      .def("_execute_in_process",
           [](cpl::ui::CRecipe& r,
              std::shared_ptr<cpl::ui::FrameSet> inputFrames,
              std::map<std::string, std::pair<cpl::ui::value_type, bool>>
                  settings,
              py::object& q) -> void {
#if 0
             // Private method: only mean't to be called from
             // cpl.ui.Recipe.run()
             std::shared_ptr<cpl::ui::FrameSet> input_FrameSet =
                 std::make_shared<cpl::ui::FrameSet>();
             // FIXME: When FrameSet to supports pickling, replace the whole
             //        "cast from list to frameset" process and just pass the
             //        frameset
             for (int i = 0; i < inputFrames.attr("__len__")().cast<int>();
                  i++) {
               input_FrameSet->append(
                   inputFrames[i].cast<std::shared_ptr<cpl::ui::Frame>>());
             }
#endif
             try {
               std::shared_ptr<cpl::ui::FrameSet> output =
                   r.run(inputFrames, settings);

               // Put output frames from run into the queue
               for (int i = 0; i < output->size(); i++) {
                 std::shared_ptr<cpl::ui::Frame> outputFrame =
                     output->get_at(i);
                 q.attr("put")(outputFrame);
               }
             }
             catch (const cpl::core::Error* e) {
               // Serialize the most recent error by putting its
               // components in a py::tuple, which is then passed
               // to the parent process.
               cpl::core::ErrorFrame err(e->last());
               py::tuple errdata =
                   py::make_tuple(static_cast<uint64_t>(err.get_code()),
                                  err.get_function_name(), err.get_file_name(),
                                  err.get_line(), err.get_error_message());
               q.attr("put")(errdata);
             }
           })
      .def("__repr__",
           [](cpl::ui::CRecipe& a) -> py::str {
             // There is no simple, general method to convert arbitrary data
             // types to a string in C++17, however this is straightforward to
             // do using Python's string formatting functionality. To take
             // advantage of this flexibility we implement __repr__() and
             // __str__() methods by creating a Python string object and use its
             // format() method to insert string representations of the required
             // attributes.
             py::str representation = "<cpl.ui.Recipe {}>";
             return representation.format(a.get_name());
           })
      .def(py::pickle(
          [](cpl::ui::CRecipe& r) -> py::tuple {
            return py::make_tuple(r.get_name(), r.get_recipe_dir());
          },
          [](py::tuple t) -> std::shared_ptr<cpl::ui::CRecipe> {
            cpl::ui::CRecipe::set_recipe_dir(
                t[1].cast<std::vector<std::string>>());
            std::shared_ptr<cpl::ui::CRecipe> r =
                std::make_shared<cpl::ui::CRecipe>(t[0].cast<std::string>());
            return r;
          }));

  py::exec(
      R"(
            import abc
            class AbstractRecipe(abc.ABC):
                '''
                Abstract Base Class to be used by PyRecipe
                '''
                _name=None
                _author=None
                _copyright=None
                _description=None
                _email=None
                _synopsis=None
                _version=None

                def __new__(cls):
                    if cls._name is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _name not set.")
                    if cls._author is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _author not set.")
                    if cls._copyright is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _copyright not set.")
                    if cls._description is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _description not set.")
                    if cls._email is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _email not set.")
                    if cls._synopsis is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _synopsis not set.")
                    if cls._version is None:
                        raise TypeError(f"Can't instantiate class {cls.__name__} with class variable _version not set.")
                    return super().__new__(cls)

                @property
                def name(self):
                    return self._name

                @property
                def author(self):
                    return self._author

                @property
                def copyright(self):
                    return self._copyright

                @property
                def description(self):
                    return self._description

                @property
                def email(self):
                  return self._email

                @property
                def synopsis(self):
                    return self._synopsis

                @property
                def version(self):
                    return self._version

                @abc.abstractmethod
                def run(frameset, settings):
                    pass

                def __repr__(self):
                    return "<cpl.ui.Recipe {}>".format(self.name)

            class PyRecipe(AbstractRecipe, metaclass=abc.ABCMeta):
                '''
                PyRecipe base class for the implementation of custom Python recipes. 

                When inheriting this class the following members are expected to be overwitten:

                - _name
                - _author
                - _copyright
                - _description
                - _email
                - _synopsis
                - _version
                - run(frameset,settings)

                It is also recommended that new recipes include their own docstrings. New __init__ and __del__ methods 
                can be written to handle data before/after execution.
                '''
                pass

            AbstractRecipe.register(CRecipe)
      )",
      m.attr("__dict__"));
}
