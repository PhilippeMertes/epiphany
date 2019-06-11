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

static GtkWidget *
create_row (EphyPvdAttributesDialog *self,
            const char *attr_key,
            const char *attr_val)
{
  printf("create_row\n");
  GtkWidget *row;
  GtkWidget *key;
  GtkWidget *value;
  GtkWidget *grid;

  PangoAttrList *attrlist;
  PangoAttribute *attr;

  row = gtk_list_box_row_new ();
  g_object_set_data (G_OBJECT (row), "name", g_strdup (attr_key));
  printf ("object_set_data\n");

  grid = gtk_grid_new ();
  gtk_widget_set_margin_start (grid, 6);
  gtk_widget_set_margin_end (grid, 6);
  gtk_widget_set_margin_top (grid, 6);
  gtk_widget_set_margin_bottom (grid, 6);
  printf("set_margin\n");
  //gtk_grid_set_column_spacing (GTK_GRID(grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID(grid), 6);
  gtk_widget_set_tooltip_text (grid, attr_key);
  printf ("attribute key\n");
  // attribute key
  key = gtk_label_new (attr_key);
  gtk_label_set_ellipsize (GTK_LABEL (key), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand (key, TRUE);
  gtk_label_set_xalign (GTK_LABEL (key), 0);
  printf ("grid\n");
  // grid
  attrlist = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_SEMIBOLD);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (key), attrlist);
  pango_attr_list_unref (attrlist);

  gtk_grid_attach (GTK_GRID (grid), key, 0, 0, 1, 1);
  printf ("attached\n");

  // attribute value
  value = gtk_label_new (attr_val);
  gtk_label_set_ellipsize (GTK_LABEL (value), PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (GTK_LABEL (value), 0);
  gtk_widget_set_sensitive (value, FALSE);

  gtk_grid_attach (GTK_GRID (grid), value, 0, 1, 1, 1);

  printf ("container_add\n");
  gtk_container_add (GTK_CONTAINER (row), grid);
  printf ("show_all\n");
  gtk_widget_show_all (row);
  printf("return\n");

  return row;
}

void
ephy_pvd_attributes_dialog_add_attr_rows (EphyPvdAttributesDialog *self)
{
  printf("add_pvd_attributes\n");
  const char *attr_keys[] = {"name", "id", "sequenceNumber", "hFlag", "lFlag", "aFlag", "implicit", "lla", "dev"};
  const char *attr_vals[] = {"test.example.com.", "1", "0", "0", "0", "0", "false", "fe80::d828:9ff:feee:4310", ""};
  GtkWidget *row;

  for (int i = 0; i < 9; ++i) {
    row = create_row(self, attr_keys[i], attr_vals[i]);
    gtk_list_box_insert (GTK_LIST_BOX (self->listbox), row, -1);
    printf ("gtk_list_box_insert, i = %d\n", i);
  }
}

static void
box_header_func (GtkListBoxRow *row,
                 GtkListBoxRow *before,
                 gpointer       user_data)
{
  GtkWidget *current;

  if (!before) {
    gtk_list_box_row_set_header (row, NULL);
    return;
  }

  current = gtk_list_box_row_get_header (row);
  if (!current) {
    current = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (current);
    gtk_list_box_row_set_header (row, current);
  }
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

  gtk_list_box_set_header_func (GTK_LIST_BOX (self->listbox), box_header_func, NULL, NULL);
  ephy_gui_ensure_window_group (GTK_WINDOW (self));
}