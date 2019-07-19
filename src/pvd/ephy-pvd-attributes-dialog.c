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
#include "ephy-time-helpers.h"
#include "ephy-window.h"
#include "ephy-pvd-manager.h"

#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

struct _EphyPvdAttributesDialog {
    GtkWindow parent_instance;

    GtkWidget *attributes_listbox;
    GtkWidget *extra_attributes_listbox;

    GtkWidget *extra_attributes_label;
    GtkWidget *extra_attributes_window;

    EphyPvd *pvd;
};

G_DEFINE_TYPE (EphyPvdAttributesDialog, ephy_pvd_attributes_dialog, GTK_TYPE_WINDOW)

static void transform_attribute_value_to_string (GString *string, JsonNode *value);
static void transform_array_elements (JsonArray *array, guint index, JsonNode *element_node, gpointer user_data);

static void
ephy_pvd_attributes_dialog_dispose (GObject *object)
{
  G_OBJECT_CLASS (ephy_pvd_attributes_dialog_parent_class)->dispose (object);
}

static void
transform_array_elements (JsonArray *array,
                          guint index,
                          JsonNode *element_node,
                          gpointer user_data)
{
  GString *string = (GString *)user_data;
  transform_attribute_value_to_string (string, element_node);
  if (index < json_array_get_length (array)-1)
    g_string_append (string, "\n\n"); // add blank line except if it is the last array element
}

static void
transform_attribute_value_to_string (GString *string,
                                     JsonNode *value)
{
  const char *type = json_node_type_name (value);

  if (strcmp (type, "Boolean") == 0)
    g_string_append (string, json_node_get_boolean (value) ? "true" : "false");
  else if (strcmp (type, "Integer") == 0)
    g_string_append_printf (string, "%ld", json_node_get_int (value));
  else if (strcmp (type, "String") == 0)
    g_string_append (string, json_node_dup_string (value));
  else if (strcmp (type, "JsonArray") == 0) {
    JsonArray *array = json_node_get_array (value);
    json_array_foreach_element (array, transform_array_elements, string);
  } else if (strcmp (type, "JsonObject") == 0) {
    JsonObject *object = json_node_get_object (value);
    JsonObjectIter iter;
    const char *key;
    JsonNode *val;
    gboolean first = TRUE;

    json_object_iter_init (&iter, object);
    while (json_object_iter_next (&iter, &key, &val)) {
      if (!first) // prepend each line by a newline char except the first
        g_string_append (string, "\n");
      else
        first = FALSE;
      g_string_append_printf (string, "%s = ", key);
      transform_attribute_value_to_string (string, val);
    }
  }
}

static GtkWidget *
create_row (EphyPvdAttributesDialog *self,
            const char *attr_key,
            JsonNode *attr_val)
{
  GtkWidget *row;
  GtkWidget *key;
  GtkWidget *value;
  GtkWidget *grid;
  GString *attr_val_str = g_string_new (NULL);

  transform_attribute_value_to_string (attr_val_str, attr_val);

  PangoAttrList *attrlist;
  PangoAttribute *attr;

  row = gtk_list_box_row_new ();
  g_object_set_data (G_OBJECT (row), "name", g_strdup (attr_key));

  grid = gtk_grid_new ();
  gtk_widget_set_margin_start (grid, 6);
  gtk_widget_set_margin_end (grid, 6);
  gtk_widget_set_margin_top (grid, 6);
  gtk_widget_set_margin_bottom (grid, 6);
  gtk_grid_set_row_spacing (GTK_GRID(grid), 6);
  gtk_widget_set_tooltip_text (grid, attr_key);

  // attribute key
  key = gtk_label_new (attr_key);
  gtk_label_set_ellipsize (GTK_LABEL (key), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand (key, TRUE);
  gtk_label_set_xalign (GTK_LABEL (key), 0);

  // grid
  attrlist = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_SEMIBOLD);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (key), attrlist);
  pango_attr_list_unref (attrlist);

  gtk_grid_attach (GTK_GRID (grid), key, 0, 0, 1, 1);

  // attribute value
  value = gtk_label_new (attr_val_str->str);
  gtk_label_set_ellipsize (GTK_LABEL (value), PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (GTK_LABEL (value), 0);
  gtk_widget_set_sensitive (value, FALSE);

  gtk_grid_attach (GTK_GRID (grid), value, 0, 1, 1, 1);

  gtk_container_add (GTK_CONTAINER (row), grid);
  gtk_widget_show_all (row);

  g_string_free (attr_val_str, TRUE);

  return row;
}

void
ephy_pvd_attributes_dialog_add_attr_rows (EphyPvdAttributesDialog *self)
{
  GtkWidget *row;
  GHashTable *attributes;
  GHashTableIter iter;
  gpointer attr_key, attr_val;

  g_assert (EPHY_IS_PVD_ATTRIBUTES_DIALOG (self));

  // adding "normal" PvD attributes into the listbox
  attributes = ephy_pvd_get_attributes (self->pvd);
  g_hash_table_iter_init (&iter, attributes);
  while (g_hash_table_iter_next (&iter, &attr_key, &attr_val)) {
    row = create_row (self, (const char *)attr_key, (JsonNode *)attr_val);
    gtk_list_box_insert (GTK_LIST_BOX (self->attributes_listbox), row, -1);
  }

  // adding extra PvD attributes into their listbox (if present)
  if (!ephy_pvd_has_extra_attributes (self->pvd))
    return;
  attributes = ephy_pvd_get_extra_attributes (self->pvd);
  g_hash_table_iter_init (&iter, attributes);
  while (g_hash_table_iter_next (&iter, &attr_key, &attr_val)) {
    row = create_row (self, (const char *)attr_key, (JsonNode *)attr_val);
    gtk_list_box_insert (GTK_LIST_BOX (self->extra_attributes_listbox), row, -1);
  }
}

static void
ephy_pvd_attributes_dialog_class_init (EphyPvdAttributesDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = ephy_pvd_attributes_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/pvd-attributes-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyPvdAttributesDialog, attributes_listbox);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdAttributesDialog, extra_attributes_listbox);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdAttributesDialog, extra_attributes_label);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdAttributesDialog, extra_attributes_window);
}

GtkWidget *
ephy_pvd_attributes_dialog_new (const char *pvd_name)
{
  EphyPvdAttributesDialog *self;
  EphyPvdManager *manager = ephy_shell_get_pvd_manager (ephy_shell_get_default ());

  g_assert (pvd_name != NULL);

  self = g_object_new (EPHY_TYPE_PVD_ATTRIBUTES_DIALOG, NULL);

  self->pvd = ephy_pvd_manager_get_pvd (manager, pvd_name);

  gtk_widget_set_visible (self->extra_attributes_label, ephy_pvd_has_extra_attributes (self->pvd));
  gtk_widget_set_visible (self->extra_attributes_window, ephy_pvd_has_extra_attributes (self->pvd));

  return GTK_WIDGET (self);
}

static void
ephy_pvd_attributes_dialog_init (EphyPvdAttributesDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  ephy_gui_ensure_window_group (GTK_WINDOW (self));
}