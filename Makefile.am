## Process this file with automake to generate Makefile.in
AUTOMAKE_OPTIONS = foreign nostdinc
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I. -I$(top_srcdir) -I$(top_srcdir)/src
AM_LD = $(CC)
VERSION_INFO = -version-info $(SONAME_CURRENT):$(SONAME_REVISION):$(SONAME_AGE)

if DEBUG
DEBUG_CFLAGS = -DDEBUG
endif

AM_CFLAGS = -Wall $(DEBUG_CFLAGS)
AM_LDFLAGS = -z relro -z now -z defs

bin_PROGRAMS =
lib_LTLIBRARIES =
lib_LIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =

EXTRA_DIST =
	bin/Makefile.am \
	src/Makefile.am
	LICENCE \
	README.md

include $(top_srcdir)/bin/Makefile.am
include $(top_srcdir)/src/Makefile.am
