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


// Wrapper class used to create and execute cpl_recipe
// Currently not supporting user defined recipes: only C recipes installed and
// runnable by esorex

#include "cplui/plugin.hpp"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>

#include <cpl_error.h>
#include <cpl_frame.h>
#include <cpl_frameset.h>
#include <cpl_memory.h>
#include <cpl_parameter.h>
#include <cpl_parameterlist.h>
#include <cpl_plugin.h>
#include <cpl_pluginlist.h>
#include <cpl_recipe.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace ui
{
RecipeNotFoundException::RecipeNotFoundException(
    const std::string name, const std::vector<std::string> dirs)
    : m_recipe_name(name), m_recipe_dirs(dirs)
{
}

const char*
RecipeNotFoundException::what() const throw()
{
  std::stringstream res;
  copy(m_recipe_dirs.begin(), m_recipe_dirs.end(),
       std::ostream_iterator<std::string>(res, ", "));
  std::string msg =
      "Cannot find recipe " + m_recipe_name + " in dirs " + res.str();
  char* return_str = new char[msg.length() + 1];
  strcpy(return_str, msg.c_str());
  return return_str;
}

RecipeInitException::RecipeInitException(const std::string name)
    : m_recipe_name(name) {};

const char*
RecipeInitException::what() const throw()
{
  std::string msg = "Recipe " + m_recipe_name + " failed to initialise";
  char* return_str = new char[msg.length() + 1];
  strcpy(return_str, msg.c_str());
  return return_str;
};

std::shared_ptr<FrameSet>
Recipe::run(std::shared_ptr<FrameSet> set,
            std::map<std::string, std::pair<value_type, bool>> /* settings */)
{
  // TODO: Default behaviour
  std::cout << "Base template not overridden!\n";
  return set;
};

Recipe::Recipe() { m_interface = (cpl_recipe*)calloc(1, sizeof(cpl_recipe)); };

// Initialise require fields
Recipe::Recipe(const std::string& recipe_name, const std::string& author,
               const std::string& copyright, const std::string& description,
               const std::string& email, const std::string& synopsis,
               const std::string& version)
{
  m_interface = (cpl_recipe*)calloc(1, sizeof(cpl_recipe));
  set_name(recipe_name);
  set_author(author);
  set_copyright(copyright);
  set_description(description);
  set_email(email);
  set_synopsis(synopsis);
  set_version(version);
  m_parameters = std::make_shared<ParameterList>();
}

// Initialise require fields
Recipe::Recipe(const std::string& recipe_name, const std::string& author,
               const std::string& copyright, const std::string& description,
               const std::string& email, const std::string& synopsis,
               int version)
{
  m_interface = (cpl_recipe*)calloc(1, sizeof(cpl_recipe));
  set_name(recipe_name);
  set_author(author);
  set_copyright(copyright);
  set_description(description);
  set_email(email);
  set_synopsis(synopsis);
  set_version(version);
  m_parameters = std::make_shared<ParameterList>();
}

// Any Python Class that inherits Recipe will call this destructor as well
Recipe::~Recipe()
{
  cpl_plugin_delete((cpl_plugin*)m_interface);  // Delete plugin
}

std::shared_ptr<ParameterList>
Recipe::get_parameters()
{
  return m_parameters;
}

std::string
Recipe::get_name()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_name, (cpl_plugin*)m_interface));
}

void
Recipe::set_name(const std::string& name)
{
  cpl::core::Error::throw_errors_with(cpl_plugin_set_name,
                                      (cpl_plugin*)m_interface, name.c_str());
}

std::string
Recipe::get_author()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_author, (cpl_plugin*)m_interface));
}

void
Recipe::set_author(const std::string& author)
{
  cpl::core::Error::throw_errors_with(cpl_plugin_set_author,
                                      (cpl_plugin*)m_interface, author.c_str());
}

std::string
Recipe::get_copyright()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_copyright, (cpl_plugin*)m_interface));
}

