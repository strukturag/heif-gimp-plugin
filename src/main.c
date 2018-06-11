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
#define SAVE_PROC   "save_heif_file"

/*  Local function prototypes  */

static void   query(void);

static void   run(const gchar      *name,
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


  static const GimpParamDef save_args[] =
    {
      { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-NONINTERACTIVE (1) }" },
      { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
      { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export"           },
      { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
      { GIMP_PDB_STRING,   "raw-filename", "The name of the file to export the image in" },
      { GIMP_PDB_INT32,    "quality",      "Quality factor (range: 0-100. 0 = worst, 100 = best)" },
      { GIMP_PDB_INT32,    "lossless",     "Use lossless compression (0 = lossy, 1 = lossless)" }
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

  if (heif_have_decoder_for_format(heif_compression_HEVC)) {
    gimp_install_procedure (LOAD_PROC,
                            _("Load HEIF images."),
                            _("Load image stored in HEIF format (High Efficiency Image File Format). Typical suffices for HEIF files are .heif, .heic."),
                            "Dirk Farin <farin@struktur.de>",
                            "Dirk Farin <farin@struktur.de>",
                            "2018",
                            _("HEIF/HEIC"),
                            NULL,
                            GIMP_PLUGIN,
                            G_N_ELEMENTS (load_args),
                            G_N_ELEMENTS (load_return_vals),
                            load_args,
                            load_return_vals);

    gimp_register_load_handler(LOAD_PROC, "heic,heif", ""); // TODO: 'avci'
  }


  if (heif_have_encoder_for_format(heif_compression_HEVC)) {
    gimp_install_procedure (SAVE_PROC,
                            _("Save HEIF images."),
                            _("Save image in HEIF format (High Efficiency Image File Format)."),
                            "Dirk Farin <farin@struktur.de>",
                            "Dirk Farin <farin@struktur.de>",
                            "2018",
                            _("HEIF/HEIC"),
                            "RGB*",
                            GIMP_PLUGIN,
                            G_N_ELEMENTS (save_args),
                            0, // # return values
                            save_args,
                            NULL); // return values

    gimp_register_save_handler(SAVE_PROC, "heic,heif", ""); // TODO: 'avci'
  }
}



#define LOAD_HEIF_ERROR -1
#define LOAD_HEIF_CANCEL -2

gint32 load_heif(const gchar *filename, int interactive)
{
  struct heif_error err;

  struct heif_context* ctx = heif_context_alloc();
  err = heif_context_read_from_file(ctx, filename, NULL);
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

  heif_item_id primary;

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


  heif_item_id selected_image = primary;


  // if there are several images in the file and we are running interactive,
  // let the user choose a picture

  if (interactive && num > 1) {
    if (!dialog(ctx, &selected_image)) {
      heif_context_free(ctx);
      return LOAD_HEIF_CANCEL;
    }
  }



  // load the picture

  struct heif_image_handle* handle = 0;
  err = heif_context_get_image_handle(ctx, selected_image, &handle);
  if (err.code) {
    gimp_message(err.message);
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }

  int has_alpha = heif_image_handle_has_alpha_channel(handle);

  struct heif_image* img = 0;
  err = heif_decode_image(handle,
                          &img,
                          heif_colorspace_RGB,
                          has_alpha ? heif_chroma_interleaved_32bit :
                          heif_chroma_interleaved_24bit,
                          NULL);

  if (err.code) {
    gimp_message(err.message);
    heif_image_handle_release(handle);
    heif_context_free(ctx);
    return LOAD_HEIF_ERROR;
  }

  int width = heif_image_get_width(img, heif_channel_interleaved);
  int height = heif_image_get_height(img, heif_channel_interleaved);


  // --- create GIMP image and copy HEIF image into the GIMP image (converting it to RGB)

  gint32 image_ID = gimp_image_new(width, height, GIMP_RGB);
  gimp_image_set_filename(image_ID, filename);

  gint32 layer_ID = gimp_layer_new(image_ID,
                                   _("image content"),
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

  int bpp = heif_image_get_bits_per_pixel(img, heif_channel_interleaved)/8;

  if (stride == width*bpp) {
    // we can transfer the whole image at once

    gimp_pixel_rgn_set_rect(&rgn_out,
                            data,
                            0,0,width,height);
  }
  else {
    int y;
    for (y=0;y<height;y++) {
      // stride has some padding, we have to send the image line by line

      gimp_pixel_rgn_set_row(&rgn_out,
                             data+y*stride,
                             0,y,width);
    }
  }

  int num_metadata;
  heif_item_id metadata_id;
  num_metadata = heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif", &metadata_id, 1);

  if (num_metadata>0) {
    size_t data_size = heif_image_handle_get_metadata_size(handle, metadata_id);

    uint8_t* data = alloca(data_size);
    err = heif_image_handle_get_metadata(handle, metadata_id, data);

    const int heif_exif_skip = 4;

    gimp_image_attach_new_parasite(image_ID,
                                   "exif-data",
                                   0,
                                   data_size - heif_exif_skip,
                                   data + heif_exif_skip);
  }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        0,0, width,height);

  gimp_drawable_detach(drawable);

  heif_image_handle_release(handle);
  heif_context_free(ctx);
  heif_image_release(img);

  return image_ID;
}



static gboolean
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            const struct save_parameters* params,
            GError **error)
{
  // --- copy GIMP image into HEIF image

  gint           width;
  gint           height;

  width  = gimp_drawable_width(drawable_ID);
  height = gimp_drawable_height(drawable_ID);

  const gboolean has_alpha = gimp_drawable_has_alpha(drawable_ID);

  struct heif_image* image = NULL;
  struct heif_error err = heif_image_create(width, height,
                                            heif_colorspace_RGB,
                                            has_alpha ? heif_chroma_interleaved_32bit : heif_chroma_interleaved_24bit,
                                            &image);

  heif_image_add_plane(image, heif_channel_interleaved,  width, height, has_alpha ? 32 : 24);

  int stride;
  uint8_t* data = heif_image_get_plane(image, heif_channel_interleaved, &stride);

  GimpPixelRgn rgn_in;
  GimpDrawable *drawable = gimp_drawable_get(drawable_ID);

  gimp_pixel_rgn_init(&rgn_in, drawable,
                      0,0,width,height, FALSE, FALSE);

  int y;
  for (y=0;y<height;y++) {
    gimp_pixel_rgn_get_row(&rgn_in,
                           data + y*stride, 0,y, width);
  }

  gimp_drawable_detach(drawable);


  // --- encode to HEIF file ---

  struct heif_context* context = heif_context_alloc();

  struct heif_encoder* encoder;
  err = heif_context_get_encoder_for_format(context, heif_compression_HEVC, &encoder);
  if (err.code != 0) {
    fprintf(stderr, "error getting HEVC encoder: %s (%d)\n", err.message, err.code);
    heif_context_free(context);
    heif_image_release(image);
    return FALSE;
  }

  heif_encoder_set_lossy_quality(encoder, params->quality);
  heif_encoder_set_lossless(encoder, params->lossless);
  //heif_encoder_set_logging_level(encoder, logging_level);

  struct heif_image_handle* handle;
  err = heif_context_encode_image(context,
                                  image,
                                  encoder,
                                  NULL,
                                  &handle);
  if (err.code != 0) {
    fprintf(stderr, "error encoding image: %s (%d)\n", err.message, err.code);
    heif_encoder_release(encoder);
    heif_context_free(context);
    heif_image_release(image);
    return FALSE;
  }

  heif_image_handle_release(handle);

  err = heif_context_write_to_file(context, filename);
  if (err.code != 0) {
    fprintf(stderr, "error writing image to file: %s (%d)\n", err.message, err.code);
    heif_encoder_release(encoder);
    heif_context_free(context);
    heif_image_release(image);
    return FALSE;
  }

  heif_context_free(context);
  heif_image_release(image);
  heif_encoder_release(encoder);

  return TRUE;
}



static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  *return_vals  = values;
  *nreturn_vals = 1; // by default only return success code (first parameter)

  /*  Initialize i18n support  */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  gimp_ui_init (PLUGIN_NAME, TRUE);

  GimpRunMode run_mode = param[0].data.d_int32;

  if (strcmp (name, LOAD_PROC) == 0) {

    // Make sure all the arguments are there
    if (n_params != 3)
      status = GIMP_PDB_CALLING_ERROR;

    const char* filename = param[1].data.d_string;
    int is_interactive = (run_mode == GIMP_RUN_INTERACTIVE);

    if (status == GIMP_PDB_SUCCESS) {
      gint32 gimp_image_ID = load_heif(filename, is_interactive);

      if (gimp_image_ID >= 0) {
        *nreturn_vals = 2;
        values[1].type         = GIMP_PDB_IMAGE;
        values[1].data.d_image = gimp_image_ID;
      }
      else if (gimp_image_ID == LOAD_HEIF_CANCEL) {
        // No image was selected.
        status = GIMP_PDB_CANCEL;
      }
      else {
        status = GIMP_PDB_EXECUTION_ERROR;
      }
    }
  }
  else if (strcmp(name, SAVE_PROC) == 0) {

    gint32 image_ID;
    gint32 drawable_ID;

    image_ID    = param[1].data.d_int32;
    drawable_ID = param[2].data.d_int32;

    struct save_parameters params;
    params.lossless = FALSE;
    params.quality = 50;


    GimpExportReturn export = GIMP_EXPORT_CANCEL;

    switch (run_mode) {
      case GIMP_RUN_INTERACTIVE:
      case GIMP_RUN_WITH_LAST_VALS:
        export = gimp_export_image (&image_ID, &drawable_ID, "HEIF",
                                    GIMP_EXPORT_CAN_HANDLE_RGB |
                                    // GIMP_EXPORT_CAN_HANDLE_GRAY    |  // TODO
                                    GIMP_EXPORT_CAN_HANDLE_ALPHA
                                    );

        if (export == GIMP_EXPORT_CANCEL) {
          values[0].data.d_status = GIMP_PDB_CANCEL;
          return;
        }
        break;
      default:
        break;
      }


    switch (run_mode) {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (SAVE_PROC, &params);

      if (! save_dialog(&params))
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (SAVE_PROC, &params);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (n_params != 7) {
        status = GIMP_PDB_CALLING_ERROR;
      }
      else
        {
          params.quality = (param[5].data.d_int32);
          params.lossless = (param[6].data.d_int32);
        }
      break;
    }


    if (status == GIMP_PDB_SUCCESS) {
      GError* error = NULL;
      if (save_image (param[3].data.d_string, image_ID, drawable_ID, &params,
                      &error)) {
        /*  Store psvals data  */
        gimp_set_data (SAVE_PROC, &params, sizeof (params));
      }
      else {
        status = GIMP_PDB_EXECUTION_ERROR;
      }
    }
  }
  else {
    status = GIMP_PDB_CALLING_ERROR;
  }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
