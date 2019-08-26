/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2016 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
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

#include "ephy-bookmarks-popover.h"

#include "ephy-bookmark.h"
#include "ephy-bookmark-row.h"
#include "ephy-bookmarks-manager.h"
#include "ephy-debug.h"
#include "ephy-header-bar.h"
#include "ephy-pvd-manager.h"
#include "ephy-pvd-popover.h"
#include "ephy-pvd-row.h"
#include "ephy-shell.h"
#include "ephy-window.h"

#include <glib/gi18n.h>

struct _EphyBookmarksPopover {
  GtkPopover             parent_instance;

  GtkWidget             *toplevel_stack;
  GtkWidget             *bookmarks_list_box;
  GtkWidget             *tags_list_box;
  GtkWidget             *tag_detail_list_box;
  GtkWidget             *tag_detail_back_button;
  GtkWidget             *tag_detail_label;
  char                  *tag_detail_tag;
  GtkWidget             *tag_detail_pvd_label; // shows PvD a tag is associated with
  GtkEventBox           *pvd_event_box; // allows to change the PvD a tag is associated with by clicking on the label
  GtkWidget             *tag_pvd_back_button; // back button for the "tag<->Pvd" widget
  GtkWidget             *tag_pvd_label; // label containing the tag name in the "tag<->PvD" widget
  GtkWidget             *tag_pvd_list_box; // lists all the currently known PvDs

  EphyBookmarksManager  *manager;
  EphyPvdManager        *pvd_manager; // manages currently known PvDs
};

G_DEFINE_TYPE (EphyBookmarksPopover, ephy_bookmarks_popover, GTK_TYPE_POPOVER)

#define EPHY_LIST_BOX_ROW_TYPE_BOOKMARK "bookmark"
#define EPHY_LIST_BOX_ROW_TYPE_TAG "tag"

static GtkWidget *create_bookmark_row (gpointer item, gpointer user_data);
static GtkWidget *create_tag_row (const char *tag);

static void
remove_bookmark_row_from_container (GtkContainer *container,
                                    const char   *url)
{
  GList *children;

  g_assert (GTK_IS_CONTAINER (container));

  children = gtk_container_get_children (container);
  for (GList *l = children; l && l->data; l = l->next) {
    const char *type = g_object_get_data (l->data, "type");

    if (g_strcmp0 (type, EPHY_LIST_BOX_ROW_TYPE_BOOKMARK) == 0 &&
        g_strcmp0 (ephy_bookmark_row_get_bookmark_url (l->data), url) == 0) {
      gtk_container_remove (container, l->data);
      break;
    }
  }
  g_list_free (children);
}

static void
ephy_bookmarks_popover_bookmark_tag_added_cb (EphyBookmarksPopover *self,
                                              EphyBookmark         *bookmark,
                                              const char           *tag,
                                              EphyBookmarksManager *manager)
{
  GtkWidget *tag_row;
  GList *children;
  GList *l;
  gboolean exists;
  const char *visible_stack_child;
  const char *title;
  const char *type;
  printf("bookmark_tag_added_cb\n");

  g_assert (EPHY_IS_BOOKMARK (bookmark));
  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));

  /* If the bookmark no longer has 0 tags, we remove it from the tags list box */
  if (g_sequence_get_length (ephy_bookmark_get_tags (bookmark)) == 1)
    remove_bookmark_row_from_container (GTK_CONTAINER (self->tags_list_box),
                                        ephy_bookmark_get_url (bookmark));

  /* If we are on the tag detail list box, then the user has toggled the state
   * of the tag widget multiple times. The first time the bookmark was removed
   * from the list box. Now we have to add it back. */
  visible_stack_child = gtk_stack_get_visible_child_name (GTK_STACK (self->toplevel_stack));
  if (g_strcmp0 (visible_stack_child, "tag_detail") == 0 &&
      g_strcmp0 (self->tag_detail_tag, tag) == 0) {
    GtkWidget *row;

    row = create_bookmark_row (bookmark, self);
    gtk_container_add (GTK_CONTAINER (self->tag_detail_list_box), row);
  }

  exists = FALSE;
  children = gtk_container_get_children (GTK_CONTAINER (self->tags_list_box));
  for (l = children; l != NULL; l = l->next) {
    title = g_object_get_data (G_OBJECT (l->data), "title");
    type = g_object_get_data (G_OBJECT (l->data), "type");

    if (g_strcmp0 (title, tag) == 0 &&
        g_strcmp0 (type, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0) {
      exists = TRUE;
      break;
    }
  }
  g_list_free (children);

  if (!exists) {
    tag_row = create_tag_row (tag);
    gtk_container_add (GTK_CONTAINER (self->tags_list_box), tag_row);
  }
}

