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

#ifndef PYCPL_PLUGIN_HPP
#define PYCPL_PLUGIN_HPP

#include <dlfcn.h>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cpl_recipe.h>

#include "cplui/frameset.hpp"
#include "cplui/parameter.hpp"
#include "cplui/parameterlist.hpp"

namespace cpl
{
namespace ui
{
class RecipeNotFoundException : public std::exception
{
  std::string m_recipe_name;
  std::vector<std::string> m_recipe_dirs;

 public:
  RecipeNotFoundException(const std::string name,
                          const std::vector<std::string> dirs);
  virtual const char* what() const throw();
};

class RecipeInitException : public std::exception
{
  std::string m_recipe_name;

 public:
  RecipeInitException(const std::string name);
  virtual const char* what() const throw();
};

/**
 * @brief Class interface for constructing both Python  Recipes
 *
 * Recipe is used as a base class that is inherited to create own recipes.
 *
 * cpl_recipe_init and cpl_recipe_deinit are not present as their purpose
 * is achieved with basic C++ class constructors/destructors
 *
 * Is to be inherited by the CRecipes class
 */
class Recipe
{
 public:
  // Empty constructor for CRecipe inheritence, is not bound
  Recipe();
  // Default constructors intialise fields. Required when inheriting
  // cpl.ui.Recipe
  Recipe(const std::string& recipe_name, const std::string& author,
         const std::string& copyright, const std::string& description,
         const std::string& email, const std::string& synopsis,
         const std::string& version);
  Recipe(const std::string& recipe_name, const std::string& author,
         const std::string& copyright, const std::string& description,
         const std::string& email, const std::string& synopsis, int version);
  virtual ~Recipe();

  virtual std::shared_ptr<FrameSet>
  run(std::shared_ptr<FrameSet> frameset,
      std::map<std::string, std::pair<cpl::ui::value_type, bool>> settings);
  std::string get_name();
  void set_name(const std::string& name);
  std::string get_author();
  void set_author(const std::string& author);
  std::string get_copyright();
  void set_copyright(const std::string& copyright);
  std::string get_description();
  void set_description(const std::string& description);
  std::string get_email();
  void set_email(const std::string& email);
  std::string get_synopsis();
  void set_synopsis(const std::string& synopsis);
  std::string get_version();
  void set_version(int version);
  void set_version(const std::string& version);
  virtual std::shared_ptr<ParameterList> get_parameters();

 protected:
  static std::vector<std::string> recipe_dir;
  static std::map<std::string, std::string> library_locations;
  cpl_recipe* m_interface;
  std::shared_ptr<ParameterList> m_parameters;
  std::shared_ptr<FrameSet> m_frames = std::make_shared<FrameSet>();
};

/**
 * @brief Interface for initialising Recipes written and compiled in C
 *
 * Includes various functions for loading and executing compiled recipes by
 * name, with a given directory
 *
 * Achieved using cpl plugin interfaces
 */
class CRecipe : public Recipe
{
 public:
  CRecipe(std::string recipe_name);
  ~CRecipe();

  std::shared_ptr<FrameSet>
  run(std::shared_ptr<FrameSet> frameset,
      std::map<std::string, std::pair<cpl::ui::value_type, bool>> settings)
      override;
  static std::vector<std::string> get_recipe_dir();
  static std::vector<std::string> list();
  std::shared_ptr<ParameterList> get_parameters() override;
  static void set_recipe_dir(const std::vector<std::string>& dir_list);

 private:
  static std::vector<std::string> recipe_dir;
  static std::map<std::string, std::string> library_locations;
  void* dl_handle;
  // std::shared_ptr<ParameterList>
  // m_parameters=std::make_shared<ParameterList>();
  std::shared_ptr<FrameSet> m_frames = std::make_shared<FrameSet>();
};

}  // namespace ui
}  // namespace cpl

#endif  // PYCPL_PLUGIN_HPP