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

#include <string.h>
#include <assert.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"

#include "plugin-intl.h"

#include "libheif/heif.h"


/*  Constants  */

#define LOAD_PROC   "load_heif_file"

/*  Local function prototypes  */

static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()


static void
query (void)
{
  static const GimpParamDef load_args[] =
    {
      { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-NONINTERACTIVE (1) }" },
      { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
      { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
    };

  static const GimpParamDef load_return_vals[] =
    {
      { GIMP_PDB_IMAGE, "image", "Output image" }
    };

  gimp_plugin_domain_register (PLUGIN_NAME, LOCALEDIR);

  /*
  gchar *help_path;
  gchar *help_uri;

  help_path = g_build_filename (DATADIR, "help", NULL);
  help_uri = g_filename_to_uri (help_path, NULL, NULL);
  g_free (help_path);

  gimp_plugin_help_register ("http://developer.gimp.org/plug-in-template/help",
                             help_uri);
  */

  gimp_install_procedure (LOAD_PROC,
			  _("Load HEIF images."),
                          _("Load image stored in HEIF format (High Efficiency Image File Format). Typical suffices for HEIF files are .heif, .heic."),
			  "Dirk Farin <farin@struktur.de>",
			  "Dirk Farin <farin@struktur.de>",
			  "2018",
			  _("Load HEIF image"),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (load_args),
			  G_N_ELEMENTS (load_return_vals),
			  load_args,
                          load_return_vals);

  gimp_register_load_handler(LOAD_PROC, "heic,heif", ""); // TODO: 'avci'
}



#define LOAD_HEIF_ERROR -1
#define LOAD_HEIF_CANCEL -2

gint32 load_heif(const gchar *filename, int interactive)
{
  struct heif_error err;

  struct heif_context* ctx = heif_context_alloc();
  err = heif_context_read_from_file(ctx, filename);
  if (err.code) {
    gimp_message(err.message);
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }


  // analyze image content
  // Is there more than one image? Which image is the primary image?

  int num = heif_context_get_number_of_top_level_images(ctx);
  if (num==0) {
    gimp_message(_("Input file contains no readable images"));
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }


  // get the primary image

  uint32_t primary;

  err = heif_context_get_primary_image_ID(ctx, &primary);
  if (err.code) {
    gimp_message(err.message);
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }


  // if primary image is no top level image or not present (invalid file), just take the first image

  if (!heif_context_is_top_level_image_ID(ctx, primary)) {
    int n = heif_context_get_list_of_top_level_image_IDs(ctx, &primary, 1);
    assert(n==1);
  }


  printf("primary image idx: %d\n",primary);

  uint32_t selected_image = primary;


  // if there are several images in the file and we are running interactive,
  // let the user choose a picture

  if (interactive && num > 1) {
    if (!dialog(num,&selected_image, ctx)) {
      heif_context_free(ctx);
      return LOAD_HEIF_CANCEL;
    }
  }



  // load the picture

  struct heif_image_handle* handle = 0;
  err = heif_context_get_image_handle_for_ID(ctx, selected_image, &handle);
  if (err.code) {
    gimp_message(err.message);
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }

  int has_alpha = heif_image_handle_has_alpha_channel(handle);

  struct heif_image* img = 0;
  err = heif_decode_image(handle,
                          heif_colorspace_RGB,
                          has_alpha ? heif_chroma_interleaved_32bit :
                          heif_chroma_interleaved_24bit,
                          &img);

  heif_image_handle_release(handle);
  heif_context_free(ctx);

  if (err.code) {
    gimp_message(err.message);
    return LOAD_HEIF_ERROR;
  }

  int width = heif_image_get_width(img, heif_channel_interleaved);
  int height = heif_image_get_height(img, heif_channel_interleaved);


  // --- create GIMP image and copy HEIF image into the GIMP image (converting it to RGB)

  gint32 image_ID = gimp_image_new(width, height, GIMP_RGB);
  gimp_image_set_filename(image_ID, filename);

  gint32 layer_ID = gimp_layer_new(image_ID,
                                   "image content",
                                   width,height,
                                   has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                                   100.0,
                                   GIMP_NORMAL_MODE);

  gboolean success = gimp_image_insert_layer(image_ID,
                                             layer_ID,
                                             0, // gint32 parent_ID,
                                             0); // gint position);
  if (!success) {
    heif_image_release(img);
    gimp_image_delete(image_ID);
    // TODO: do we have to delete the layer?
    return LOAD_HEIF_ERROR;
  }


  GimpDrawable *drawable = gimp_drawable_get(layer_ID);

  GimpPixelRgn rgn_out;
  gimp_pixel_rgn_init (&rgn_out,
                       drawable,
                       0,0,
                       width,height,
                       TRUE, TRUE);

  int stride;
  const uint8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

  int y;
  for (y=0;y<height;y++) {
    gimp_pixel_rgn_set_row(&rgn_out,
                           data+y*stride,
                           0,y,width);
  }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        0,0, width,height);

  gimp_drawable_detach(drawable);

  heif_image_release(img);

  return image_ID;
}


static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  //GimpDrawable      *drawable;
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  //GError            *error  = NULL;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*  Initialize i18n support  */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  run_mode = param[0].data.d_int32;
  //image_ID = param[1].data.d_int32;
  //drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          printf("interactive\n");
          break;

        case GIMP_RUN_NONINTERACTIVE:
          printf("non-interactive\n");

          /*  Make sure all the arguments are there!  */
          if (n_params != 3)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          printf("other\n");
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = load_heif (param[1].data.d_string,
                                run_mode == GIMP_RUN_INTERACTIVE);

          if (image_ID >= 0)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else if (image_ID == LOAD_HEIF_CANCEL)
            {
              // No image was selected.
              status = GIMP_PDB_CANCEL;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
#if 0
  if (strcmp (name, PROCEDURE_NAME) == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_NONINTERACTIVE:
	  if (n_params != 8)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      vals.dummy1      = param[3].data.d_int32;
	      vals.dummy2      = param[4].data.d_int32;
	      vals.dummy3      = param[5].data.d_int32;
	      vals.seed        = param[6].data.d_int32;
	      vals.random_seed = param[7].data.d_int32;

              if (vals.random_seed)
                vals.seed = g_random_int ();
	    }
	  break;

	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data (DATA_KEY_VALS,    &vals);
	  gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);

	  if (! dialog (image_ID, drawable,
			&vals, &image_vals, &drawable_vals, &ui_vals))
	    {
	      status = GIMP_PDB_CANCEL;
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data (DATA_KEY_VALS, &vals);

          if (vals.random_seed)
            vals.seed = g_random_int ();
	  break;

	default:
	  break;
	}
    }
#endif
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
#if 0
      render (image_ID, drawable, &vals, &image_vals, &drawable_vals);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  gimp_set_data (DATA_KEY_VALS,    &vals,    sizeof (vals));
	  gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
	}

      gimp_drawable_detach (drawable);
#endif
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
