/* GStreamer
 * Copyright (C) 2014 Cesar Fabian Orccon Chipana
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstimagesequencesrc
 *
 * Reads buffers from a playlist (a file) of images and stream them a sequence.
 * <refsect2>
 *
 *
 * <title>How to use the element?</title>
 *
 * This is an example of the content of a playlist file.
 * |[
 * metadata,framerate=(fraction)3/1
 * image,location=/path/to/a.png
 * image,location=/path/to/b.png
 * image,location=/path/to/c.png
 * ]|
 *
 * You could use this playlist file to create an image sequence using this pipeline.
 *
 * |[
 * gst-launch-1.0 -v imagesequencesrc location="playlist" framerate="24/1" ! decodebin ! videoconvert ! xvimagesink
 * ]|
 *
 * or you can use a playbin
 *
 * |[
 * gst-launch-1.0 -v playbin uri="imagesequence:///playlist"
 * ]|
 *
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/base/gsttypefindhelper.h>

#include "gstimagesequencesrc.h"

/* prototypes */
static GstFlowReturn gst_imagesequencesrc_create (GstPushSrc * src,
    GstBuffer ** buffer);
static void gst_imagesequencesrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_imagesequence_src_change_state (GstElement *
    element, GstStateChange transition);
static void gst_imagesequencesrc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_imagesequencesrc_dispose (GObject * object);
static GstCaps *gst_imagesequencesrc_getcaps (GstBaseSrc * bsrc,
    GstCaps * filter);
static gboolean gst_imagesequencesrc_query (GstBaseSrc * bsrc,
    GstQuery * query);
static void gst_imagesequencesrc_parse_location (GstImageSequenceSrc * src);

/* pad templates */
static GstStaticPadTemplate gst_imagesequencesrc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* Utilities */
static void gst_imagesequencesrc_set_duration (GstImageSequenceSrc * src);
static void gst_imagesequencesrc_set_caps (GstImageSequenceSrc * src,
    GstCaps * value);

GST_DEBUG_CATEGORY_STATIC (gst_imagesequencesrc_debug_category);
#define GST_CAT_DEFAULT gst_imagesequencesrc_debug_category

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_FRAMERATE,
  PROP_LOOP,
  PROP_FILENAMES,
  PROP_URI
};

#define DEFAULT_URI "imagesequence://."
#define DEFAULT_FRAMERATE "1/1"
#define DEFAULT_START_INDEX 0
#define DEFAULT_LOCATION ""


static void gst_imagesequencesrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

#define _do_init \
  G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_imagesequencesrc_uri_handler_init); \
  GST_DEBUG_CATEGORY_INIT (gst_imagesequencesrc_debug_category, "imagesequencesrc", 0, "imagesequencesrc element");
#define gst_imagesequencesrc_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE (GstImageSequenceSrc, gst_imagesequencesrc,
    GST_TYPE_PUSH_SRC, _do_init);

static gboolean
is_seekable (GstBaseSrc * bsrc)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (bsrc);

  if ((src->filenames->len != 0) && (src->fps_n) && (src->fps_d))
    return TRUE;
  return FALSE;
}

static gboolean
do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstImageSequenceSrc *src;

  src = GST_IMAGESEQUENCESRC (bsrc);

  segment->time = segment->start;

  src->index = DEFAULT_START_INDEX;
  return TRUE;
}