static void
ephy_bookmarks_popover_bookmark_tag_removed_cb (EphyBookmarksPopover *self,
                                                EphyBookmark         *bookmark,
                                                const char           *tag,
                                                EphyBookmarksManager *manager)
{
  GtkWidget *row;
  GList *children;
  GList *l;
  const char *visible_stack_child;
  gboolean exists;
  gpointer title;

  g_assert (EPHY_IS_BOOKMARK (bookmark));
  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));

  /* If the bookmark has 0 tags after removing one, we add it to the tags list
   * box */
  if (g_sequence_is_empty (ephy_bookmark_get_tags (bookmark))) {
    exists = FALSE;
    children = gtk_container_get_children (GTK_CONTAINER (self->tags_list_box));
    for (l = children; l != NULL; l = l->next) {
      const char *type = g_object_get_data (l->data, "type");

      if (g_strcmp0 (type, EPHY_LIST_BOX_ROW_TYPE_BOOKMARK) == 0) {
        const char *url = ephy_bookmark_row_get_bookmark_url (l->data);

        if (g_strcmp0 (ephy_bookmark_get_url (bookmark), url) == 0) {
          exists = TRUE;
          break;
        }
      }
    }
    g_list_free (children);

    if (!exists) {
      row = create_bookmark_row (bookmark, self);
      gtk_container_add (GTK_CONTAINER (self->tags_list_box), row);
    }
  }

  /* If we are on the tag detail list box of the tag that was removed, we
   * remove the bookmark from it to reflect the changes. */
  visible_stack_child = gtk_stack_get_visible_child_name (GTK_STACK (self->toplevel_stack));
  if (g_strcmp0 (visible_stack_child, "tag_detail") == 0 &&
      g_strcmp0 (self->tag_detail_tag, tag) == 0) {
    remove_bookmark_row_from_container (GTK_CONTAINER (self->tag_detail_list_box),
                                        ephy_bookmark_get_url (bookmark));

    /* If we removed the tag's last bookmark, switch back to the tags list. */
    if (g_sequence_is_empty (ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, tag))) {
      GActionGroup *group;
      GAction *action;

      group = gtk_widget_get_action_group (GTK_WIDGET (self), "popover");
      action = g_action_map_lookup_action (G_ACTION_MAP (group), "tag-detail-back");
      g_action_activate (action, NULL);
    }
  }

  /* If the tag no longer contains bookmarks, remove it from the tags list */
  if (g_sequence_is_empty (ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, tag))) {
      children = gtk_container_get_children (GTK_CONTAINER (self->tags_list_box));
      for (l = children; l != NULL; l = l->next) {
        title = g_object_get_data (G_OBJECT (l->data), "title");
        if (g_strcmp0 (title, tag) == 0)
          gtk_container_remove (GTK_CONTAINER (self->tags_list_box), GTK_WIDGET (l->data));
      }
      g_list_free (children);
  }
}

static GtkWidget *
create_bookmark_row (gpointer item,
                     gpointer user_data)
{
  EphyBookmark *bookmark = EPHY_BOOKMARK (item);
  GtkWidget *row;

  row = ephy_bookmark_row_new (bookmark);
  g_object_set_data_full (G_OBJECT (row), "type",
                          g_strdup (EPHY_LIST_BOX_ROW_TYPE_BOOKMARK),
                          (GDestroyNotify)g_free);
  g_object_set_data_full (G_OBJECT (row), "title",
                          g_strdup (ephy_bookmark_get_title (bookmark)),
                          (GDestroyNotify)g_free);

  return row;
}

