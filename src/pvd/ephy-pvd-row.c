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

#include "ephy-pvd-row.h"

struct _EphyPvdRow {
    GtkListBoxRow    parent_instance;

    EphyPvd          *pvd;

    GtkWidget        *name_label;
};

G_DEFINE_TYPE (EphyPvdRow, ephy_pvd_row, GTK_TYPE_LIST_BOX_ROW)

#define EPHY_LIST_BOX_ROW_TYPE_PVD "pvd"

enum {
    PROP_0,
    PROP_PVD,
    LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

static void
ephy_pvd_row_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  EphyPvdRow *self = EPHY_PVD_ROW (object);

  switch (prop_id)
  {
    case PROP_PVD:
      self->pvd = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_row_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  EphyPvdRow *self = EPHY_PVD_ROW (object);

  switch (prop_id)
  {
    case PROP_PVD:
      g_value_set_object (value, ephy_pvd_row_get_pvd (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_row_dispose (GObject *object)
{
  EphyPvdRow *self = EPHY_PVD_ROW (object);

  g_clear_object (&self->pvd);

  G_OBJECT_CLASS (ephy_pvd_row_parent_class)->dispose (object);
}

/*static gboolean
transform_pvd_name (GBinding     *binding,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data)
{
  const char *name;

  name = g_value_get_string (from_value);
  g_value_set_string (to_value, name);

  return TRUE;
}*/

static void
ephy_pvd_row_constructed (GObject *object)
{
  EphyPvdRow *self = EPHY_PVD_ROW (object);

  G_OBJECT_CLASS (ephy_pvd_row_parent_class)->constructed (object);

  // bind row label to PvD name
  g_object_bind_property (self->pvd, "name",
    self->name_label, "label",
    G_BINDING_SYNC_CREATE);
}

static void
ephy_pvd_row_class_init (EphyPvdRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = ephy_pvd_row_set_property;
  object_class->get_property = ephy_pvd_row_get_property;
  object_class->dispose = ephy_pvd_row_dispose;
  object_class->constructed = ephy_pvd_row_constructed;

  obj_properties[PROP_PVD] =
    g_param_spec_object ("pvd",
                         "An EphyPvd object",
                         "The EphyPvd shown by this widget",
                         EPHY_TYPE_PVD,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/pvd-row.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyPvdRow, name_label);
}

static void
ephy_pvd_row_init (EphyPvdRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
ephy_pvd_row_new (EphyPvd *pvd)
{
  return g_object_new (EPHY_TYPE_PVD_ROW,
                       "pvd", pvd,
                       NULL);
}

EphyPvd *
ephy_pvd_row_get_pvd (EphyPvdRow *self)
{
  g_assert (EPHY_IS_PVD_ROW (self));

  return self->pvd;
}