static void
gst_imagesequencesrc_class_init (GstImageSequenceSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstPushSrcClass *gstpushsrc_class = GST_PUSH_SRC_CLASS (klass);
  GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_imagesequencesrc_set_property;
  gobject_class->get_property = gst_imagesequencesrc_get_property;

  /* Setting properties */
  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "File Location",
          "The path to a playlist file.", DEFAULT_LOCATION,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LOOP,
      g_param_spec_boolean ("loop", "Loop",
          "Whether to repeat from the beginning when all files have been read.",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FILENAMES,
      g_param_spec_boxed ("filenames-list", "Filenames (path) List",
          "Set a list of filenames directly instead of a location pattern."
          "If you *get* the current list, you will obtain a copy of it.",
          G_TYPE_PTR_ARRAY, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_FRAMERATE,
      gst_param_spec_fraction ("framerate", "Framerate",
          "Set the framerate to internal caps.",
          1, 1, G_MAXINT, 1, -1, -1,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "Set the uri.",
          "The URI of an ImageSequenceSrc is as follow:",
          "imagesequencesrc://{location}"
          DEFAULT_URI, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_imagesequencesrc_src_template));

  gst_element_class_set_static_metadata (gstelement_class,
      "ImageSequenceSrc plugin", "Src/File",
      "Creates an image-sequence video stream",
      "Cesar Fabian Orccon Chipana <cfoch.fabian@gmail.com>");

  gstbasesrc_class->get_caps = gst_imagesequencesrc_getcaps;
  gstbasesrc_class->query = gst_imagesequencesrc_query;
  gstbasesrc_class->is_seekable = is_seekable;
  gstbasesrc_class->do_seek = do_seek;

  gstpushsrc_class->create = gst_imagesequencesrc_create;

  gstelement_class->change_state = gst_imagesequence_src_change_state;

  gobject_class->dispose = gst_imagesequencesrc_dispose;
}

static GstStateChangeReturn
gst_imagesequence_src_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (src->location) {
        gst_imagesequencesrc_parse_location (src);
      }
      ret = GST_ELEMENT_CLASS (parent_class)->change_state (element,
          transition);
      break;
    default:
      ret = GST_ELEMENT_CLASS (parent_class)->change_state (element,
          transition);
      break;
  }
  return ret;
}

static void
gst_imagesequencesrc_init (GstImageSequenceSrc * src)
{
  GstBaseSrc *bsrc = GST_BASE_SRC (src);
  gst_base_src_set_format (bsrc, GST_FORMAT_TIME);
  src->duration = GST_CLOCK_TIME_NONE;
  src->location = g_strdup (DEFAULT_LOCATION);
  src->filenames = g_ptr_array_new ();
  src->index = DEFAULT_START_INDEX;
  src->fps_n = src->fps_d = -1;
}

static GstCaps *
gst_imagesequencesrc_getcaps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstCaps *caps;
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (bsrc);

  if (src->caps) {
    if (filter)
      caps = gst_caps_intersect_full (filter, src->caps,
          GST_CAPS_INTERSECT_FIRST);
    else
      caps = gst_caps_ref (src->caps);
  } else {
    if (filter)
      caps = gst_caps_ref (filter);
    else
      caps = gst_caps_new_any ();
  }

  GST_DEBUG_OBJECT (src, "Caps: %s", gst_caps_to_string (src->caps));
  GST_DEBUG_OBJECT (src, "Filter caps: %s", gst_caps_to_string (filter));
  GST_DEBUG_OBJECT (src, "Returning caps: %s", gst_caps_to_string (caps));

  return caps;
}

static gboolean
gst_imagesequencesrc_query (GstBaseSrc * bsrc, GstQuery * query)
{
  gboolean ret;
  GstImageSequenceSrc *src;

  src = GST_IMAGESEQUENCESRC (bsrc);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      switch (format) {
        case GST_FORMAT_TIME:
          if ((src->filenames->len != 0) && (!src->loop))
            gst_query_set_duration (query, format, src->duration);
          else
            gst_query_set_duration (query, format, -1);
          ret = TRUE;
          break;
        default:
          ret = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      }
      break;
    }
    default:
      ret = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      break;
  }

  return ret;
}

static gchar **
_file_get_lines (GFile * file)
{
  gsize size;
  GRegex *clean_action_str;

  GError *err = NULL;
  gchar *content = NULL, *escaped_content = NULL, **lines = NULL;

  clean_action_str = g_regex_new ("\\\\\n|#.*\n", G_REGEX_CASELESS, 0, NULL);

  /* TODO Handle GCancellable */
  if (!g_file_load_contents (file, NULL, &content, &size, NULL, &err))
    goto failed;

  if (g_strcmp0 (content, "") == 0)
    goto failed;

  escaped_content = g_regex_replace (clean_action_str, content, -1, 0, "", 0,
      NULL);
  g_free (content);

  lines = g_strsplit (escaped_content, "\n", 0);
  g_free (escaped_content);

done:

  return lines;

failed:
  if (err) {
    GST_WARNING ("Failed to load contents: %d %s", err->code, err->message);
    g_error_free (err);
  }

  if (content)
    g_free (content);
  content = NULL;

  if (escaped_content)
    g_free (escaped_content);
  escaped_content = NULL;

  if (lines)
    g_strfreev (lines);
  lines = NULL;

  goto done;
}

static gchar **
_get_lines (const gchar * _file)
{
  GFile *file = NULL;
  gchar **lines = NULL;

  GST_DEBUG ("Trying to load %s", _file);
  if ((file = g_file_new_for_path (_file)) == NULL) {
    GST_WARNING ("%s wrong uri", _file);
    return NULL;
  }

  lines = _file_get_lines (file);

  g_object_unref (file);

  return lines;
}

