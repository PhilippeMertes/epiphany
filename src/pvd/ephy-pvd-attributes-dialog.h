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

#include <gtk/gtk.h>

#include "ephy-pvd.h"

G_BEGIN_DECLS

#define EPHY_TYPE_PVD_ATTRIBUTES_DIALOG (ephy_pvd_attributes_dialog_get_type ())

G_DECLARE_FINAL_TYPE (EphyPvdAttributesDialog, ephy_pvd_attributes_dialog, EPHY, PVD_ATTRIBUTES_DIALOG, GtkWindow)

GtkWidget      *ephy_pvd_attributes_dialog_new                 (const char *pvd_name);
void            ephy_pvd_attributes_dialog_add_attr_rows       (EphyPvdAttributesDialog *self);
void            ephy_pvd_attributes_dialog_add_extra_attr_rows (EphyPvdAttributesDialog *self);

G_END_DECLS
