plugindir = ${pkglibdir}/@PLUGINS_SUBDIR@
plugin_LTLIBRARIES =

plugin_LTLIBRARIES += kplugin-sysrepo.la
kplugin_sysrepo_la_SOURCES =
kplugin_sysrepo_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version -module
kplugin_sysrepo_la_LIBS =
kplugin_sysrepo_la_CFLAGS = $(AM_LDFLAGS)
#kplugin_sysrepo_la_LIBADD = libsysrepo.la

kplugin_sysrepo_la_SOURCES += \
	src/private.h \
	src/plugin.c \
	src/sr_copypaste.h \
	src/sr_copypaste.c \
	src/pline.h \
	src/pline.c
