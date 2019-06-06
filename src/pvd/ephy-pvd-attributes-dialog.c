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
#include "ephy-pvd-attributes-dialog.h"

#include "ephy-debug.h"
#include "ephy-gui.h"
#include "ephy-prefs.h"
#include "ephy-settings.h"
#include "ephy-shell.h"
#include "ephy-snapshot-service.h"
#include "ephy-uri-helpers.h"
#include "ephy-time-helpers.h"
#include "ephy-window.h"

#include <gtk/gtk.h>

struct _EphyPvdAttributesDialog {
    GtkWindow parent_instance;

    GtkWidget *listbox;
};

G_DEFINE_TYPE (EphyPvdAttributesDialog, ephy_pvd_attributes_dialog, GTK_TYPE_WINDOW)

static void
ephy_pvd_attributes_dialog_dispose (GObject *object)
{
  G_OBJECT_CLASS (ephy_pvd_attributes_dialog_parent_class)->dispose (object);
}

static void
on_edge_reached (GtkScrolledWindow *scrolled,
                 GtkPositionType    pos,
                 gpointer           user_data)
{
  printf ("on_edge_reached\n");
  /*EphyPvdAttributesDialog *self = EPHY_PVD_ATTRIBUTES_DIALOG (user_data);

  if (pos == GTK_POS_BOTTOM)*/
}

static void
ephy_pvd_attributes_dialog_class_init (EphyPvdAttributesDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = ephy_pvd_attributes_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/pvd-attributes-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyPvdAttributesDialog, listbox);
  gtk_widget_class_bind_template_callback (widget_class, on_edge_reached);
}

GtkWidget *
ephy_pvd_attributes_dialog_new (const char *pvd_name)
{
  EphyPvdAttributesDialog *self;

  g_assert (pvd_name != NULL);

  self = g_object_new (EPHY_TYPE_PVD_ATTRIBUTES_DIALOG, NULL);

  return GTK_WIDGET (self);
}

static void
ephy_pvd_attributes_dialog_init (EphyPvdAttributesDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  ephy_gui_ensure_window_group (GTK_WINDOW (self));
}