static GtkWidget *
create_tag_row (const char *tag)
{
  GtkWidget *row;
  GtkWidget *box;
  GtkWidget *image;
  GtkWidget *label;

  row = gtk_list_box_row_new ();
  g_object_set_data_full (G_OBJECT (row), "type",
                          g_strdup (EPHY_LIST_BOX_ROW_TYPE_TAG),
                          (GDestroyNotify)g_free);
  g_object_set_data_full (G_OBJECT (row), "title",
                          g_strdup (tag),
                          (GDestroyNotify)g_free);
  g_object_set (G_OBJECT (row), "height-request", 40, NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (box, GTK_ALIGN_START);

  if (g_strcmp0 (tag, EPHY_BOOKMARKS_FAVORITES_TAG) == 0) {
    image = gtk_image_new_from_icon_name ("emblem-favorite-symbolic", GTK_ICON_SIZE_MENU);
    label = gtk_label_new (_(EPHY_BOOKMARKS_FAVORITES_TAG));
  } else {
    image = gtk_image_new_from_icon_name ("ephy-bookmark-tag-symbolic", GTK_ICON_SIZE_MENU);
    label = gtk_label_new (tag);
  }

  gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (row), box);
  gtk_widget_show_all (row);

  return row;
}

static void
ephy_bookmarks_popover_bookmark_added_cb (EphyBookmarksPopover *self,
                                          EphyBookmark         *bookmark,
                                          EphyBookmarksManager *manager)
{
  GtkWidget *row;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));
  g_assert (EPHY_IS_BOOKMARK (bookmark));
  g_assert (EPHY_IS_BOOKMARKS_MANAGER (manager));

  if (g_sequence_is_empty (ephy_bookmark_get_tags (bookmark))) {
    row = create_bookmark_row (bookmark, self);
    gtk_container_add (GTK_CONTAINER (self->tags_list_box), row);
  }

  if (strcmp (gtk_stack_get_visible_child_name (GTK_STACK (self->toplevel_stack)), "empty-state") == 0)
    gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "default");
}

static void
ephy_bookmarks_popover_bookmark_removed_cb (EphyBookmarksPopover *self,
                                            EphyBookmark         *bookmark,
                                            EphyBookmarksManager *manager)
{
  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));
  g_assert (EPHY_IS_BOOKMARK (bookmark));
  g_assert (EPHY_IS_BOOKMARKS_MANAGER (manager));

  remove_bookmark_row_from_container (GTK_CONTAINER (self->tags_list_box),
                                      ephy_bookmark_get_url (bookmark));
  remove_bookmark_row_from_container (GTK_CONTAINER (self->tag_detail_list_box),
                                      ephy_bookmark_get_url (bookmark));

  if (g_list_model_get_n_items (G_LIST_MODEL (self->manager)) == 0) {
    gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "empty-state");
  } else if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (self->toplevel_stack)), "tag_detail") == 0 &&
             g_sequence_is_empty (ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, self->tag_detail_tag))) {
    /* If we removed the tag's last bookmark, switch back to the tags list. */
    GActionGroup *group;
    GAction *action;

    group = gtk_widget_get_action_group (GTK_WIDGET (self), "popover");
    action = g_action_map_lookup_action (G_ACTION_MAP (group), "tag-detail-back");
    g_action_activate (action, NULL);
  }
}

static void
ephy_bookmarks_popover_tag_created_cb (EphyBookmarksPopover *self,
                                       const char           *tag,
                                       EphyBookmarksManager *manager)
{
  GtkWidget *tag_row;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));
  g_assert (tag != NULL);
  g_assert (EPHY_IS_BOOKMARKS_MANAGER (manager));

  tag_row = create_tag_row (tag);
  gtk_container_add (GTK_CONTAINER (self->tags_list_box), tag_row);
}

static void
ephy_bookmarks_popover_tag_deleted_cb (EphyBookmarksPopover *self,
                                       const char           *tag,
                                       int                   position,
                                       EphyBookmarksManager *manager)
{
  GtkListBoxRow *row;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));
  g_assert (EPHY_IS_BOOKMARKS_MANAGER (manager));

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (self->tags_list_box), position);
  gtk_container_remove (GTK_CONTAINER (self->tags_list_box),
                        GTK_WIDGET (row));

  if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (self->toplevel_stack)), "tag_detail") == 0 &&
      g_strcmp0 (self->tag_detail_tag, tag) == 0) {
    GActionGroup *group;
    GAction *action;

    group = gtk_widget_get_action_group (GTK_WIDGET (self), "popover");
    action = g_action_map_lookup_action (G_ACTION_MAP (group), "tag-detail-back");
    g_action_activate (action, NULL);
  }
}