void
Recipe::set_copyright(const std::string& copyright)
{
  cpl::core::Error::throw_errors_with(
      cpl_plugin_set_copyright, (cpl_plugin*)m_interface, copyright.c_str());
}

std::string
Recipe::get_description()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_description, (cpl_plugin*)m_interface));
}

void
Recipe::set_description(const std::string& description)
{
  cpl::core::Error::throw_errors_with(cpl_plugin_set_description,
                                      (cpl_plugin*)m_interface,
                                      description.c_str());
}

std::string
Recipe::get_email()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_name, (cpl_plugin*)m_interface));
}

void
Recipe::set_email(const std::string& email)
{
  cpl::core::Error::throw_errors_with(cpl_plugin_set_email,
                                      (cpl_plugin*)m_interface, email.c_str());
}

std::string
Recipe::get_synopsis()
{
  return std::string(cpl::core::Error::throw_errors_with(
      cpl_plugin_get_synopsis, (cpl_plugin*)m_interface));
}

void
Recipe::set_synopsis(const std::string& synopsis)
{
  cpl::core::Error::throw_errors_with(
      cpl_plugin_set_synopsis, (cpl_plugin*)m_interface, synopsis.c_str());
}

std::string
Recipe::get_version()
{
  char* version_str = cpl::core::Error::throw_errors_with(
      cpl_plugin_get_version_string, (cpl_plugin*)m_interface);
  // Pass buffer into new std::string to create copy to return
  std::string cppString = std::string(version_str);
  // Deallocated buffer using cpl_free, as required by CPL documentation
  cpl_free(version_str);
  return cppString;
}

void
Recipe::set_version(int version)
{
  cpl::core::Error::throw_errors_with(cpl_plugin_set_version,
                                      (cpl_plugin*)m_interface, version);
}

void
Recipe::set_version(const std::string& version)
{
  int numRep = 0;     // Integer representation of version
  int repOffset = 2;  // Start with the major portion of the version
  std::string::size_type pos = 0;  // Position of "."
  std::string copy(version);

  int token;
  // Need to pass version with integer representation xx.xx.xx
  while ((pos = copy.find(".")) != std::string::npos || repOffset >= 0) {
    token = std::stoi(copy.substr(0, pos));
    numRep += (token % 100) * std::pow(100, repOffset--);  // Go to next numeral
    // Cut off the part we were using to calculate version
    copy.erase(0, pos + 1);
  }
  cpl::core::Error::throw_errors_with(cpl_plugin_set_version,
                                      (cpl_plugin*)m_interface, numRep);
}

CRecipe::CRecipe(std::string recipe_name) : Recipe()
{
  std::map<std::string, std::string>::iterator it =
      library_locations.find(recipe_name);
  if (it != library_locations.end()) {
    cpl_pluginlist* so_interface = cpl_pluginlist_new();
    dl_handle =
        dlopen(it->second.c_str(),
               RTLD_LAZY | RTLD_LOCAL);  // Open library with local symbols
    int (*getPlugin)(cpl_pluginlist*);
    getPlugin = (int (*)(cpl_pluginlist*))dlsym(
        dl_handle,
        "cpl_plugin_get_info");  // Extract initialisation function from lib
    getPlugin(so_interface);     // Load the plugin into the plugin list
    // long n = sizeof (cpl_recipe);
    // m_interface = (cpl_recipe *) calloc (1,(size_t) n);
    cpl_plugin_copy(
        (cpl_plugin*)&m_interface->interface,
        cpl_pluginlist_get_last(so_interface));  // Allocate and move the recipe
                                                 // in without initialisation
  } else {
    throw(RecipeNotFoundException(recipe_name, recipe_dir));
  }
}

CRecipe::~CRecipe()
{
  // Recipe struct dealloced in ~Recipe
  dlclose(dl_handle);  // Close the handler
};

