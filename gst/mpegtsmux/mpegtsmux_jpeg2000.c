/* 
 * Copyright 2006, 2007, 2008 Fluendo S.A. 
 *  Authors: Jan Schmidt <jan@fluendo.com>
 *           Kapil Agrawal <kapil@fluendo.com>
 *           Julien Moutte <julien@fluendo.com>
 *
 * This library is licensed under 4 different licenses and you
 * can choose to use it under the terms of any one of them. The
 * four licenses are the MPL 1.1, the LGPL, the GPL and the MIT
 * license.
 *
 * MPL:
 * 
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * LGPL:
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
 *
 * GPL:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * MIT:
 *
 * Unless otherwise indicated, Source Code is licensed under MIT license.
 * See further explanation attached in License Statement (distributed in the file
 * LICENSE).
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "mpegtsmux_jpeg2000.h"
#include <string.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbytewriter.h>
#include <gst/gst.h>

#define GST_CAT_DEFAULT mpegtsmux_debug

GstBuffer *
mpegtsmux_prepare_jpeg2000 (GstBuffer * buf, MpegTsPadData * data,
    MpegTsMux * mux)
{
  GstMapInfo map_info;
  GstMapInfo buf_map;
  GstByteWriter *wr = NULL;
  GstBuffer *out_buf = NULL;
  j2k_private_data *private_data = NULL;
  guint8 *elsm_header = NULL;
  int header_size;
  GstMemory *prepare_data = data->prepare_data;
  guint8 seconds = (guint8) (buf->pts / 1e9);
  guint8 minutes = (guint8) (seconds / 60);
  guint8 hours = (guint8) (minutes / 60);
  gsize out_size = 0;


  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;

  if (!gst_memory_map (prepare_data, &map_info, GST_MAP_READ)) {
    GST_ERROR_OBJECT (mux, "Unable to map memory");
    return NULL;
  }

  private_data = (j2k_private_data *) map_info.data;

  /* Hack for missing frame number index in buffer offset */
  /* guint8 frame_number = private_data->frame_number % 60; */


  if (private_data->interlace) {
    header_size = 48;
  } else {
    header_size = 38;
  }

  out_size = gst_buffer_get_size (buf) + header_size;

  GST_DEBUG_OBJECT (mux, "Creating buffer of size: %lu", out_size);

  wr = gst_byte_writer_new ();

  /* Elementary stream header box 'elsm' == 0x656c736d */
  if (gst_byte_writer_put_int32_be (wr, 0x656c736d)) {
    /* Framerate box 'frat' == 0x66726174 */
    if (gst_byte_writer_put_int32_be (wr, 0x66726174)) {
      /* put framerate denominator */
      if (!gst_byte_writer_put_uint16_be (wr, private_data->den)) {
        GST_ERROR_OBJECT (mux, "Unable to write den value");
        goto error;
      }
      /* put framerate numerator */
      if (!gst_byte_writer_put_uint16_be (wr, private_data->num)) {
        GST_ERROR_OBJECT (mux, "Unable to write num value");
        goto error;
      }

    } else {
      GST_ERROR_OBJECT (mux, "Unable to write frat tag");
      goto error;
    }

    /* Maximum bitrate box 'brat' == 0x62726174 */
    if (gst_byte_writer_put_int32_be (wr, 0x62726174)) {
      /* put Maximum bitrate */
      if (!gst_byte_writer_put_int32_be (wr, private_data->max_bitrate)) {
        GST_ERROR_OBJECT (mux, "Unable to write MaxBr value");
        goto error;
      }
      /* put size of first codestream */
      if (!gst_byte_writer_put_int32_be (wr, gst_buffer_get_size (buf))) {      /* private_data->AUF[0] */
        GST_ERROR_OBJECT (mux, "Unable to write AUF[0] value");
        goto error;
      }
      if (private_data->interlace) {
        /* put size of second codestream */
        if (!gst_byte_writer_put_int32_be (wr, gst_buffer_get_size (buf))) {
          GST_ERROR_OBJECT (mux, "Unable to write AUF[1] value");
          goto error;
        }
      }
    } else {
      GST_ERROR_OBJECT (mux, "Unable to write brat tag");
      goto error;
    }

    if (private_data->interlace) {
      /* Time Code Box 'fiel' == 0x6669656c */
      if (gst_byte_writer_put_int32_be (wr, 0x6669656c)) {
        /* put Fic */
        if (!gst_byte_writer_put_int8 (wr, private_data->Fic)) {
          GST_ERROR_OBJECT (mux, "Unable to write Fic value");
          goto error;
        }
        /* put Fio */
        if (!gst_byte_writer_put_int8 (wr, private_data->Fio)) {
          GST_ERROR_OBJECT (mux, "Unable to write Fio value");
          goto error;
        }

      } else {
        GST_ERROR_OBJECT (mux, "Unable to write fiel tag");
        goto error;
      }
    }
    /* Time Code Box 'tcod' == 0x74636f64 */
    if (gst_byte_writer_put_int32_be (wr, 0x74636f64)) {
      /* put HHMMSSFF */
      gst_byte_writer_put_uint8 (wr, hours);
      gst_byte_writer_put_uint8 (wr, minutes);
      gst_byte_writer_put_uint8 (wr, seconds);
      gst_byte_writer_put_uint8 (wr, 0x0);
      /* private_data->frame_number++; */

      /* private_data->HHMMSSFF */
      /*
         if(!gst_byte_writer_put_int32_be(wr, 0x00))
         {
         GST_ERROR_OBJECT (mux, "Unable to write HHMMSSFF value");
         goto error;
         }
       */ } else {
      GST_ERROR_OBJECT (mux, "Unable to write tcod tag");
      goto error;
    }

    /* Broadcast Color Box 'bcol' == 0x62636f6c */
    if (gst_byte_writer_put_int32_be (wr, 0x62636f6c)) {
      /* put HHMMSSFF */
      if (!gst_byte_writer_put_int8 (wr, private_data->color_spec)) {
        GST_ERROR_OBJECT (mux, "Unable to write broadcast color value");
        goto error;
      }
    } else {
      GST_ERROR_OBJECT (mux, "Unable to write broadcast color tag");
      goto error;
    }

    /* put reserved 8-bit */
    if (!gst_byte_writer_put_int8 (wr, 0xff)) {
      GST_ERROR_OBJECT (mux, "Unable to write reserved bits");
      goto error;
    }

  } else {
    GST_ERROR_OBJECT (mux, "Unable to write elsm tag");
    goto error;
  }

  /* Allocate output buffer + ELSM header size */
  out_buf = gst_buffer_new_and_alloc (out_size);

  /* Copy ELSM header */
  elsm_header = gst_byte_writer_free_and_get_data (wr);
  wr = NULL;
  gst_buffer_copy_into (out_buf, buf,
      GST_BUFFER_COPY_METADATA | GST_BUFFER_COPY_TIMESTAMPS, 0, 0);
  gst_buffer_fill (out_buf, 0, elsm_header, header_size);
  g_free (elsm_header);

  /* Copy complete frame */
  gst_buffer_map (buf, &buf_map, GST_MAP_READ);
  gst_buffer_fill (out_buf, header_size, buf_map.data, buf_map.size);
  gst_buffer_unmap (buf, &buf_map);

error:
  /* Byte-writer not yet freed and some error occurs */
  if (wr != NULL) {
    gst_byte_writer_free (wr);
    GST_ERROR_OBJECT (mux, "Couldn't write value to buffer");
  }
  /* Unmap prepare data */
  gst_memory_unmap (prepare_data, &map_info);

  return out_buf;
}

void
mpegtsmux_free_jpeg2000 (gpointer prepare_data)
{
  /*  Free prepare data memory object */
  GstAllocator *default_allocator = gst_allocator_find (GST_ALLOCATOR_SYSMEM);
  gst_allocator_free (default_allocator, (GstMemory *) prepare_data);
  gst_object_unref (default_allocator);
}
