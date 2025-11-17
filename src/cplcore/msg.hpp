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

#ifndef PYCPL_MSG_HPP
#define PYCPL_MSG_HPP

#include <filesystem>
#include <string>

#include <cpl_msg.h>

namespace cpl
{
namespace core
{
// FIXME: Not really necessary to maintain a class: just have a set of
// functions that interface with cpl msg and maintain variables for
// tracking
class Msg
{
 public:
  /**
   * @brief Get the log file name.
   * To set the log name, which can only occur once,
   * use start_log
   *
   * The name of the log file is returned.
   *
   * @return Logfile name
   */
  static std::filesystem::path get_log_name();

  /**
   * @brief Set verbosity level of output to terminal.
   *
   * The @em verbosity specifies the lowest severity level that a message
   * should have for being displayed to terminal. If this function is not
   * called, the verbosity level defaults to @c CPL_MSG_INFO.
   *
   */

  static void set_level(int verbosity);

  /**
   * @brief Get current terminal verbosity level.
   *
   * Get current verbosity level set for the output to terminal.
   *
   * @return Current verbosity level.
   */
  static cpl_msg_severity get_level();

  /**
   * @brief Start log file with a given verbosity
   *
   * Starts a file with name in the current working directory with output
   *
   */
  static void
  start_log(cpl_msg_severity verbosity, const std::filesystem::path& name);

  /**
   * @brief
   *   Close the current log file.
   *
   * An attempt to close a non existing log file would not generate an error
   * condition. This routine may be called in case the logging should be
   * terminated before the end of a program. Otherwise, the function
   * @c cpl_msg_stop() would automatically close the log file when called
   * at the end of the program.
   */
  static void stop_log();

  /**
   * @brief
   *   Get the @em domain name.
   *
   * @return Value of "domain" string.
   *
   */
  static std::string get_domain();

  /**
   * @brief
   *   Set the @em domain name.
   *
   * @param name Any task identifier, typically a pipeline recipe name.
   *
   * @return Nothing.
   *
   * This routine should be called at a pipeline recipe start, and
   * before a possible call to the function cpl_msg_set_log_level() or the
   * proper task identifier would not appear in the log file header.
   * The @em domain tag is attached to messages sent to terminal only
   * if the function @c cpl_msg_set_domain_on() is called. If the
   * @em domain tag is on and no domain tag was specified, the string
   * "Undefined domain" (or something analogous) would be attached
   * to all messages. To turn the @em domain tag off the function
   * @c cpl_msg_set_domain_off() should be called. If @em name is a
   * @c NULL pointer, nothing is done, and no error is set.
   *
   * @note
   *   This function is meant to configure once and for all the behaviour
   *   and the "style" of the messaging system, and it is not supposed to
   *   be called in threads.
   */
  static void set_domain(const std::string& name);

  /**
   * @brief
   *   Display a debug message.
   *
   * @return Nothing.
   *
   * @param Name       of the function generating the message.
   * @param message    Message string.
   *
   * See the description of the function @c error().
   */
  static void debug(const std::string& component, const std::string& message);

  /**
   * @brief
   *   Display an error message.
   *
   * @return Nothing.
   *
   * @param component  Name of the function generating the message.
   * @param message    Message string.
   *
   * Newline characters shouldn't generally be used, as the message
   * would be split automatically according to the width specified with
   * @b cpl_msg_set_width(). Inserting a newline character would
   * enforce breaking a line of text even before the current row is
   * filled. Newline characters at the end of the @em format string
   * are not required. If @em component is a @c NULL pointer, it would
   * be set to the string "<empty field>". If @em format is a @c NULL
   * pointer, the message "<empty message>" would be printed.
   */
  static void error(const std::string& component, const std::string& message);

  /**
   * @brief
   *   Display an information message.
   *
   * @return Nothing.
   *
   * @param component  Name of the function generating the message.
   * @param message    Message string.
   *
   * See the description of the function @c cpl_msg_error().
   */
  static void info(const std::string& component, const std::string& message);

  /**
   * @brief
   *   Display a warning message.
   *
   * @return Nothing.
   *
   * @param component  Name of the function generating the message.
   * @param message    Message string.
   *
   * See the description of the function @c cpl_msg_error().
   */
  static void warning(const std::string& component, const std::string& message);

  /**
   * @brief
   *   Set the maximum width of the displayed text.
   *
   * @param widthSet Max width of the displayed text.
   *
   * @return Nothing.
   *
   * If a message is longer than @em width characters, it would be broken
   * into shorter lines before being displayed to terminal. However, words
   * longer than @em width would not be broken, and in this case longer
   * lines would be printed. This function is automatically called by the
   * messaging system every time the terminal window is resized, and the
   * width is set equal to the new width of the terminal window. If @em width
   * is zero or negative, long message lines would not be broken. Lines are
   * never broken in log files.
   */
  static void set_width(int widthSet);
  static int get_width();

  /**
   * @brief
   *   Set the indentation level.
   *
   * @return Nothing.
   *
   * @param level Indentation level.
   *
   * Any message printed after a call to this function would be indented
   * by a number of characters equal to the @em level multiplied by the
   * indentation step specified with the function @c cpl_msg_set_indentation().
   * Specifying a negative indentation level would set the indentation
   * level to zero.
   */
  static void set_indent(int level);
  static int get_indent();

  /**
   * @brief
   *   Changes the setting to show thread_id
   *
   * @return Nothing.
   *
   * @param setting Activates/deactivates thread id in messages
   *
   * Calls thread cpl_msg_set_threadid_off() if setting is false,
   * cpl_msg_set_threadid_on() if setting is true
   */
  static void set_thread_id_switch(bool setting);
  static bool get_thread_id_switch();

  /**
   * @brief
   *   Changes the setting to show domain
   *
   * @return Nothing.
   *
   * @param setting Activates/deactivates domain in messages
   *
   * Calls thread cpl_msg_set_domain_off() if setting is false,
   * cpl_msg_set_domain_on() if setting is true
   */
  static void set_domain_switch(bool setting);
  static bool get_domain_switch();

  /**
   * @brief
   *   Changes the setting to show the time message is printed
   *
   * @return Nothing.
   *
   * @param setting Activates/deactivates time in messages
   *
   * Calls thread cpl_msg_set_time_off() if setting is false,
   * cpl_msg_set_time_on() if setting is true
   */
  static void set_time_switch(bool setting);
  static bool get_time_switch();

  /**
   * @brief
   *   Changes the setting to show the component in messages
   *
   * @return Nothing.
   *
   * @param setting Activates/deactivates component in messages
   *
   * Calls thread cpl_msg_set_component_off() if setting is false,
   * cpl_msg_set_component_on() if setting is true
   */
  static void set_component_switch(bool setting);
  static bool get_component_switch();

 private:
  /**
   * @brief Private method for casting python logging levels to cpl equivalents
   *
   * @return Nothing.
   *
   * @param val Value of the python logging level
   *
   */
  static cpl_msg_severity castPythonLevels(int val);
  // Series of properties to keep track of user settings: as cpl does not
  // provide getters for these.
  static int current_indentation;
  static int current_width;
  static bool display_thread_id;
  static bool display_domain;
  static bool display_time;
  static bool display_component;
};
}  // namespace core
}  // namespace cpl

#endif  // PYCPL_MSG_HPP