static GList *
_lines_get_strutures (gchar ** lines)
{
  gint i;
  GList *structures = NULL;

  for (i = 0; lines[i]; i++) {
    GstStructure *structure;

    if (g_strcmp0 (lines[i], "") == 0)
      continue;

    structure = gst_structure_from_string (lines[i], NULL);
    if (structure == NULL) {
      GST_ERROR ("Could not parse action %s", lines[i]);
      goto failed;
    }

    structures = g_list_append (structures, structure);
  }

done:
  if (lines)
    g_strfreev (lines);

  return structures;

failed:
  if (structures)
    g_list_free_full (structures, (GDestroyNotify) gst_structure_free);
  structures = NULL;

  goto done;
}

static gboolean
gst_imagesequencesrc_set_location_from_file (GstImageSequenceSrc * src)
{
  GList *structures, *l;
  gchar **lines;
  gint fps_n, fps_d;
  gint i;

  lines = _get_lines (src->location);
  if (lines == NULL)
    return FALSE;

  structures = _lines_get_strutures (lines);

  for (l = structures; l != NULL; l = l->next) {
    gchar *filename;
    gst_structure_get_fraction (l->data, "framerate", &fps_n, &fps_d);
    filename = g_strdup (gst_structure_get_string (l->data, "location"));
    if (filename) {
      g_ptr_array_add (src->filenames, filename);
      i++;
    }
  }

  g_object_set (src, "framerate", fps_n, fps_d, NULL);
  return TRUE;
}

static void
gst_imagesequencesrc_set_location (GstImageSequenceSrc * src,
    const gchar * location)
{
  GST_DEBUG_OBJECT (src, "setting location: %s", location);
  g_free (src->location);
  src->location = g_strdup (location);
}

static void
gst_imagesequencesrc_parse_location (GstImageSequenceSrc * src)
{
  GST_DEBUG_OBJECT (src, "Setting the playlist location.");
  gst_imagesequencesrc_set_location_from_file (src);
  src->index = DEFAULT_START_INDEX;
}


static void
gst_imagesequencesrc_set_duration (GstImageSequenceSrc * src)
{
  /* Calculating duration */
  src->duration =
      gst_util_uint64_scale (GST_SECOND * src->filenames->len, src->fps_d,
      src->fps_n);
}

static void
gst_imagesequencesrc_set_caps (GstImageSequenceSrc * src, GstCaps * caps)
{
  GstCaps *new_caps;

  g_assert (caps != NULL);
  new_caps = gst_caps_copy (caps);

  if (src->filenames->len > 0) {
    GValue fps = G_VALUE_INIT;
    g_value_init (&fps, GST_TYPE_FRACTION);
    gst_value_set_fraction (&fps, src->fps_n, src->fps_d);
    gst_caps_set_value (new_caps, "framerate", &fps);
  }

  gst_caps_replace (&src->caps, new_caps);
  gst_pad_set_caps (GST_BASE_SRC_PAD (src), new_caps);

  GST_DEBUG_OBJECT (src, "Setting new caps: %s", gst_caps_to_string (new_caps));
}

