plugin_LTLIBRARIES = libgstsequences.la

libgstsequences_la_SOURCES = \
	gstimagesequencesrc.c \
	gstimagesequence.c
libgstsequences_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS) $(GIO_CFLAGS)
libgstsequences_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) -lgstvideo-@GST_API_VERSION@ $(GST_BASE_LIBS) $(GST_LIBS) $(GIO_LIBS)
libgstsequences_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsequences_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS = gstimagesequencesrc.h
