/* MegaZeux
 *
 * Copyright (C) 1996 Greg Janson
 * Copyright (C) 1999 Charles Goetzman
 * Copyright (C) 2002 B.D.A. (Koji) - Koji_Takeo@worldmailer.com
 * Copyright (C) 2002 Gilead Kutnick <exophase@adelphia.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "compat.h"
#include "platform.h"

#include "configure.h"
#include "core.h"
#include "event.h"
#include "helpsys.h"
#include "graphics.h"
#include "window.h"
#include "data.h"
#include "game.h"
#include "error.h"
#include "idput.h"
#include "util.h"
#include "world.h"
#include "counter.h"
#include "run_stubs.h"

#include "audio/audio.h"
#include "audio/sfx.h"
#include "network/network.h"

#ifdef CONFIG_SDL
#include <SDL.h>
#endif

#ifndef VERSION
#error Must define VERSION for MegaZeux version string
#endif

#ifndef VERSION_DATE
#define VERSION_DATE
#endif

#define CAPTION "MegaZeux " VERSION VERSION_DATE

#ifdef __WIN32__
// Export symbols to indicate MegaZeux would be prefer to be run with
// the better video card on switchable graphics platforms.

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001; // Nvidia
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; // AMD
#endif

#ifdef __amigaos__
#define __libspec LIBSPEC
#else
#define __libspec
#endif

__libspec int main(int argc, char *argv[])
{
  char *_backup_argv[] = { (char*) SHAREDIR "/megazeux" };
  int err = 1;

  core_context *core_data;
  struct config_info *conf;

  // Keep this 7.2k structure off the stack..
  static struct world mzx_world;

  if(!platform_init())
    goto err_out;

  // We need to store the current working directory so it's
  // always possible to get back to it..
  getcwd(current_dir, MAX_PATH);

#ifdef __APPLE__
  if(!strcmp(current_dir, "/") || !strncmp(current_dir, "/App", 4))
  {
    // Mac .APPs start at / and we don't like that.
    char *user = getlogin();
    snprintf(current_dir, MAX_PATH, "/Users/%s", user);
  }
#endif // __APPLE__

  // argc may be 0 on e.g. some Wii homebrew loaders.
  if(argc == 0)
  {
    argv = _backup_argv;
    argc = 1;
  }

  if(mzx_res_init(argv[0], is_editor()))
    goto err_free_res;

  editor_init();

  // Figure out where all configuration files should be loaded
  // form. For game.cnf, et al this should eventually be wrt
  // the game directory, not the config.txt's path.
  get_path(mzx_res_get_by_id(CONFIG_TXT), config_dir, MAX_PATH);

  // Move into the configuration file's directory so that any
  // "include" statements are done wrt this path. Move back
  // into the "current" directory after loading.
  chdir(config_dir);

  default_config();
  set_config_from_file_startup(mzx_res_get_by_id(CONFIG_TXT));
  set_config_from_command_line(&argc, argv);
  conf = get_config();

  default_editor_config();
  set_editor_config_from_file(mzx_res_get_by_id(CONFIG_TXT));
  set_editor_config_from_command_line(&argc, argv);
  store_editor_config_backup();

  init_macros();

  // Startup path might be relative, so change back before checking it.
  chdir(current_dir);

  // At this point argv should have all the config options
  // of the form var=value removed, leaving only unparsed
  // parameters. Interpret the first unparsed parameter
  // as a file to load (overriding startup_file etc.)
  if(argc > 1)
    split_path_filename(argv[1], conf->startup_path, 256,
     conf->startup_file, 256);

  if(conf->startup_path && strlen(conf->startup_path))
  {
    debug("Config: Using startup path '%s'\n", conf->startup_path);
    snprintf(current_dir, MAX_PATH, "%s", conf->startup_path);
  }

  chdir(current_dir);

  counter_fsg();

  rng_seed_init();

  initialize_joysticks();

  set_mouse_mul(8, 14);

  init_event();

  if(!init_video(conf, CAPTION))
    goto err_free_config;
  init_audio(conf);

  if(network_layer_init(conf))
  {
#ifdef CONFIG_UPDATER
    if(is_updater())
    {
      if(updater_init(argc, argv))
      {
        // No auto update checks on repo builds.
        if(!strcmp(VERSION, "GIT") &&
         !strcmp(conf->update_branch_pin, "Stable"))
          conf->update_auto_check = UPDATE_AUTO_CHECK_OFF;

        if(conf->update_auto_check)
          check_for_updates(&mzx_world, true);
      }
      else
        info("Updater disabled.\n");
    }
#endif
  }
  else
    info("Network layer disabled.\n");

  warp_mouse(39, 12);
  cursor_off();
  default_scroll_values(&mzx_world);

#ifdef CONFIG_HELPSYS
  help_open(&mzx_world, mzx_res_get_by_id(MZX_HELP_FIL));
#endif

  snprintf(curr_file, MAX_PATH, "%s", conf->startup_file);
  snprintf(curr_sav, MAX_PATH, "%s", conf->default_save_name);
  mzx_world.mzx_speed = conf->mzx_speed;

  // Run main game (mouse is hidden and palette is faded)
  core_data = core_init(&mzx_world);
  title_screen((context *)core_data);
  core_run(core_data);
  core_free(core_data);

  vquick_fadeout();

  destruct_layers();

  if(mzx_world.active)
  {
    clear_world(&mzx_world);
    clear_global_data(&mzx_world);
  }

#ifdef CONFIG_HELPSYS
  help_close(&mzx_world);
#endif

  network_layer_exit(conf);
  quit_audio();

  err = 0;
err_free_config:
  // FIXME maybe shouldn't be here...?
  if(mzx_world.update_done)
    free(mzx_world.update_done);
  free_config();
  free_editor_config();
err_free_res:
  mzx_res_free();
  platform_quit();
err_out:
  return err;
}
