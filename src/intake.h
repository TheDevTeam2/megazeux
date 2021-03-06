/* MegaZeux
 *
 * Copyright (C) 1996 Greg Janson
 * Copyright (C) 2018 Alice Rowan <petrifiedrowan@gmail.com>
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

#ifndef __INTAKE_H
#define __INTAKE_H

#include "compat.h"

__M_BEGIN_DECLS

#include "core.h"

enum intake_exit_type
{
  INTK_EXIT_ENTER,
  INTK_EXIT_ENTER_ESC,
  INTK_EXIT_ANY
};

CORE_LIBSPEC int intake(struct world *mzx_world, char *string, int max_len,
 int x, int y, char color, enum intake_exit_type exit_type, int *return_x_pos);

CORE_LIBSPEC subcontext *intake2(context *parent, char *dest, int max_length,
 int x, int y, int width, int color, int *pos_external, int *length_external);

CORE_LIBSPEC void intake_sync(subcontext *intk);
CORE_LIBSPEC void intake_set_color(subcontext *intk, int color);
CORE_LIBSPEC void intake_set_screen_pos(subcontext *intk, int x, int y);
CORE_LIBSPEC const char *intake_input_string(subcontext *intk, const char *src,
 char linebreak_char);

__M_END_DECLS

#endif // __INTAKE_H

