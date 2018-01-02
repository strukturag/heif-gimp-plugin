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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"

#include "plugin-intl.h"

#include "libheif/heif.h"


/*  Constants  */

#define LOAD_PROC   "load_heif_file"

#define DATA_KEY_VALS    "plug_in_template"
#define DATA_KEY_UI_VALS "plug_in_template_ui"


/*  Local function prototypes  */

static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);


/*  Local variables  */

const PlugInVals default_vals =
{
  0,
  1,
  2,
  0,
  FALSE
};

const PlugInImageVals default_image_vals =
{
  0
};

const PlugInDrawableVals default_drawable_vals =
{
  0
};

const PlugInUIVals default_ui_vals =
{
  TRUE
};

static PlugInVals         vals;
static PlugInImageVals    image_vals;
static PlugInDrawableVals drawable_vals;
static PlugInUIVals       ui_vals;


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
  gchar *help_path;
  gchar *help_uri;

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

  help_path = g_build_filename (DATADIR, "help", NULL);
  help_uri = g_filename_to_uri (help_path, NULL, NULL);
  g_free (help_path);

  gimp_plugin_help_register ("http://developer.gimp.org/plug-in-template/help",
                             help_uri);

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

  //  gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Misc/");
}


int clip(int x)
{
  if (x<0) return 0;
  if (x>255) return 255;
  return x;
}

gint32 load_heif(const gchar *name, GError **error)
{
  struct heif_context* ctx = heif_context_alloc();
  heif_context_read_from_file(ctx, name);

  int num = heif_context_get_number_of_images(ctx);
  //int primary = heif_context_get_primary_image_index(ctx);

  int primary = -1;
  int i;
  for (i=0; i<num; i++) {
    struct heif_image_handle* h;
    heif_context_get_image_handle(ctx, i, &h);
    if (heif_context_is_primary_image(ctx, h)) {
      primary = i;
    }
    heif_image_handle_release(h);
  }

  UIResult result;
  result.selected_image = primary;

  if (num > 1) {
    dialog(num,primary,&result);

    printf("selected idx: %d\n", result.selected_image);
  }

  struct heif_image_handle* handle = 0;
  //heif_context_get_primary_image_handle(ctx, &handle);
  heif_context_get_image_handle(ctx, result.selected_image, &handle);

  //drawable,
  //        &vals, &image_vals, &drawable_vals, &ui_vals);

  struct heif_image* img = 0;
  struct heif_error err = heif_decode_image(ctx, handle, &img);

  int strideY;
  const uint8_t* dataY = heif_image_get_plane_readonly(img, heif_channel_Y, &strideY);

  int strideCb;
  const uint8_t* dataCb = heif_image_get_plane_readonly(img, heif_channel_Cb, &strideCb);

  int strideCr;
  const uint8_t* dataCr = heif_image_get_plane_readonly(img, heif_channel_Cr, &strideCr);

  int width = heif_image_get_width(img, heif_channel_Y);
  int height = heif_image_get_height(img, heif_channel_Y);


  // --- create GIMP image and copy HEIF image into the GIMP image (converting it to RGB)

  gint32 image_ID = gimp_image_new(width, height, GIMP_RGB);
  gimp_image_set_filename(image_ID, name);

  gint32 layer_ID = gimp_layer_new(image_ID,
                                   "image content",
                                   width,height,
                                   GIMP_RGB_IMAGE,
                                   100.0,
                                   GIMP_NORMAL_MODE);

  gboolean success = gimp_image_insert_layer(image_ID,
                                             layer_ID,
                                             0, // gint32 parent_ID,
                                             0); // gint position);


  GimpDrawable *drawable = gimp_drawable_get(layer_ID);

  GimpPixelRgn rgn_out;
  gimp_pixel_rgn_init (&rgn_out,
                       drawable,
                       0,0,
                       width,height,
                       TRUE, TRUE);

  guchar* buf = alloca(width*3);

  int x,y;
  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      int yv = dataY[y*strideY + x] - 16;
      int uv = dataCb[y/2*strideCb + x/2] - 128;
      int vv = dataCr[y/2*strideCr + x/2] - 128;

      float y_val = 1.164 * yv;
      buf[3*x + 0] = clip(y_val + 1.596 * vv);
      buf[3*x + 1] = clip(y_val - 0.813 * vv - 0.391 * uv);
      buf[3*x + 2] = clip(y_val + 2.018 * uv);
    }

    gimp_pixel_rgn_set_row(&rgn_out,
                           buf,
                           0,y,width);
  }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        0,0, width,height);

  gimp_drawable_detach(drawable);

  heif_image_release(img);
  heif_image_handle_release(handle);
  heif_context_free(ctx);

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
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

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

  /*  Initialize with default values  */
  vals          = default_vals;
  image_vals    = default_image_vals;
  drawable_vals = default_drawable_vals;
  ui_vals       = default_ui_vals;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (n_params != 3)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = load_heif (param[1].data.d_string, &error);

          if (image_ID != -1)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
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
