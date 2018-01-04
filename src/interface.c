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
 *                                                                                                                                                                                                                  * You should have received a copy of the GNU General Public License
 * along with gimp-libheif-plugin.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"

#include "plugin-intl.h"


/*  Constants  */

#define SCALE_WIDTH        180
#define SPIN_BUTTON_WIDTH   75
#define RANDOM_SEED_WIDTH  100


/*  Local function prototypes  */

#if 0
static gboolean   dialog_image_constraint_func (gint32    image_id,
                                                gpointer  data);
#endif


/*  Local variables  */

#if 0
static PlugInUIVals *ui_state = NULL;
#endif


/*  Public functions  */

gboolean
dialog (int num_images,
        int primary_image,
        UIResult* ui_result,
        struct heif_context* heif)

/*
image_ID,
	GimpDrawable       *drawable,
	PlugInVals         *vals,
	PlugInImageVals    *image_vals,
	PlugInDrawableVals *drawable_vals,
	PlugInUIVals       *ui_vals)
*/
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  gboolean   run = FALSE;
#if 0
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *coordinates;
  GtkWidget *combo;
  GtkObject *adj;
  gint       row;
  GimpUnit   unit;
  gdouble    xres, yres;
#endif

  int i;

  struct heif_error err;

  //ui_state = ui_vals;

  gimp_ui_init (PLUGIN_NAME, TRUE);

  dlg = gimp_dialog_new (_("Load HEIF image content"), PLUGIN_NAME,
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-template", // TODO

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  /*  gimp_scale_entry_new() examples  */

#if 0
  frame = gimp_frame_new (_("Select image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);


  GtkWidget* combobox = gtk_combo_box_text_new();

  int i;
  for (i = 0; i < num_images; i++)
    {
      gchar some_data[20];
      sprintf(some_data,"image %d%s",i+1,
              i == primary_image ? " (primary)" : "");
      gtk_combo_box_text_append_text((GtkComboBoxText*) combobox, some_data);
    }

  gtk_combo_box_set_active ((GtkComboBox*) combobox, primary_image);

  gtk_container_add (GTK_CONTAINER (frame), combobox);
  gtk_widget_show(combobox);
#endif


  frame = gimp_frame_new (_("Select image"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  GtkListStore* liststore;
  GtkTreeIter   iter;

  liststore = gtk_list_store_new(2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
  int numImages = heif_context_get_number_of_images(heif);
  for (i=0; i<numImages; i++) {

    struct heif_image_handle* handle;
    err = heif_context_get_image_handle(heif, i, &handle);
    if (err.code) {
      // TODO(farindk): Handle error.
      continue;
    }

    char buf[100];
    int width,height;
    heif_image_handle_get_resolution(heif,handle,&width,&height);
    sprintf(buf,"%dx%d%s", width,height,
            heif_image_handle_is_primary_image(heif,handle) ? " (primary)":"");

    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter, 0, buf, -1);

    if (heif_image_handle_get_number_of_thumbnails(heif, handle)) {
      struct heif_image_handle* thumbnail_handle;
      heif_image_handle_get_thumbnail(heif, handle, 0, &thumbnail_handle);

      struct heif_image* thumbnail_img;
      err = heif_decode_image(heif, thumbnail_handle, &thumbnail_img,
                              heif_colorspace_RGB, heif_chroma_interleaved_24bit);


      int stride;
      const uint8_t* data = heif_image_get_plane_readonly(thumbnail_img,
                                                          heif_channel_interleaved,
                                                          &stride);

      int thumbnail_width,thumbnail_height;
      heif_image_handle_get_resolution(heif,thumbnail_handle,
                                       &thumbnail_width,&thumbnail_height);

      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (data,
                                                    GDK_COLORSPACE_RGB,
                                                    FALSE,
                                                    8,
                                                    thumbnail_width,
                                                    thumbnail_height,
                                                    stride,
                                                    NULL,
                                                    NULL);

      gtk_list_store_set(liststore, &iter, 1, pixbuf, -1);

      // heif_image_release(thumbnail_img);  // TODO: free image, but keep image data for pixbuf...
      //heif_image_handle_release(thumbnail_handle);
    }

    //heif_image_handle_release(handle);
  }

  GtkWidget* iconview = gtk_icon_view_new();
  gtk_icon_view_set_model((GtkIconView*) iconview, (GtkTreeModel*) liststore);
  gtk_icon_view_set_text_column((GtkIconView*) iconview, 0);
  gtk_icon_view_set_pixbuf_column((GtkIconView*) iconview, 1);
  gtk_container_add (GTK_CONTAINER (frame), iconview);
  gtk_widget_show(iconview);


#if 0
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Dummy 1:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
			      vals->dummy1, 0, 100, 1, 10, 0,
			      TRUE, 0, 0,
			      _("Dummy scale entry 1"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vals->dummy1);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Dummy 2:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
			      vals->dummy2, 0, 200, 1, 10, 0,
			      TRUE, 0, 0,
			      _("Dummy scale entry 2"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vals->dummy2);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Dummy 3:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
			      vals->dummy3, -100, 100, 1, 10, 0,
			      TRUE, 0, 0,
			      _("Dummy scale entry 3"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vals->dummy3);

  /*  gimp_random_seed_new() example  */

  frame = gimp_frame_new (_("A Random Seed Entry"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  hbox2 = gimp_random_seed_new (&vals->seed, &vals->random_seed);
  gtk_widget_set_size_request (GTK_WIDGET (GIMP_RANDOM_SEED_SPINBUTTON (hbox2)),
                               RANDOM_SEED_WIDTH, -1);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  /*  gimp_coordinates_new() example  */

  frame = gimp_frame_new (_("A GimpCoordinates Widget\n"
			   "Initialized with the Drawable's Size"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  //OBS unit = gimp_image_get_unit (image_ID);
  //OBS gimp_image_get_resolution (image_ID, &xres, &yres);

  coordinates = gimp_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
				      GIMP_SIZE_ENTRY_UPDATE_SIZE,

				      ui_vals->chain_active, TRUE,

				      _("Width:"), drawable->width, xres,
				      1, GIMP_MAX_IMAGE_SIZE,
				      0, drawable->width,

				      _("Height:"), drawable->height, yres,
				      1, GIMP_MAX_IMAGE_SIZE,
				      0, drawable->height);
  gtk_box_pack_start (GTK_BOX (hbox), coordinates, FALSE, FALSE, 0);
  gtk_widget_show (coordinates);

  /*  Image and drawable menus  */

  frame = gimp_frame_new (_("Image and Drawable Menu Examples"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;

  combo = gimp_layer_combo_box_new (NULL, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), drawable->drawable_id,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &drawable_vals->drawable_id);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
			     _("Layers:"), 0.0, 0.5, combo, 1, FALSE);

  combo = gimp_image_combo_box_new (dialog_image_constraint_func, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), image_ID,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &image_vals->image_id);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
			     _("RGB Images:"), 0.0, 0.5, combo, 1, FALSE);

  /*  Show the main containers  */
#endif

  gtk_widget_show (main_vbox);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      /*  Save ui values  */
      /*
      ui_state->chain_active =
        gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON (coordinates));
      */

      //ui_result->selected_image = gtk_combo_box_get_active(combobox);
    }

  GList* selected_items = gtk_icon_view_get_selected_items((GtkIconView*) iconview);
  if (selected_items) {
    GtkTreePath* path = (GtkTreePath*)(selected_items->data);
    gint* indices = gtk_tree_path_get_indices(path);
    ui_result->selected_image = indices[0];

    g_list_free_full(selected_items, (GDestroyNotify) gtk_tree_path_free);
  }

  gtk_widget_destroy (dlg);

  return run;
}


/*  Private functions  */

#if 0
static gboolean
dialog_image_constraint_func (gint32    image_id,
                              gpointer  data)
{
  return (gimp_image_base_type (image_id) == GIMP_RGB);
}
#endif
