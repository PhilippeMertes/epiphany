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
#include "ephy-tag-pvd.h"

struct _EphyTagPvd {
    GObject parent_instance;

    const char *tag;
    const char *pvd;
};

G_DEFINE_TYPE (EphyTagPvd, ephy_tag_pvd, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_TAG,
    PROP_PVD,
    LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

static void
ephy_tag_pvd_set_property (GObject       *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  EphyTagPvd *self = EPHY_TAG_PVD (object);

  switch (prop_id) {
    case PROP_TAG:
      ephy_tag_pvd_set_tag (self, g_value_get_string (value));
      break;
    case PROP_PVD:
      ephy_tag_pvd_set_pvd (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_tag_pvd_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  EphyTagPvd *self = EPHY_TAG_PVD (object);

  switch (prop_id) {
    case PROP_TAG:
      g_value_set_string (value, ephy_tag_pvd_get_tag (self));
      break;
    case PROP_PVD:
      g_value_set_string (value, ephy_tag_pvd_get_pvd (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_tag_pvd_finalize (GObject *object)
{
  EphyTagPvd *self = EPHY_TAG_PVD (object);

  g_free ((char *)self->tag);

  g_free ((char *)self->pvd);

  G_OBJECT_CLASS (ephy_tag_pvd_parent_class)->finalize (object);
}

static void
ephy_tag_pvd_class_init (EphyTagPvdClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ephy_tag_pvd_set_property;
  object_class->get_property = ephy_tag_pvd_get_property;
  object_class->finalize = ephy_tag_pvd_finalize;

  obj_properties[PROP_TAG] =
    g_param_spec_string ("tag",
      "Tag",
      "Bookmark tag",
      "Default tag name",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_PVD] =
    g_param_spec_string ("pvd",
      "PvD",
      "Associated PvD's name",
      "Default PvD name",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);
}

static void
ephy_tag_pvd_init (EphyTagPvd *self)
{
  self->tag = NULL;
}

EphyTagPvd *
ephy_tag_pvd_new (const char *tag, const char *pvd)
{
  return g_object_new (EPHY_TYPE_TAG_PVD,
                       "tag", tag,
                       "pvd", pvd,
                       NULL);
}

const char *
ephy_tag_pvd_get_tag (EphyTagPvd *self)
{
  g_assert (EPHY_IS_TAG_PVD (self));

  return self->tag;
}

void
ephy_tag_pvd_set_tag (EphyTagPvd *self,
                      const char *tag)
{
  g_assert (EPHY_IS_TAG_PVD (self));

  g_free ((char *)self->tag);
  self->tag = g_strdup (tag);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_TAG]);
}

const char *
ephy_tag_pvd_get_pvd (EphyTagPvd *self)
{
  g_assert (EPHY_IS_TAG_PVD (self));

  return self->pvd;
}

void
ephy_tag_pvd_set_pvd (EphyTagPvd *self,
                      const char *pvd)
{
  g_assert (EPHY_IS_TAG_PVD (self));

  g_free ((char *)self->pvd);
  self->pvd = g_strdup (pvd);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_PVD]);
}