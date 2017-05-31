/* GStreamer
 * Copyright (C) 2014 <cfoch.fabian@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_IMAGESEQUENCESRC_H_
#define _GST_IMAGESEQUENCESRC_H_

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_IMAGESEQUENCESRC   (gst_imagesequencesrc_get_type())
#define GST_IMAGESEQUENCESRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_IMAGESEQUENCESRC,GstImageSequenceSrc))
#define GST_IMAGESEQUENCESRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_IMAGESEQUENCESRC,GstImageSequenceSrcClass))
#define GST_IS_IMAGESEQUENCESRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_IMAGESEQUENCESRC))
#define GST_IS_IMAGESEQUENCESRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_IMAGESEQUENCESRC))

typedef struct _GstImageSequenceSrc GstImageSequenceSrc;
typedef struct _GstImageSequenceSrcClass GstImageSequenceSrcClass;

struct _GstImageSequenceSrc
{
  GstPushSrc parent;
  GstCaps *caps;
  GPtrArray *filenames;
  int offset;
  GstClockTime duration;
  gint fps_n, fps_d;
  gchar *uri;

  /* properties */
  gchar *location;
  guint index;
  guint len;
  gboolean loop;
};

struct _GstImageSequenceSrcClass
{
  GstPushSrcClass parent_class;
};

#define GST_IMAGESEQUENCE_URI_PROTOCOL "imagesequence"

G_GNUC_INTERNAL GType gst_imagesequencesrc_get_type (void);

G_END_DECLS

#endif