std::shared_ptr<FrameSet>
CRecipe::run(std::shared_ptr<FrameSet> frameset,
             std::map<std::string, std::pair<value_type, bool>> settings)
{
  // Update the CPL recipe data structure with the input frame set. They
  // must be available when the exec method of the CPL recipe instance
  // is called.
  m_interface->frames = frameset->to_cpl();

  cpl_plugin_func init = cpl_plugin_get_init((cpl_plugin*)m_interface);
  if (init == NULL) {
    throw RecipeInitException(get_name());
  }

  // Run the plugin (recipe) initialisation. Throw an exception if the
  // initialisation does not succeed.
  int code = init((cpl_plugin*)m_interface);
  if (code != 0) {
    throw RecipeInitException(get_name());
  }

  // Unfortunately can't use our own parameterlists as those will try to destroy
  // the parameters: something the recipe deinit tends to do as well. Have to
  // stick with cpl_parameterlist for this Set to defaults
  for (cpl_parameter* p = cpl_parameterlist_get_first(m_interface->parameters);
       p != NULL; p = cpl_parameterlist_get_next(m_interface->parameters)) {
    cpl_type ptype = cpl_parameter_get_type(p);
    // Need to set default first: as its not expected by the recipe
    switch (ptype) {
      case CPL_TYPE_BOOL:
        cpl_parameter_set_bool(p, cpl_parameter_get_default_bool(p));
        break;

      case CPL_TYPE_INT:
        cpl_parameter_set_int(p, cpl_parameter_get_default_int(p));
        break;

      case CPL_TYPE_DOUBLE:
        cpl_parameter_set_double(p, cpl_parameter_get_default_double(p));
        break;

      case CPL_TYPE_STRING:
        cpl_parameter_set_string(p, cpl_parameter_get_default_string(p));
        break;
      default: break;
    }
    cpl_parameter_set_default_flag(p, 0);
  }

  // Lookup the parameter names passed in the settings argument in the parameter
  // list of the CPL recipe data structure and update their configuration there.
  for (auto& [name, config] : settings) {
    cpl_parameter* parameter =
        cpl_parameterlist_find(m_interface->parameters, name.c_str());
    if (parameter != NULL) {
      cpl_type ptype = cpl_parameter_get_type(parameter);
      switch (ptype) {
        case CPL_TYPE_BOOL:
          cpl_parameter_set_bool(parameter, std::get<bool>(config.first));
          break;
        case CPL_TYPE_INT:
          // Possibly lossy conversion from double to int. Warn user?
          if (auto val = std::get_if<double>(&config.first)) {
            cpl_parameter_set_int(parameter, *val);
          }
          if (auto val = std::get_if<int>(&config.first)) {
            cpl_parameter_set_int(parameter, *val);
          }
          break;
        case CPL_TYPE_DOUBLE:
          if (auto val = std::get_if<double>(&config.first)) {
            cpl_parameter_set_double(parameter, *val);
          }
          if (auto val = std::get_if<int>(&config.first)) {
            cpl_parameter_set_double(parameter, *val);
          }
          break;
        case CPL_TYPE_STRING:
          cpl_parameter_set_string(parameter,
                                   std::get<std::string>(config.first).c_str());
          break;
        default: break;
      }
      cpl_parameter_set_default_flag(parameter,
                                     (config.second == true) ? 1 : 0);
    }
  }

  cpl_plugin_func exec = cpl_plugin_get_exec((cpl_plugin*)m_interface);

  try {
    // Throws if any recipe errors occurred
    int status =
        cpl::core::Error::throw_errors_with(exec, (cpl_plugin*)m_interface);
    if (status != 0) {
      std::ostringstream msg;
      msg << "Recipe '" << get_name() << "' failed with code '" << status
          << "'. Error details are not available!";
      cpl::core::Error* error = cpl::core::Error::make_error(
          CPL_ERROR_UNSPECIFIED, PYCPL_ERROR_LOCATION, msg.str());
      throw error;
    }

    // Find products in recipe frameset, and move them to the output frameset
    std::shared_ptr<FrameSet> products(new FrameSet());
    FrameSet output_frames(m_interface->frames);

    for (auto frame : output_frames) {
      // In the cpl_plugin_get_deinit line of code below this,
      // all framesets from m_interface->frames will be deallocated.
      // However, these frames are still needed to be returned, so
      // they are duplicated here, then the originals are 'leaked'
      // (but plugin deinit will free them)
      if (frame->get_group() == CPL_FRAME_GROUP_PRODUCT) {
        products->append(frame->duplicate());
      }
      frame->leak();
    }

    cpl_plugin_get_deinit((cpl_plugin*)m_interface)(
        (cpl_plugin*)m_interface);  // Deinit the recipe
    cpl_frameset_delete(
        m_interface->frames);  // Delete the copied frames into the recipe

    return products;
  }
  catch (...) {
    cpl_plugin_get_deinit((cpl_plugin*)m_interface)(
        (cpl_plugin*)m_interface);  // Deinit the recipe
    cpl_frameset_delete(
        m_interface->frames);  // Delete the copied frames into the recipe
    throw;
  }
}

