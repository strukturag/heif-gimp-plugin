## Process this file with automake to produce Makefile.in

bin_PROGRAMS = heif-gimp-plugin

bindir = $(GIMP_LIBDIR)/plug-ins

heif_gimp_plugin_SOURCES = \
	plugin-intl.h	\
	interface.c	\
	interface.h	\
	main.c		\
	main.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

# LDADD = $(GIMP_LIBS)

heif_gimp_plugin_CFLAGS = \
	$(libheif_CFLAGS) \
	-I$(top_srcdir) \
	@GIMP_CFLAGS@ \
	-I$(includedir)

heif_gimp_plugin_LDADD = $(libheif_LIBS) $(GIMP_LIBS)
