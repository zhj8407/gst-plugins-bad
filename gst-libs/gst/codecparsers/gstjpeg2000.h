/* GStreamer JPEG 2000 File Format and Code Stream Markers 
 * Copyright (C) <2016> Grok Image Compression Inc.
 *  @author Aaron Boxer <boxerab@gmail.com>
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

#ifndef __GST_JPEG2000_H__
#define __GST_JPEG2000_H__

#include <gst/gst.h>

/**
 * JPEG 2000 Formats
 */

typedef enum
{
  GST_JPEG2000_FORMAT_NO_CODEC,
  GST_JPEG2000_FORMAT_JPC,      /* JPEG 2000 code stream */
  GST_JPEG2000_FORMAT_J2C,      /* JPEG 2000 contiguous code stream box plus code stream */
  GST_JPEG2000_FORMAT_JP2,      /* JPEG 2000 part I file format */

} GstJPEG2000Format;


/**
 * JPEG 2000 Part 1 File Format
 */

#define GST_JPEG2000_JP2_SIZE_OF_BOX_ID  	4
#define GST_JPEG2000_JP2_SIZE_OF_BOX_LEN	4


/**
 * JPEG 2000 Code Stream Markers
 */

#define GST_JPEG2000_MARKER_SIZE  	4

/** Delimiting markers and marker segments */
#define GST_JPEG2000_MARKER_SOC 0xFF4F
#define GST_JPEG2000_MARKER_SOT 0xFF90
#define GST_JPEG2000_MARKER_SOD 0xFF93
#define GST_JPEG2000_MARKER_EOC 0xFFD9

/** Fixed information marker segments */
#define GST_JPEG2000_MARKER_SIZ 0xFF51

/** Functional marker segments */
#define GST_JPEG2000_MARKER_COD 0xFF52
#define GST_JPEG2000_MARKER_COC 0xFF53
#define GST_JPEG2000_MARKER_RGN 0xFF5E
#define GST_JPEG2000_MARKER_QCD 0xFF5C
#define GST_JPEG2000_MARKER_QCC 0xFF5D
#define GST_JPEG2000_MARKER_POC 0xFF5F

/** Pointer marker segments */
#define GST_JPEG2000_MARKER_PLM 0xFF57
#define GST_JPEG2000_MARKER_PLT 0xFF58
#define GST_JPEG2000_MARKER_PPM 0xFF60
#define GST_JPEG2000_MARKER_PPT 0xFF61
#define GST_JPEG2000_MARKER_TLM 0xFF55

/** In-bit-stream markers and marker segments */
#define GST_JPEG2000_MARKER_SOP 0xFF91
#define GST_JPEG2000_MARKER_EPH 0xFF92

/** Informational marker segments */
#define GST_JPEG2000_MARKER_CRG 0xFF63
#define GST_JPEG2000_MARKER_COM 0xFF64

/** JPEG 2000 progression orders
 * L - layer
 * R - resolution/decomposition level
 * C - component
 * P - position/precinct
 */
typedef enum
{
  GST_JPEG2000_PROGRESSION_ORDER_LRCP = 0,
  GST_JPEG2000_PROGRESSION_ORDER_RLCP,
  GST_JPEG2000_PROGRESSION_ORDER_RPCL,
  GST_JPEG2000_PROGRESSION_ORDER_PCRL,
  GST_JPEG2000_PROGRESSION_ORDER_CPRL,
  GST_JPEG2000_PROGRESSION_ORDER_MAX
} GstJPEG2000ProgressionOrder;


#endif