static int
tags_list_box_sort_func (GtkListBoxRow *row1, GtkListBoxRow *row2)
{
  const char *type1;
  const char *type2;
  const char *title1;
  const char *title2;

  g_assert (GTK_IS_LIST_BOX_ROW (row1));
  g_assert (GTK_IS_LIST_BOX_ROW (row2));

  type1 = g_object_get_data (G_OBJECT (row1), "type");
  type2 = g_object_get_data (G_OBJECT (row2), "type");

  title1 = g_object_get_data (G_OBJECT (row1), "title");
  title2 = g_object_get_data (G_OBJECT (row2), "title");

  if (g_strcmp0 (type1, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0
      && g_strcmp0 (type2, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0)
      return ephy_bookmark_tags_compare (title1, title2);

  if (g_strcmp0 (type1, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0)
    return -1;
  if (g_strcmp0 (type2, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0)
    return 1;

  return g_strcmp0 (title1, title2);
}

static void
ephy_bookmarks_popover_actions_tag_detail_back (GSimpleAction *action,
                                                GVariant      *value,
                                                gpointer       user_data)
{
  EphyBookmarksPopover *self = user_data;
  GList *children;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));

  gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack),
                                    "default");

  children = gtk_container_get_children (GTK_CONTAINER (self->tag_detail_list_box));
  for (GList *l = children; l != NULL; l = l->next)
    gtk_container_remove (GTK_CONTAINER (self->tag_detail_list_box), l->data);
  g_list_free (children);
}

/**
 * ephy_bookmarks_popover_actions_tag_pvd_back:
 * @action: a #GSimpleAction
 * @value: a #GVariant
 * @user_data: gpointer to the #EphyBookmarksPopover
 *
 * Sets the visible portion of the widget to "tag_detail".
 **/
static void
ephy_bookmarks_popover_actions_tag_pvd_back (GSimpleAction *action,
                                             GVariant      *value,
                                             gpointer       user_data)
{
  EphyBookmarksPopover *self = user_data;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));

  gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack),
                                    "tag_detail");
}

/**
 * ephy_bookmarks_popover_show_tag_pvd:
 * @self: an #EphyBookmarksPopover
 *
 * Sets the visible portion of the widget to "tag_pvd",
 * allowing to choose the PvD a tag will be associated with.
 **/
static void
ephy_bookmarks_popover_show_tag_pvd (EphyBookmarksPopover *self)
{
  gtk_label_set_label (GTK_LABEL (self->tag_pvd_label), self->tag_detail_tag);

  gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "tag_pvd");
}

static void
ephy_bookmarks_popover_show_tag_detail (EphyBookmarksPopover *self,
                                        const char           *tag)
{
  GSequence *bookmarks;
  GSequenceIter *iter;
  const char *pvd_name;
  EphyPvd *pvd;

  bookmarks = ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, tag);
  for (iter = g_sequence_get_begin_iter (bookmarks);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter)) {
    EphyBookmark *bookmark = g_sequence_get (iter);
    GtkWidget *row;

    row = create_bookmark_row (bookmark, self);
    gtk_container_add (GTK_CONTAINER (self->tag_detail_list_box), row);
  }

  if (strcmp (tag, EPHY_BOOKMARKS_FAVORITES_TAG) == 0)
    gtk_label_set_label (GTK_LABEL (self->tag_detail_label), _(EPHY_BOOKMARKS_FAVORITES_TAG));
  else
    gtk_label_set_label (GTK_LABEL (self->tag_detail_label), tag);

  pvd_name = ephy_bookmarks_manager_get_pvd_from_tag (self->manager, tag);

  // make tag detail widget visible
  gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "tag_detail");

  if (self->tag_detail_tag != NULL)
    g_free (self->tag_detail_tag);
  self->tag_detail_tag = g_strdup (tag);

  /* check if extra attributes are provided for the PvD
   * If yes, use the corresponding name instead of the FQDN.*/
  pvd = ephy_pvd_manager_get_pvd (self->pvd_manager, pvd_name);
  if (pvd && ephy_pvd_has_extra_attributes (pvd))
    pvd_name = ephy_pvd_get_extra_attribute_name (pvd);

  // show up the name of the PvD the tag is bound to
  gtk_label_set_text (GTK_LABEL (self->tag_detail_pvd_label), pvd_name);

  g_sequence_free (bookmarks);
}

