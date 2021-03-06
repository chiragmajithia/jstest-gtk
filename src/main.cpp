/*
**  jstest-gtk - A graphical joystick tester
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <gtkmm/main.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "binreloc.h"

#include "joystick_test_widget.hpp"
#include "joystick_list_widget.hpp"
#include "joystick_map_widget.hpp"
#include "joystick_calibration_widget.hpp"
#include "joystick.hpp"
#include "main.hpp"

Main* Main::current_ = 0;

Main::Main(const std::string& datadir_)
  : datadir(datadir_),
    m_simple_ui(false),
    list_dialog(0)
{
  current_ = this;
}

Main::~Main()
{
}

void
Main::show_device_list_dialog()
{
  if (list_dialog)
  {
    list_dialog->show();
  }
  else
  {
    list_dialog = new JoystickListWidget();
    dialogs.push_back(list_dialog);
    list_dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), list_dialog));
    list_dialog->show_all();
  }
}

void
Main::show_device_property_dialog(const std::string& filename)
{
  Joystick* joystick = new Joystick(filename);
  JoystickTestWidget* dialog = new JoystickTestWidget(*joystick, m_simple_ui);
  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), dialog));
  dialog->show_all();
  joysticks.push_back(joystick);
  dialogs.push_back(dialog);
}

void
Main::show_calibration_dialog(Joystick& joystick)
{
  JoystickCalibrationWidget* dialog = new JoystickCalibrationWidget(joystick);
  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), dialog));
  dialog->show_all();
  dialogs.push_back(dialog);
}

void
Main::show_mapping_dialog(Joystick& joystick)
{
  JoystickMapWidget* dialog = new JoystickMapWidget(joystick);
  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), dialog));
  dialog->show_all();
  dialogs.push_back(dialog);
}

void
Main::on_dialog_hide(Gtk::Dialog* dialog)
{
  dialogs.erase(std::remove(dialogs.begin(), dialogs.end(), dialog), dialogs.end());
  delete dialog;

  if (dialogs.empty())
  {
    Gtk::Main::quit();
  }
}

int
Main::main(int argc, char** argv)
{
  typedef std::vector<std::string> DeviceFiles;
  DeviceFiles device_files;

  for(int i = 1; i < argc; ++i)
  {
    if (strcmp("--help", argv[i]) == 0 ||
        strcmp("-h", argv[i]) == 0)
    {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]... [DEVICE]...\n"
                << "A graphical joystick tester.\n"
                << "\n"
                << "Options:\n"
                << "  -h, --help      Display this help and exit\n"
                << "  -v, --version   Display version information and exit\n"
                << "  --simple        Hide graphical representation of axis\n"
                << "\n"
                << "Report bugs to Ingo Ruhnke <grumbel@gmx.de>.\n";
      return 0;
    }
    else if (strcmp("--version", argv[i]) == 0 ||
             strcmp("-v", argv[i]) == 0)
    {
      std::cout << "jstest-gtk 0.1.0" << std::endl;
      return 0;
    }
    else if (strcmp("--simple", argv[i]) == 0)
    {
      m_simple_ui = true;
    }
    else if (argv[i][0] == '-')
    {
      std::cout << "Error: " << argv[0] << ": unrecognized option '" << argv[i] << "'" << std::endl;
      return EXIT_FAILURE;
    }
    else
    {
      device_files.push_back(argv[i]);
    }
  }

  try
  {
    Glib::set_application_name("Joystick Test");
    Glib::set_prgname("jstest-gtk");
    cfg_directory = Glib::build_filename(Glib::get_user_config_dir(), Glib::get_prgname());
    if (access(cfg_directory.c_str(), R_OK | W_OK) != 0 &&
        mkdir(cfg_directory.c_str(), 0770) != 0)
    {
      throw std::runtime_error(cfg_directory + ": " + strerror(errno));
    }
    Gtk::Main kit(&argc, &argv);

    if (device_files.empty())
    {
      show_device_list_dialog();
    }
    else
    {
      for(DeviceFiles::iterator i = device_files.begin(); i != device_files.end(); ++i)
      {
        show_device_property_dialog(*i);
      }
    }
    Gtk::Main::run();
  }
  catch(std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch(Glib::Exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}

std::string find_datadir()
{
  BrInitError error;
  if (!br_init(&error))
  {
    std::ostringstream out;
    out << "Error: Couldn't init binreloc: " << error;
    throw std::runtime_error(out.str());
  }
  else
  {
    char* c_prefix = br_find_exe_dir(NULL);
    if (!c_prefix)
    {
      throw std::runtime_error("Error: Couldn't find prefix");
    }
    else
    {
      std::string prefix = c_prefix;
      free(c_prefix);
      return prefix + "/data/";
    }
  }
}

int main(int argc, char** argv)
{
  try
  {
    Main app(find_datadir());
    return app.main(argc, argv);
  }
  catch(std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}

/* EOF */