// Return a copy of the parameters with their defaults set in C++ object form
std::shared_ptr<ParameterList>
CRecipe::get_parameters()
{
  cpl_plugin_func init = cpl_plugin_get_init(
      (cpl_plugin*)m_interface);  // Init to get recipe to load parameters
  if (init == NULL) {
    throw(RecipeInitException(get_name()));
  }
  int code = init((cpl_plugin*)m_interface);
  if (code != 0) {  // If init has any issues
    throw(RecipeInitException(get_name()));
  }
  cpl_parameterlist* duplicate = cpl_parameterlist_new();
  for (cpl_parameter* p = cpl_parameterlist_get_first(m_interface->parameters);
       p != NULL; p = cpl_parameterlist_get_next(m_interface->parameters)) {
    cpl_parameterlist_append(duplicate, cpl_parameter_duplicate(p));
  }
  cpl_plugin_get_deinit((cpl_plugin*)m_interface)((cpl_plugin*)m_interface);
  return std::make_shared<ParameterList>(duplicate);
}

void
CRecipe::set_recipe_dir(const std::vector<std::string>& dir_list)
{
  library_locations.clear();  // Clear current map of old directory
  cpl_pluginlist* pluginlist = cpl_pluginlist_new();
  for (const std::string& dir : dir_list) {
    // Go through all files in dir including subdirectories
    for (auto file : std::filesystem::recursive_directory_iterator(
             dir,
             std::filesystem::directory_options::follow_directory_symlink)) {
      std::string path = file.path();
      if (!path.compare(path.size() - 3, 3,
                        ".so")) {  // Recipes must be in .so format
        void* handle = dlopen(std::string(file.path()).c_str(), RTLD_LAZY);
        if (!handle) {  // Error with opening library, then its not usable
          continue;
        }
        int (*getPlugin)(cpl_pluginlist*);
        getPlugin =
            (int (*)(cpl_pluginlist*))dlsym(handle, "cpl_plugin_get_info");
        if (getPlugin != NULL) {  // If not null, lib contained the symbol and
                                  // thus is a valid plugin
          getPlugin(pluginlist);
          std::string recipe_name = std::string(
              cpl_plugin_get_name(cpl_pluginlist_get_last(pluginlist)));
          library_locations.insert(
              std::make_pair(recipe_name, file.path()));  // Add to location map
        }
        dlclose(handle);
      }
    }
  }
  cpl_pluginlist_delete(
      pluginlist);  // Delete the pluginlist: we just wanted the available
                    // recipes and their location.
  recipe_dir = dir_list;
}

std::vector<std::string>
CRecipe::get_recipe_dir()
{
  return recipe_dir;
}

// Loop through the map of recipe locations and just return the keys
std::vector<std::string>
CRecipe::list()
{
  std::vector<std::string> nameList;
  for (auto const& entry : library_locations) {
    nameList.push_back(entry.first);
  }
  return nameList;
}

std::map<std::string, std::string> CRecipe::library_locations =
    std::map<std::string, std::string>();


std::vector<std::string> CRecipe::recipe_dir = std::vector<std::string>();


}  // namespace ui
}  // namespace cpl