static void
ephy_bookmarks_popover_list_box_row_activated_cb (EphyBookmarksPopover   *self,
                                                  GtkListBoxRow          *row,
                                                  GtkListBox             *box)
{
  const char *type;
  const char *tag;
  const char *pvd;

  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row));
  g_assert (GTK_IS_LIST_BOX (box));

  type = g_object_get_data (G_OBJECT (row), "type");
  if (g_strcmp0 (type, EPHY_LIST_BOX_ROW_TYPE_BOOKMARK) == 0) {
    GtkWidget *window;
    GActionGroup *action_group;
    GAction *action;
    const char *url, *pvd;
    GtkWidget *header_bar;
    EphyPvdPopover *pvd_popover;

    url = ephy_bookmark_row_get_bookmark_url (EPHY_BOOKMARK_ROW (row));
    window = gtk_widget_get_ancestor (GTK_WIDGET (self), EPHY_TYPE_WINDOW);
    g_assert (EPHY_IS_WINDOW (window));

    // bind network process to PvD depending on tags
    pvd = ephy_shell_bind_to_pvd_on_url (ephy_shell_get_default (), url);
    header_bar = ephy_window_get_header_bar (EPHY_WINDOW (window));
    pvd_popover = ephy_header_bar_get_pvd_popover (EPHY_HEADER_BAR (header_bar));
    if (pvd)
      ephy_pvd_popover_set_current_pvd (pvd_popover, pvd);

    // load website in WebView
    action_group = gtk_widget_get_action_group (window, "win");
    g_assert (action_group != NULL);
    action = g_action_map_lookup_action (G_ACTION_MAP (action_group), "open-bookmark");
    g_assert (action != NULL);

    g_action_activate (action, g_variant_new_string (url));
  } else if (g_strcmp0 (type, EPHY_LIST_BOX_ROW_TYPE_TAG) == 0) {
    tag = g_object_get_data (G_OBJECT (row), "title");
    ephy_bookmarks_popover_show_tag_detail (self, tag);
  } else { // row containing a PvD => bind the corresponding tag to the PvD
    GList *tag_detail_list_children;

    pvd = g_strdup (ephy_pvd_row_get_pvd_name (EPHY_PVD_ROW (row)));
    ephy_bookmarks_manager_bind_tag_to_pvd (self->manager, g_strdup (self->tag_detail_tag), pvd);
    // remove all the bookmarks inside the list of the tag_detail widget
    tag_detail_list_children = gtk_container_get_children (GTK_CONTAINER (self->tag_detail_list_box));
    for (GList *l = tag_detail_list_children; l != NULL; l = l->next)
      gtk_container_remove (GTK_CONTAINER (self->tag_detail_list_box), l->data);
    g_list_free (tag_detail_list_children);
    // show tag detail widget
    ephy_bookmarks_popover_show_tag_detail (self, g_strdup (self->tag_detail_tag));
  }
}

/**
 * ephy_bookmarks_popover_pvd_box_button_press_cb:
 * @self: an #EphyBookmarksPopover
 *
 * Function called when clicking on the PvD associated with a tag.
 * Will make the "tag_pvd" box visible, in order to change the association.
 **/
static void
ephy_bookmarks_popover_pvd_box_button_press_cb (EphyBookmarksPopover *self)
{
  g_assert (EPHY_IS_BOOKMARKS_POPOVER (self));

  ephy_bookmarks_popover_show_tag_pvd (self);
}

static void
ephy_bookmarks_popover_finalize (GObject *object)
{
  EphyBookmarksPopover *self = EPHY_BOOKMARKS_POPOVER (object);

  g_free (self->tag_detail_tag);

  G_OBJECT_CLASS (ephy_bookmarks_popover_parent_class)->finalize (object);
}

static void
ephy_bookmarks_popover_class_init (EphyBookmarksPopoverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ephy_bookmarks_popover_finalize;

  // bind template attributes to object attributes
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/bookmarks-popover.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, toplevel_stack);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, bookmarks_list_box);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tags_list_box);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_detail_list_box);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_detail_back_button);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_detail_label);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_detail_pvd_label);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, pvd_event_box);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_pvd_back_button);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_pvd_label);
  gtk_widget_class_bind_template_child (widget_class, EphyBookmarksPopover, tag_pvd_list_box);
}

static const GActionEntry entries[] = {
  { "tag-detail-back", ephy_bookmarks_popover_actions_tag_detail_back },
  { "tag-pvd-back",    ephy_bookmarks_popover_actions_tag_pvd_back }
};

