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
#include "ephy-pvd-manager.h"

#include <libpvd.h>
#include <json-glib/json-glib.h>

struct _EphyPvdManager {
    GObject     parent_instance;

    GSequence  *pvd_list; // list of currently known PvDs
    GHashTable *pvd_name_to_object; // PvD FQDN to EphyPvd association

    const char *default_pvd; // PvD to which WebKit's network process should be bound by default
};

static void list_model_iface_init (GListModelInterface *iface);
static void ephy_pvd_manager_retrieve_pvd_attributes (EphyPvdManager *self,
                                                      EphyPvd *pvd,
                                                      t_pvd_connection *conn);

// define class to be an instance of GListModel
G_DEFINE_TYPE_WITH_CODE (EphyPvdManager, ephy_pvd_manager, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
ephy_pvd_manager_finalize (GObject *object)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (object);

  g_sequence_free (self->pvd_list);
  g_hash_table_destroy (self->pvd_name_to_object);
  g_free ((char *)self->default_pvd);

  G_OBJECT_CLASS (ephy_pvd_manager_parent_class)->finalize (object);
}

static void
ephy_pvd_manager_class_init (EphyPvdManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ephy_pvd_manager_finalize;
}

static void
ephy_pvd_manager_init (EphyPvdManager *self)
{
  EphyPvd *pvd;

  self->pvd_list = g_sequence_new (NULL);
  self->pvd_name_to_object = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->default_pvd = NULL;

  // collect PvD names from pvdd
  t_pvd_connection *conn = pvd_connect (-1);
  t_pvd_list *pvd_list = g_malloc (sizeof (t_pvd_list));

  if (pvd_get_pvd_list_sync (conn, pvd_list))
    goto out;

  // store PvDs in sequence
  for (int i = 0; i < pvd_list->npvd; ++i) {
    pvd = ephy_pvd_new (g_strdup (pvd_list->pvdnames[i]));
    ephy_pvd_manager_retrieve_pvd_attributes(self, pvd, conn);
    g_hash_table_insert (self->pvd_name_to_object, g_strdup (pvd_list->pvdnames[i]), pvd);
    g_sequence_append (self->pvd_list, pvd);
    g_free (pvd_list->pvdnames[i]);
  }

out:
  pvd_disconnect (conn);
  g_free(pvd_list);
}

EphyPvdManager *
ephy_pvd_manager_new (void)
{
  return EPHY_PVD_MANAGER (g_object_new (EPHY_TYPE_PVD_MANAGER, NULL));
}

/** GListModel specific functions **/

static GType
ephy_pvd_manager_list_model_get_item_type (GListModel *model)
{
  return EPHY_TYPE_PVD;
}

static guint
ephy_pvd_manager_list_model_get_n_items (GListModel *model)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (model);

  return g_sequence_get_length (self->pvd_list);
}

static gpointer
ephy_pvd_manager_list_model_get_item (GListModel *model,
                                      guint       position)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (model);
  GSequenceIter *iter;

  iter = g_sequence_get_iter_at_pos (self->pvd_list, position);

  return g_object_ref (g_sequence_get (iter));
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = ephy_pvd_manager_list_model_get_item_type;
  iface->get_n_items = ephy_pvd_manager_list_model_get_n_items;
  iface->get_item = ephy_pvd_manager_list_model_get_item;
}

/**
 * ephy_pvd_manager_retrieve_pvd_attributes:
 * @self: an #EphyPvdManager
 * @pvd: an #EphyPvd
 *
 * Retrieves the attributes of a PvD from the pvdd daemon and
 * stores them inside the #EphyPvd object.
 **/
static void
ephy_pvd_manager_retrieve_pvd_attributes (EphyPvdManager *self,
                                          EphyPvd *pvd,
                                          t_pvd_connection *conn)
{
  const char *pvd_name = ephy_pvd_get_name (pvd);
  char *attributes;

  // check if name is specified
  if (!pvd_name) {
    g_warning ("Cannot retrieve PvD attributes if no FQDN is specified.");
    return;
  }

  // retrieve PvD attributes from pvdd
  if (pvd_get_attributes_sync (conn, (char *) pvd_name, &attributes)) {
    g_warning ("Unable to retrieve attributes from the pvdd daemon.\n");
    return;
  }

  ephy_pvd_set_attributes (pvd, attributes);
}

/**
 * ephy_pvd_manager_get_pvd:
 * @self: an #EphyPvdManager
 * @pvd_name: constant string (FQDN)
 *
 * Returns the #EphyPvD object corresponding to a PvD's FQDN.
 *
 * Return value: the corresponding #EphyPvd
 **/
EphyPvd *
ephy_pvd_manager_get_pvd (EphyPvdManager *self,
                          const char     *pvd_name)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  return g_hash_table_lookup (self->pvd_name_to_object, pvd_name);
}

/**
 * ephy_pvd_manager_is_advertised:
 * @self: an #EphyPvdManager
 * @pvd_name: constant string (FQDN)
 *
 * Checks whether a PvD has been advertised to the system.
 * This means, that it has been retrieved through pvdd and is,
 * thus, contained in the list maintained by the manager.
 *
 * Return value: boolean indicating whether (TRUE) or not (FALSE)
 * the PvD has been advertised.
 **/
gboolean
ephy_pvd_manager_is_advertised (EphyPvdManager *self,
                                const char *pvd_name)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  return ephy_pvd_manager_get_pvd (self, pvd_name) ? TRUE : FALSE;
}

/**
 * ephy_pvd_manager_get_default_pvd:
 * @self: an #EphyPvdManager
 *
 * Returns the default PvD currently set.
 *
 * Return value: the PvD's FQDN (constant string)
 **/
const char *
ephy_pvd_manager_get_default_pvd (EphyPvdManager *self)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  return self->default_pvd;
}

/**
 * ephy_pvd_manager_set_default_pvd:
 * @self: an #EphyPvdManager
 * @pvd_name: constant string (FQDN)
 *
 * Sets the default PvD.
 **/
void
ephy_pvd_manager_set_default_pvd (EphyPvdManager *self,
                                  const char *pvd_name)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  g_free ((char *)self->default_pvd);
  self->default_pvd = g_strdup (pvd_name);
}