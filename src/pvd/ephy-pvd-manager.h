/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2019 Philippe Mertes <pmertes@student.uliege.be>
 *
 *  This file is part of Epiphany.
 *
 *  Epiphany is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Epiphany is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Epiphany.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "ephy-pvd.h"

//#include <glib/gi18n.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EPHY_TYPE_PVD_MANAGER (ephy_pvd_manager_get_type ())

G_DECLARE_FINAL_TYPE (EphyPvdManager, ephy_pvd_manager, EPHY, PVD_MANAGER, GObject)

EphyPvdManager *ephy_pvd_manager_new                  (void);

GSequence      *ephy_pvd_manager_get_pvd_list         (EphyPvdManager *self);

G_END_DECLS