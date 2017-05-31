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


#include <glib.h>
#include <unistd.h>
#include <gst/check/gstcheck.h>
#include <gio/gio.h>
#include <glib/gstdio.h>


static gboolean
_create_playlist (gchar ** playlist_location, GError ** error)
{
  GFile *playlist_gfile;
  GFileIOStream *stream;
  FILE *playlist_file;
  gchar *image_location_template;
  gchar *playlist_content;
  const gchar *playlist_content_template =
      "metadata,framerate=(fraction)1/1\n"
      "\n"
      "image,location=%s\n"
      "# Pigs fly.\n" "image,location=%s\n" "image,location=%s\n";

  playlist_gfile = g_file_new_tmp ("gst_XXXXXX.playlist", &stream, NULL);

  *playlist_location = g_file_get_path (playlist_gfile);
  playlist_file = g_fopen (*playlist_location, "w");

  if (!playlist_file)
    return FALSE;

  image_location_template = g_build_filename (GST_TEST_FILES_PATH, "%d.jpg",
      NULL);
  playlist_content = g_strdup_printf (playlist_content_template,
      image_location_template, image_location_template,
      image_location_template);

  g_fprintf (playlist_file, playlist_content, 1, 2, 3);

  g_free (playlist_content);
  g_free (image_location_template);
  fclose (playlist_file);

  return TRUE;
}

GST_START_TEST (test_properties)
{
  GstElement *pipeline, *source, *decoder, *converter, *sink;
  GPtrArray *filenames;
  gchar *playlist_location;
  gchar *test_playlist_location;
  gchar *test_img_location;

  pipeline = gst_pipeline_new ("player");
  source = gst_element_factory_make ("imagesequencesrc", "source");
  decoder = gst_element_factory_make ("decodebin", "decoder");
  converter = gst_element_factory_make ("videoconvert", "converter");
  sink = gst_element_factory_make ("fakesink", "sink");

  fail_unless (pipeline && source && decoder && converter && sink);
  fail_unless (_create_playlist (&playlist_location, NULL));

  g_object_set (G_OBJECT (source), "location", playlist_location, NULL);
  g_object_get (G_OBJECT (source), "location", &test_playlist_location,
      "filenames-list", &filenames, NULL);

  gst_bin_add_many (GST_BIN (pipeline), source, decoder, converter, sink, NULL);
  fail_unless (gst_element_link (source, decoder));
  fail_unless (gst_element_link_many (converter, sink, NULL));

  gst_element_set_state (pipeline, GST_STATE_READY);

  fail_unless (g_strcmp0 (test_playlist_location, playlist_location) == 0);

  test_img_location = g_build_filename (GST_TEST_FILES_PATH, "1.jpg", NULL);
  fail_unless (g_strcmp0 (test_img_location,
          g_ptr_array_index (filenames, 0)) == 0);
  g_free (test_img_location);

  test_img_location = g_build_filename (GST_TEST_FILES_PATH, "2.jpg", NULL);
  fail_unless (g_strcmp0 (test_img_location,
          g_ptr_array_index (filenames, 1)) == 0);
  g_free (test_img_location);

  test_img_location = g_build_filename (GST_TEST_FILES_PATH, "3.jpg", NULL);
  fail_unless (g_strcmp0 (test_img_location,
          g_ptr_array_index (filenames, 2)) == 0);
  g_free (test_img_location);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (pipeline));
}

GST_END_TEST;

static Suite *
imagesequence_suite (void)
{
  Suite *s = suite_create ("imagesequence");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_properties);

  return s;
}

GST_CHECK_MAIN (imagesequence);
