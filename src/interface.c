/*
 * GIMP HEIF loader / write plugin.
 * Copyright (c) 2018 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of gimp-libheif-plugin.
 *
 * gimp-libheif-plugin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gimp-libheif-plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gimp-libheif-plugin.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <assert.h>

#include "main.h"
#include "interface.h"

#include "plugin-intl.h"


#define MAX_THUMBNAIL_SIZE 320


struct HeifImage
{
  uint32_t ID;
  char caption[100]; // image text (filled with resolution description)
  struct heif_image* thumbnail;
  int width, height;
};


gboolean load_thumbnails(struct heif_context* heif,
                         struct HeifImage* images)
{
  int i;

  int numImages = heif_context_get_number_of_top_level_images(heif);

  // get list of all (top level) image IDs

  uint32_t* IDs = (uint32_t*)alloca(numImages * sizeof(uint32_t));
  heif_context_get_list_of_top_level_image_IDs(heif, IDs, numImages);


  // --- Load a thumbnail for each image.

  for (i=0; i<numImages; i++) {

    images[i].ID = IDs[i];
    images[i].caption[0] = 0;
    images[i].thumbnail = NULL;

    // get image handle

    struct heif_image_handle* handle;
    struct heif_error err = heif_context_get_image_handle(heif, IDs[i], &handle);
    if (err.code) {
      gimp_message(err.message);
      continue;
    }


    // generate image caption

    int width = heif_image_handle_get_width(handle);
    int height = heif_image_handle_get_height(handle);

    if (heif_image_handle_is_primary_image(handle)) {
      sprintf(images[i].caption, "%dx%d (%s)", width,height, _("primary"));
    }
    else {
      sprintf(images[i].caption, "%dx%d", width,height);
    }


    // get handle to thumbnail image
    // if there is no thumbnail image, just the the image itself (will be scaled down later)

    struct heif_image_handle* thumbnail_handle;
    heif_item_id thumbnail_ID;

    int nThumbnails = heif_image_handle_get_list_of_thumbnail_IDs(handle, &thumbnail_ID, 1);

    if (nThumbnails > 0) {
      err = heif_image_handle_get_thumbnail(handle, thumbnail_ID, &thumbnail_handle);
      if (err.code) {
        gimp_message(err.message);
        continue;
      }
    }
    else {
      err = heif_context_get_image_handle(heif, IDs[i], &thumbnail_handle);
      if (err.code) {
        gimp_message(err.message);
        continue;
      }
    }


    // decode the thumbnail image

    struct heif_image* thumbnail_img;
    err = heif_decode_image(thumbnail_handle,
                            &thumbnail_img,
                            heif_colorspace_RGB, heif_chroma_interleaved_24bit,
                            NULL);
    if (err.code) {
      gimp_message(err.message);
      continue;
    }


    // if thumbnail image size exceeds the maximum, scale it down

    int thumbnail_width = heif_image_handle_get_width(thumbnail_handle);
    int thumbnail_height = heif_image_handle_get_height(thumbnail_handle);

    if (thumbnail_width > MAX_THUMBNAIL_SIZE ||
        thumbnail_height > MAX_THUMBNAIL_SIZE) {

      // compute scaling factor to fit into a max sized box

      float factor_h = thumbnail_width  / (float)MAX_THUMBNAIL_SIZE;
      float factor_v = thumbnail_height / (float)MAX_THUMBNAIL_SIZE;

      int new_width, new_height;

      if (factor_v > factor_h) {
        new_height = MAX_THUMBNAIL_SIZE;
        new_width  = thumbnail_width / factor_v;
      }
      else {
        new_height = thumbnail_height / factor_h;
        new_width  = MAX_THUMBNAIL_SIZE;
      }


      // scale the image

      struct heif_image* scaled_img = NULL;

      struct heif_error err = heif_image_scale_image(thumbnail_img,
                                                     &scaled_img,
                                                     new_width, new_height,
                                                     NULL);
      if (err.code) {
        gimp_message(err.message);
        continue;
      }


      // release the old image and only keep the scaled down version

      heif_image_release(thumbnail_img);
      thumbnail_img = scaled_img;

      thumbnail_width = new_width;
      thumbnail_height = new_height;
    }

    heif_image_handle_release(thumbnail_handle);
    heif_image_handle_release(handle);


    // remember the HEIF thumbnail image (we need it for the GdkPixbuf)

    images[i].thumbnail = thumbnail_img;

    images[i].width = thumbnail_width;
    images[i].height = thumbnail_height;
  }

  return TRUE;
}


gboolean dialog(struct heif_context* heif,
                uint32_t* selected_image)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  gboolean   run = FALSE;

  int i;


  int numImages = heif_context_get_number_of_top_level_images(heif);

  struct HeifImage* heif_images = (struct HeifImage*)alloca(numImages * sizeof(struct HeifImage));
  gboolean success = load_thumbnails(heif, heif_images);
  if (!success) {
    return FALSE;
  }

  dlg = gimp_dialog_new (_("Load HEIF image content"), PLUGIN_NAME,
                         NULL, 0,
                         NULL, 0, //gimp_standard_help_func, "plug-in-template", // TODO
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,
			 NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  frame = gimp_frame_new (_("Select image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);


  // prepare list store with all thumbnails and caption

  GtkListStore* liststore;
  GtkTreeIter   iter;

  liststore = gtk_list_store_new(2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  for (i=0; i<numImages; i++) {
    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter, 0, heif_images[i].caption, -1);

    int stride;
    const uint8_t* data = heif_image_get_plane_readonly(heif_images[i].thumbnail,
                                                        heif_channel_interleaved,
                                                        &stride);

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (data,
                                                  GDK_COLORSPACE_RGB,
                                                  FALSE,
                                                  8,
                                                  heif_images[i].width,
                                                  heif_images[i].height,
                                                  stride,
                                                  NULL,
                                                  NULL);

    gtk_list_store_set(liststore, &iter, 1, pixbuf, -1);
  }


  GtkWidget* iconview = gtk_icon_view_new();
  gtk_icon_view_set_model((GtkIconView*) iconview, (GtkTreeModel*) liststore);
  gtk_icon_view_set_text_column((GtkIconView*) iconview, 0);
  gtk_icon_view_set_pixbuf_column((GtkIconView*) iconview, 1);
  gtk_container_add (GTK_CONTAINER (frame), iconview);
  gtk_widget_show(iconview);


  // pre-select the primary image

  int selected_idx = -1;
  for (i=0; i<numImages; i++) {
    if (heif_images[i].ID == *selected_image) {
      selected_idx = i;
      break;
    }
  }

  if (selected_idx != -1) {
    GtkTreePath *path = gtk_tree_path_new_from_indices(selected_idx, -1);
    gtk_icon_view_select_path((GtkIconView*)iconview, path);
    gtk_tree_path_free(path);
  }


  gtk_widget_show (main_vbox);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run) {
    GList* selected_items = gtk_icon_view_get_selected_items((GtkIconView*) iconview);

    if (selected_items) {
      GtkTreePath* path = (GtkTreePath*)(selected_items->data);
      gint* indices = gtk_tree_path_get_indices(path);

      *selected_image = heif_images[indices[0]].ID;

      g_list_free_full(selected_items, (GDestroyNotify) gtk_tree_path_free);
    }
  }

  gtk_widget_destroy (dlg);


  // release thumbnail images

  for (i=0 ; i<numImages ; i++) {
    heif_image_release(heif_images[i].thumbnail);
  }

  return run;
}



static void on_lossless_button_toggled (GtkToggleButton *source, gpointer user_data) {
  GtkWidget* slider = GTK_WIDGET(user_data);
  gboolean lossless = gtk_toggle_button_get_active (source);

  gtk_widget_set_sensitive (slider, !lossless);
}


gboolean save_dialog(struct save_parameters* params)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *lossless_button;
  GtkWidget *quality_slider;
  gboolean   run = FALSE;

  //  int i;


  dlg = gimp_dialog_new (_("Save HEIF image"), PLUGIN_NAME,
                         NULL, 0,
                         NULL, 0, //gimp_standard_help_func, "plug-in-template", // TODO
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,
			 NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  lossless_button = gtk_check_button_new_with_label(_("Lossless"));
  gtk_box_pack_start (GTK_BOX(main_vbox), lossless_button, FALSE, FALSE, 0);


  hbox = gtk_hbox_new (FALSE, 10);
  //gtk_container_border_width (GTK_CONTAINER (box2), 10);
  label = gtk_label_new (_("Quality:"));
  quality_slider = gtk_hscale_new_with_range (0, 100, 5);
  gtk_scale_set_value_pos (GTK_SCALE(quality_slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(hbox), quality_slider, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(main_vbox), hbox, TRUE, TRUE, 0);


  gtk_range_set_value (GTK_RANGE(quality_slider), params->quality);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(lossless_button), params->lossless);
  gtk_widget_set_sensitive (quality_slider, !params->lossless);

  g_signal_connect (lossless_button, "toggled",
                    G_CALLBACK (on_lossless_button_toggled),
                    quality_slider);


  gtk_widget_show_all(dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run) {
    params->quality = gtk_range_get_value(GTK_RANGE(quality_slider));
    params->lossless = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(lossless_button));
  }

  gtk_widget_destroy (dlg);

  return run;
}
