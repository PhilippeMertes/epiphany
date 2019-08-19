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

#include "ephy-pvd-popover.h"

#include "ephy-pvd.h"
#include "ephy-pvd-row.h"
#include "ephy-pvd-manager.h"
#include "ephy-debug.h"
#include "ephy-shell.h"
#include "ephy-window.h"

#include <glib/gi18n.h>

struct _EphyPvdPopover {
    GtkPopover             parent_instance;

    GtkWidget             *toplevel_stack;
    GtkWidget             *pvd_list_box;
    GtkWidget             *current_pvd_label;
    GtkWidget             *default_pvd_label;

    EphyPvdManager        *manager;
};

G_DEFINE_TYPE (EphyPvdPopover, ephy_pvd_popover, GTK_TYPE_POPOVER)

static void
ephy_pvd_popover_list_box_row_activated_cb (EphyPvdPopover *self,
                                            GtkListBoxRow *row,
                                            GtkListBox *box)
{
  const char *pvd_name;
  EphyPvd *pvd;

  g_assert (EPHY_IS_PVD_POPOVER (self));
  g_assert (EPHY_IS_PVD_ROW (row));
  g_assert (GTK_IS_LIST_BOX (box));

  printf ("ephy_pvd_popover_list_box_row_activated_cb\n");

  // get row's PvD
  pvd = ephy_pvd_row_get_pvd ((EphyPvdRow *) row);
  pvd_name = ephy_pvd_get_name (pvd);

  // update default PvD
  ephy_pvd_manager_set_default_pvd (self->manager, pvd_name);
  printf ("default pvd = %s\n", ephy_pvd_manager_get_default_pvd (self->manager));
  ephy_pvd_popover_set_default_pvd (self, pvd_name);
}

static void
ephy_pvd_popover_finalize (GObject *object)
{
  G_OBJECT_CLASS (ephy_pvd_popover_parent_class)->finalize (object);
}

static void
ephy_pvd_popover_class_init (EphyPvdPopoverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ephy_pvd_popover_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/pvd-popover.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, toplevel_stack);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, pvd_list_box);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, current_pvd_label);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, default_pvd_label);
}

static void
ephy_pvd_popover_init (EphyPvdPopover *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->manager = ephy_shell_get_pvd_manager (ephy_shell_get_default ());

  gtk_list_box_bind_model (GTK_LIST_BOX (self->pvd_list_box),
                           G_LIST_MODEL (self->manager),
                           ephy_pvd_row_create,
                           self, NULL);

  if (g_list_model_get_n_items (G_LIST_MODEL (self->manager)) == 0)
    gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "empty-state");

  g_signal_connect_object (self->pvd_list_box, "row-activated",
                           G_CALLBACK (ephy_pvd_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
}

EphyPvdPopover *
ephy_pvd_popover_new (void)
{
  return g_object_new (EPHY_TYPE_PVD_POPOVER,
                       NULL);
}

const char *
ephy_pvd_popover_get_current_pvd (EphyPvdPopover *self)
{
  g_assert (EPHY_IS_PVD_POPOVER (self));

  return gtk_label_get_text (GTK_LABEL (self->current_pvd_label));
}

void
ephy_pvd_popover_set_current_pvd (EphyPvdPopover *self,
                                  const char     *pvd)
{
  const char *current_pvd;

  g_assert (EPHY_IS_PVD_POPOVER (self));

  // check if the new PvD is identical to the one currently shown in the label
  current_pvd = gtk_label_get_text (GTK_LABEL (self->current_pvd_label));
  if (g_strcmp0 (current_pvd, pvd) == 0)
    return;

  gtk_label_set_text (GTK_LABEL (self->current_pvd_label), pvd);
}

const char *
ephy_pvd_popover_get_default_pvd (EphyPvdPopover *self)
{
  g_assert (EPHY_IS_PVD_POPOVER (self));

  return gtk_label_get_text (GTK_LABEL (self->default_pvd_label));
}

void
ephy_pvd_popover_set_default_pvd (EphyPvdPopover *self,
                                  const char     *pvd)
{
  const char *default_pvd;

  g_assert (EPHY_IS_PVD_POPOVER (self));

  // check if the new pvd is identical to the one currently shown in the label
  default_pvd = gtk_label_get_text (GTK_LABEL (self->default_pvd_label));
  if (g_strcmp0 (default_pvd, pvd) == 0)
    return;

  gtk_label_set_text (GTK_LABEL (self->default_pvd_label), pvd);
}