void
gst_imagesequencesrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (object);

  switch (property_id) {
    case PROP_LOCATION:
      gst_imagesequencesrc_set_location (src, g_value_get_string (value));
      break;
    case PROP_FRAMERATE:
    {
      src->fps_n = gst_value_get_fraction_numerator (value);
      src->fps_d = gst_value_get_fraction_denominator (value);
      GST_DEBUG_OBJECT (src, "Set (framerate) property to (%d/%d)", src->fps_n,
          src->fps_d);
      break;
    }
    case PROP_LOOP:
      src->loop = g_value_get_boolean (value);
      GST_DEBUG_OBJECT (src, "Set (loop) property to (%d)", src->loop);
      break;
    case PROP_URI:
    {
      gchar *location;
      g_free (src->uri);
      src->uri = g_strdup (g_value_get_string (value));
      location = gst_uri_get_location (src->uri);
      gst_imagesequencesrc_set_location (src, location);
      break;
    }
    case PROP_FILENAMES:
    {
      GPtrArray *filenames;
      int i;

      g_ptr_array_remove_range (src->filenames, 0, src->filenames->len);
      filenames = (GPtrArray *) g_value_get_boxed (value);

      if (filenames->len > 0)
        GST_DEBUG_OBJECT (src, "Set (filenames) property. First filename: %s\n",
            (gchar *) g_ptr_array_index (filenames, 0));
      else
        GST_DEBUG_OBJECT (src, "Set (filenames) property. No elements.");

      /* Copy filenames property value to src->filenames */
      for (i = 0; i < filenames->len; i++) {
        gchar *filename;
        filename = g_strdup (g_ptr_array_index (filenames, i));
        g_ptr_array_add (src->filenames, filename);
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_imagesequencesrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (object);

  GST_DEBUG_OBJECT (src, "get_property");

  switch (property_id) {
    case PROP_LOCATION:
      g_value_set_string (value, src->location);
      break;
    case PROP_LOOP:
      g_value_set_boolean (value, src->loop);
      break;
    case PROP_FILENAMES:
    {
      g_value_set_static_boxed (value, src->filenames);
      break;
    }
    case PROP_URI:
      g_value_set_string (value, src->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_imagesequencesrc_dispose (GObject * object)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (object);

  g_free (src->uri);
  g_free (src->location);
  src->location = NULL;
  src->uri = NULL;
  if (src->caps)
    gst_caps_unref (src->caps);
  g_ptr_array_free (src->filenames, FALSE);
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static GstFlowReturn
gst_imagesequencesrc_create (GstPushSrc * bsrc, GstBuffer ** buffer)
{
  GstImageSequenceSrc *src;
  gsize size;
  gchar *data;
  gchar *filename;
  GstBuffer *buf;
  gboolean ret;
  GError *error = NULL;
  GstClockTime buffer_duration;

  src = GST_IMAGESEQUENCESRC (bsrc);

  if (src->filenames->len == 0)
    return GST_FLOW_ERROR;

  if (src->index == src->filenames->len) {
    if (src->loop)
      src->index = DEFAULT_START_INDEX;
    else {
      src->index--;
      return GST_FLOW_EOS;
    }
  }

  filename = g_ptr_array_index (src->filenames, src->index);


  ret = g_file_get_contents (filename, &data, &size, &error);
  if (!ret)
    goto handle_error;

  GST_DEBUG_OBJECT (src, "reading from file \"%s\".", filename);

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));

  if ((src->index == DEFAULT_START_INDEX) && (!src->caps)) {
    GstCaps *caps;
    caps = gst_type_find_helper_for_buffer (NULL, buf, NULL);

    gst_imagesequencesrc_set_caps (src, caps);
    gst_imagesequencesrc_set_duration (src);
    gst_caps_unref (caps);
  }

  buffer_duration = gst_util_uint64_scale (GST_SECOND, src->fps_d, src->fps_n);

  /* Setting timestamp */
  GST_BUFFER_PTS (buf) = GST_BUFFER_DTS (buf) = src->index * buffer_duration;
  GST_BUFFER_DURATION (buf) = buffer_duration;
  GST_BUFFER_OFFSET (buf) = src->offset;
  GST_BUFFER_OFFSET_END (buf) = src->offset + size;
  src->offset += size;


  *buffer = buf;

  src->index++;
  return GST_FLOW_OK;

handle_error:
  {
    if (error != NULL) {
      GST_ELEMENT_ERROR (src, RESOURCE, READ,
          ("Error while reading from file \"%s\".", filename),
          ("%s", error->message));
      g_error_free (error);
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, READ,
          ("Error while reading from file \"%s\".", filename),
          ("%s", g_strerror (errno)));
    }
    g_free (filename);
    return GST_FLOW_ERROR;
  }
}

/**************** GstURIHandlerInterface ******************/

static GstURIType
gst_imagesequencesrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_imagesequencesrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { GST_IMAGESEQUENCE_URI_PROTOCOL, NULL };

  return protocols;
}

static gchar *
gst_imagesequencesrc_uri_get_uri (GstURIHandler * handler)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (handler);

  return g_strdup (src->uri);
}

static gboolean
gst_imagesequencesrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** err)
{
  GstImageSequenceSrc *src = GST_IMAGESEQUENCESRC (handler);

  g_object_set (src, "uri", uri, NULL);
  return TRUE;
}

static void
gst_imagesequencesrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_imagesequencesrc_uri_get_type;
  iface->get_protocols = gst_imagesequencesrc_uri_get_protocols;
  iface->get_uri = gst_imagesequencesrc_uri_get_uri;
  iface->set_uri = gst_imagesequencesrc_uri_set_uri;
}
