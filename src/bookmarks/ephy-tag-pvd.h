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

#include <glib-object.h>

G_BEGIN_DECLS

#define EPHY_TYPE_TAG_PVD (ephy_tag_pvd_get_type ())

G_DECLARE_FINAL_TYPE (EphyTagPvd, ephy_tag_pvd, EPHY, TAG_PVD, GObject)

EphyTagPvd    *ephy_tag_pvd_new     (const char *tag, const char *pvd);
const char    *ephy_tag_pvd_get_tag (EphyTagPvd *self);
void           ephy_tag_pvd_set_tag (EphyTagPvd *self,
                                     const char *tag);
const char    *ephy_tag_pvd_get_pvd (EphyTagPvd *self);
void           ephy_tag_pvd_set_pvd (EphyTagPvd *self,
                                     const char *pvd);

G_END_DECLS