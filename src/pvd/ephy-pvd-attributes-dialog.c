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
#include <json-glib/json-glib.h>

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
transform_array_elem (gpointer data,
                      gpointer user_data)
{
  JsonNode *elem = (JsonNode *) data;
  char *string = (char *) user_data;
  char *type = json_node_type_name (elem);

  if (strcmp (type, "String") == 0) {
    // construct string where there is one string value per line
    strcat (string, json_node_dup_string (elem));
    strcat (string, "\n");
  } else if (strcmp (type, "JsonObject") == 0) {
    JsonObject *object = json_node_get_object (elem);
    GList *object_keys = json_object_get_members (object);
    char *key_str;
    JsonNode *value;

    // iterate through the key-value pairs of the object
    for (GList *key = g_list_first (object_keys); key; key = g_list_next (key)) {
      key_str = (char *) g_list_nth_data (key, 0);
      strcat (string, key_str);

      if ((strlen (key_str) + 1)/4 >= 5)
        strcat (string, ": "); // the key is too long to align its value with the others
      else {
        // align the values to the right
        for (size_t i = 0; i < 5 - (strlen(key_str)+1)/4; ++i)
          strcat (string, "\t");
      }

      // retrieve value and detect its type
      value = json_object_get_member (object, key_str);
      type = json_node_type_name (value);

      if (strcmp (type, "String") == 0)
        strcat (string, json_node_dup_string (value));
      else if (strcmp (type, "Integer") == 0) {
        long int value_int = json_node_get_int (value);
        char value_str[24];
        sprintf (value_str, "%ld", value_int);
        strcat (string, value_str);
      }
      strcat (string, "\n");
    }
    strcat (string, "\n");
  }
}

static const char *
parse_json_array (const char *arr_str)
{
  JsonParser *parser;
  JsonNode *root;
  JsonArray *array;
  const char *type;
  GList *array_elems;
  char *trans_str;
  GError *error;

  // parse JSON
  parser = json_parser_new ();
  error = NULL;
  json_parser_load_from_data (parser, arr_str, strlen (arr_str), &error);
  if (error) {
    return NULL; //TODO: add error message (logging)
  }

  root = json_parser_get_root (parser);
  type = json_node_type_name (root);

  if (strcmp (type, "JsonArray") != 0) {
    // no array type
    g_object_unref (parser);
    return NULL;
  }

  trans_str = g_malloc (sizeof (char) * strlen (arr_str));
  trans_str[0] = '\0';

  if (trans_str == NULL) {
    g_object_unref (parser);
    return NULL;
  }

  // get array elements
  array = json_node_get_array (root);
  array_elems = json_array_get_elements (array);

  g_list_foreach (array_elems, transform_array_elem, trans_str);

  g_list_free (array_elems);
  g_object_unref (parser);
  return trans_str;
}

static GtkWidget *
create_row (EphyPvdAttributesDialog *self,
            const char *attr_key,
            const char *attr_val)
{
  GtkWidget *row;
  GtkWidget *key;
  GtkWidget *value;
  GtkWidget *grid;
  const char *parsed_array_val = parse_json_array (attr_val);

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
  value = gtk_label_new (parsed_array_val ? parsed_array_val : attr_val);
  gtk_label_set_ellipsize (GTK_LABEL (value), PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (GTK_LABEL (value), 0);
  gtk_widget_set_sensitive (value, FALSE);

  gtk_grid_attach (GTK_GRID (grid), value, 0, 1, 1, 1);

  gtk_container_add (GTK_CONTAINER (row), grid);
  gtk_widget_show_all (row);

  return row;
}

void
ephy_pvd_attributes_dialog_add_attr_rows (EphyPvdAttributesDialog *self)
{
  const char *attr_keys[] = {"name", "id", "sequenceNumber", "hFlag", "lFlag", "aFlag", "implicit", "lla", "dev",
                             "addresses", "routes", "rdnss", "dnssl"};
  const char *attr_vals[] = {"test.example.com.", "1", "0", "0", "0", "0", "false", "fe80::d828:9ff:feee:4310", "",
                             "[\n"
                             "\t{\"address\" : \"2001:db8:1:0:300b:f0ff:fe8c:55de\", \"length\" : 64 },\n"
                             "\t{\"address\" : \"2001:db8:1:abcd:300b:f0ff:fe8c:55de\", \"length\" : 64 },\n"
                             "\t{\"address\" : \"2001:db8:1:beef:300b:f0ff:fe8c:55de\", \"length\" : 64 }\n"
                             "]",
                             "[\n"
                             "\t{\"dst\" : \"::\", \"gateway\" : \"fe80::b44a:1fff:fe47:1147\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1000::\", \"gateway\" : \"fe80::b44a:1fff:fe47:1147\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1a00::\", \"gateway\" : \"fe80::b44a:1fff:fe47:1147\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1::\", \"gateway\" : \"::\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1:0:300b:f0ff:fe8c:55de\", \"gateway\" : \"::\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1:abcd::\", \"gateway\" : \"::\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1:abcd:300b:f0ff:fe8c:55de\", \"gateway\" : \"::\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1:beef::\", \"gateway\" : \"::\", \"dev\" : \"eh0\" },\n"
                             "\t{\"dst\" : \"2001:db8:1:beef:300b:f0ff:fe8c:55de\", \"gateway\" : \"::\", \"dev\" : \"eh0\" }\n"
                             "]",
                             "[\"2001:1111:1111::8888\", \"2001:1111:1111::8844\"]",
                             "[\"office.test1.example.com\", \"test1.example.com\", \"example.com\", \"special.test1-pvd.example.com\"]"};
  GtkWidget *row;

  for (int i = 0; i < 13; ++i) {
    row = create_row (self, attr_keys[i], attr_vals[i]);
    gtk_list_box_insert (GTK_LIST_BOX (self->listbox), row, -1);
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

  //gtk_list_box_set_header_func (GTK_LIST_BOX (self->listbox), box_header_func, NULL, NULL);
  ephy_gui_ensure_window_group (GTK_WINDOW (self));
}