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

#include "config.h"
#include "ephy-pvd.h"

#include "ephy-synchronizable.h"

#define PVD_TYPE_VAL      "pvd"

struct _EphyPvd {
    GObject parent_instance;

    char *name;
    GHashTable *attributes; // TODO: maybe make this a property
};

G_DEFINE_TYPE (EphyPvd, ephy_pvd, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_NAME,
    LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

static void
ephy_pvd_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  EphyPvd *self = EPHY_PVD (object);

  switch (prop_id) {
    case PROP_NAME:
      ephy_pvd_set_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  EphyPvd *self = EPHY_PVD (object);

  switch (prop_id) {
    case PROP_NAME:
      g_value_set_string (value, ephy_pvd_get_name (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_finalize (GObject *object)
{
  EphyPvd *self = EPHY_PVD (object);

  g_free (self->name);

  G_OBJECT_CLASS (ephy_pvd_parent_class)->finalize (object);
}

static void
ephy_pvd_class_init (EphyPvdClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ephy_pvd_set_property;
  object_class->get_property = ephy_pvd_get_property;
  object_class->finalize = ephy_pvd_finalize;

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
      "Name",
      "The PvD's name",
      "Default PvD name",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);
}

static void
ephy_pvd_init (EphyPvd *self)
{
  self->name = NULL;
  self->attributes = g_hash_table_new (g_str_hash, g_str_equal);
}

EphyPvd *
ephy_pvd_new (const char *name)
{
  return g_object_new(EPHY_TYPE_PVD,
                      "name", name,
                      NULL);
}

const char *
ephy_pvd_get_name (EphyPvd *pvd)
{
  g_assert (EPHY_IS_PVD (pvd));

  return pvd->name;
}

GHashTable *
ephy_pvd_get_attributes (EphyPvd *self)
{
  g_assert (EPHY_IS_PVD (self));

  return self->attributes;
}

void
ephy_pvd_set_name (EphyPvd* self, const char *name)
{
  g_assert (EPHY_IS_PVD (self));

  g_free (self->name);
  self->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_NAME]);
}

gboolean
ephy_pvd_add_attribute (EphyPvd *self,
                        char *name,
                        gpointer value)
{
  return g_hash_table_insert (self->attributes, name, value);
}

JsonNode *
ephy_pvd_get_attribute (EphyPvd *self,
                        const char *name)
{
  return NULL;
}

gboolean
ephy_pvd_set_attribute (EphyPvd *self,
                        const char *name,
                        gpointer value)
{
  return TRUE;
}