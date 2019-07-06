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
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/*union attribute_value {
    int i;
    gboolean b;
    char *str;
};

typedef struct attribute {
    const char *type;
    union attribute_value val;
} attribute_t;*/

#define EPHY_TYPE_PVD (ephy_pvd_get_type ())

G_DECLARE_FINAL_TYPE (EphyPvd, ephy_pvd, EPHY, PVD, GObject)

EphyPvd       *ephy_pvd_new                       (const char *name);
const char    *ephy_pvd_get_name                  (EphyPvd *self);
void           ephy_pvd_set_name                  (EphyPvd *self,
                                                   const char *name);
GHashTable    *ephy_pvd_get_attributes            (EphyPvd *self);
gboolean       ephy_pvd_set_attributes            (EphyPvd *self,
                                                   const char *json_str);
/*gboolean       ephy_pvd_add_attribute             (EphyPvd *self,
                                                   const char *name,
                                                   JsonNode *value);*/
JsonNode      *ephy_pvd_get_attribute             (EphyPvd *self,
                                                   const char *name);
gboolean       ephy_pvd_set_attribute_int         (EphyPvd *self,
                                                   const char *name,
                                                   gint64 value);
gboolean       ephy_pvd_set_attribute_boolean     (EphyPvd *self,
                                                   const char *name,
                                                   gboolean value);


G_END_DECLS