static void
ephy_bookmarks_popover_init (EphyBookmarksPopover *self)
{
  GSequence *tags;
  GSequence *bookmarks;
  GSequenceIter *iter;
  GSimpleActionGroup *group;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->manager = ephy_shell_get_bookmarks_manager (ephy_shell_get_default ());
  self->pvd_manager = ephy_shell_get_pvd_manager (ephy_shell_get_default ()); // get singleton PvD manager from shell

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), entries,
                                   G_N_ELEMENTS (entries), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "popover",
                                  G_ACTION_GROUP (group));
  g_object_unref (group);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->bookmarks_list_box),
                           G_LIST_MODEL (self->manager),
                           create_bookmark_row,
                           self, NULL);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->tag_pvd_list_box),
                           G_LIST_MODEL (self->pvd_manager),
                           ephy_pvd_row_create,
                           self, NULL);

  if (g_list_model_get_n_items (G_LIST_MODEL (self->manager)) == 0)
    gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "empty-state");

  gtk_list_box_set_sort_func (GTK_LIST_BOX (self->tags_list_box),
                              (GtkListBoxSortFunc)tags_list_box_sort_func,
                              NULL, NULL);
  gtk_list_box_set_sort_func (GTK_LIST_BOX (self->tag_detail_list_box),
                              (GtkListBoxSortFunc)tags_list_box_sort_func,
                              NULL, NULL);

  tags = ephy_bookmarks_manager_get_tags (self->manager);
  for (iter = g_sequence_get_begin_iter (tags);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter)) {
    const char *tag = g_sequence_get (iter);
    GtkWidget *tag_row;

    if (!g_sequence_is_empty (ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, tag))) {
      tag_row = create_tag_row (tag);
      gtk_container_add (GTK_CONTAINER (self->tags_list_box), tag_row);
    }
  }

  bookmarks = ephy_bookmarks_manager_get_bookmarks_with_tag (self->manager, NULL);
  for (iter = g_sequence_get_begin_iter (bookmarks);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter)) {
    EphyBookmark *bookmark = g_sequence_get (iter);
    GtkWidget *bookmark_row;

    bookmark_row = create_bookmark_row (bookmark, self);
    gtk_widget_show_all (bookmark_row);
    gtk_container_add (GTK_CONTAINER (self->tags_list_box), bookmark_row);
  }

  g_signal_connect_object (self->manager, "bookmark-added",
                           G_CALLBACK (ephy_bookmarks_popover_bookmark_added_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager, "bookmark-removed",
                           G_CALLBACK (ephy_bookmarks_popover_bookmark_removed_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager, "tag-created",
                           G_CALLBACK (ephy_bookmarks_popover_tag_created_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager, "tag-deleted",
                           G_CALLBACK (ephy_bookmarks_popover_tag_deleted_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager, "bookmark-tag-added",
                           G_CALLBACK (ephy_bookmarks_popover_bookmark_tag_added_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->manager, "bookmark-tag-removed",
                           G_CALLBACK (ephy_bookmarks_popover_bookmark_tag_removed_cb),
                           self, G_CONNECT_SWAPPED);

  g_signal_connect_object (self->bookmarks_list_box, "row-activated",
                           G_CALLBACK (ephy_bookmarks_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->tags_list_box, "row-activated",
                           G_CALLBACK (ephy_bookmarks_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
  // set signal emitted when clicking on a row of the PvD list
  g_signal_connect_object (self->tag_detail_list_box, "row-activated",
                           G_CALLBACK (ephy_bookmarks_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
  // set signal emitted when pressing on the label specifying PvD associated with a tag
  g_signal_connect_object (self->pvd_event_box, "button-press-event",
                           G_CALLBACK (ephy_bookmarks_popover_pvd_box_button_press_cb),
                           self, G_CONNECT_SWAPPED);
  // set signal emitted when pressing on a row in the list box used to change the PvD associated with a tag
  g_signal_connect_object (self->tag_pvd_list_box, "row-activated",
                           G_CALLBACK (ephy_bookmarks_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
}

EphyBookmarksPopover *
ephy_bookmarks_popover_new (void)
{
  return g_object_new (EPHY_TYPE_BOOKMARKS_POPOVER,
                       NULL);
}
