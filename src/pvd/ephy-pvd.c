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

struct _EphyPvd {
    GObject parent_instance;

    char *name;
    GHashTable *attributes;
    GHashTable *extra_attributes;
    JsonParser *parser;
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

  g_object_unref (self->parser);

  g_hash_table_destroy (self->attributes);

  g_hash_table_destroy (self->extra_attributes);

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

/*static void
destroy_attribute_value (gpointer value)
{
  attribute_t *attribute = (attribute_t *) value;
  if (strcmp (attribute->type, "String") == 0)
    g_free (attribute->val.str);
  g_free ((char *) attribute->type);
  g_free (attribute);
}*/

static void
ephy_pvd_init (EphyPvd *self)
{
  self->name = NULL;
  self->attributes = g_hash_table_new (g_str_hash, g_str_equal);
  self->extra_attributes = g_hash_table_new (g_str_hash, g_str_equal);
  self->parser = json_parser_new ();
}

EphyPvd *
ephy_pvd_new (const char *name)
{
  return g_object_new (EPHY_TYPE_PVD,
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

static gboolean
ephy_pvd_set_attribute (EphyPvd *self,
                        const char *name,
                        JsonNode *value)
{
  return g_hash_table_insert (self->attributes, (char *)name, (gpointer)value);
}

static gboolean
ephy_pvd_set_extra_attribute (EphyPvd *self,
                              const char *name,
                              JsonNode *value)
{
  return g_hash_table_insert (self->extra_attributes, (char *)name, (gpointer)value);
}

static gboolean
ephy_pvd_set_extra_attributes (EphyPvd *self,
                               JsonNode *node)
{
  const char *type;
  JsonObject *object;
  JsonObjectIter iter;
  const char *attribute_name;
  JsonNode *attribute_node;

  g_assert (EPHY_IS_PVD (self));

  type = json_node_type_name (node);

  if (strcmp (type, "JsonObject"))
    return FALSE; // TODO: perhaps add error message

  object = json_node_get_object (node);

  // iterate through the key-value pairs
  json_object_iter_init (&iter, object);
  while (json_object_iter_next (&iter, &attribute_name, &attribute_node)) {
    ephy_pvd_set_extra_attribute (self, attribute_name, attribute_node);
  }

  return TRUE;
}

gboolean
ephy_pvd_set_attributes (EphyPvd *self,
                         const char *json_str)
{
  JsonNode *root;
  JsonObject *object;
  JsonObjectIter iter;
  const char *type;
  const char *attribute_name;
  JsonNode *attribute_node;
  GError *error;

  g_assert (EPHY_IS_PVD (self));

  // if the attributes table contains already some values, destroy the table and create a new one
  if (g_hash_table_size (self->attributes)) {
    g_hash_table_destroy (self->attributes);
    self->attributes = g_hash_table_new (g_str_hash, g_str_equal);
  }

  // configure JSON parser
  error = NULL;
  json_parser_load_from_data (self->parser, json_str, strlen (json_str), &error);
  if (error) {
    return FALSE; //TODO: add error message (logging)
  }

  // retrieve root node and check its type
  root = json_parser_get_root (self->parser);
  type = json_node_type_name (root);
  if (strcmp (type, "JsonObject")) {
    // not a JSON object => exit
    return FALSE;
  }

  object = json_node_get_object (root);
  // iterate through the key-value pairs
  json_object_iter_init (&iter, object);
  while (json_object_iter_next (&iter, &attribute_name, &attribute_node)) {
    if (strcmp (attribute_name, "extraInfo") == 0)
      ephy_pvd_set_extra_attributes (self, attribute_node);

    ephy_pvd_set_attribute (self, attribute_name, attribute_node);
  }

  return TRUE;
}

void
ephy_pvd_set_name (EphyPvd* self, const char *name)
{
  g_assert (EPHY_IS_PVD (self));

  g_free (self->name);
  self->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_NAME]);
}

/*gboolean
ephy_pvd_add_attribute (EphyPvd *self,
                        const char *name,
                        JsonNode *value)
{
  return g_hash_table_insert (self->attributes, (char *) name, (gpointer) value);
}*/

JsonNode *
ephy_pvd_get_attribute (EphyPvd *self,
                        const char *name)
{
  return (JsonNode *) g_hash_table_lookup (self->attributes, name);
}

gboolean
ephy_pvd_set_attribute_int (EphyPvd *self,
                            const char *name,
                            gint64 value)
{
  JsonNode *attr;
  const char *type;

  attr = ephy_pvd_get_attribute (self, name);

  if (attr != NULL) {
    type = json_node_type_name (attr);

    if (strcmp (type, "Integer"))
      return FALSE;
  } else
    attr = json_node_new (JSON_NODE_VALUE);

  json_node_set_int (attr, value);

  ephy_pvd_set_attribute (self, name, attr);

  return TRUE;
}

gboolean
ephy_pvd_set_attribute_boolean (EphyPvd *self,
                                const char *name,
                                gboolean value)
{
  return TRUE;
}