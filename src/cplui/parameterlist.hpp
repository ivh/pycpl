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


// Container class ParameterList which is similar in functionality to
// cpl_parameterlist, using vectors instead

#ifndef PYCPL_PARAMETERLIST_HPP
#define PYCPL_PARAMETERLIST_HPP

#include <memory>
#include <string>
#include <vector>

#include <cpl_parameterlist.h>

#include "cplui/parameter.hpp"

namespace cpl
{
namespace ui
{
class ParameterList
{
 public:
  typedef std::vector<value_type>::size_type size_type;
  typedef std::vector<value_type>::iterator iterator;
  typedef std::vector<value_type>::const_iterator const_iterator;

  ParameterList() {};


  ParameterList(cpl_parameterlist* p_list);
  /*
  ParameterList(vector<Parameter *> params):m_parameters(params){
    //Iteratively add parameters to interface
  }
  */
  ~ParameterList() = default;  // Default call destructor of vector, which in
                               // turn calls destructor of the parameters

  bool operator==(ParameterList& other);
  bool operator!=(ParameterList& other);

  int size();
  std::string dump() const;


  std::shared_ptr<Parameter> operator[](size_type index);
  std::shared_ptr<Parameter>
  get_first();  // Consider removing get_first and get_last?
  std::shared_ptr<Parameter> get_last();
  std::shared_ptr<Parameter> get_at(size_type index);

  void append(std::shared_ptr<Parameter> parameter);
  std::unique_ptr<struct _cpl_parameterlist_, void (*)(cpl_parameterlist*)>
  ptr() const;

 private:
  std::vector<std::shared_ptr<Parameter>> m_parameters;
};

}  // namespace ui
}  // namespace cpl

#endif  // PYCPL_PARAMETERLIST_HPP