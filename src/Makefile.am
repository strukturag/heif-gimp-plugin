## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gimp-plugin-template

bindir = $(GIMP_LIBDIR)/plug-ins

gimp_plugin_template_SOURCES = \
	plugin-intl.h	\
	interface.c	\
	interface.h	\
	main.c		\
	main.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

# LDADD = $(GIMP_LIBS)

gimp_plugin_template_CFLAGS = $(libheif_CFLAGS)
gimp_plugin_template_LDADD = $(libheif_LIBS) $(GIMP_LIBS)
