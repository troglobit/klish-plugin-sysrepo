#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 3)
m4_define([MINOR_VERSION], 0)
m4_define([MICRO_VERSION], 0)

AC_PREREQ(2.59)
AC_INIT([klish-plugin-sysrepo],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)
AC_CONFIG_MACRO_DIR([m4])

# Values for SONAME. See -version-info for details.
AC_SUBST(SONAME_CURRENT, 3)
AC_SUBST(SONAME_REVISION, 0)
AC_SUBST(SONAME_AGE, 0)

# Check for system extensions (_POSIX_THREAD_SEMANTICS for Solaris)
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(subdir-objects)
AM_PROG_CC_C_O

# needed to handle 64-bit architecture
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(size_t)


#########################################
# See if linker supports version scripts
#########################################
# Check if LD supports linker scripts,
# and define automake conditional HAVE_LD_VERSION_SCRIPT if so.
AC_ARG_ENABLE([ld-version-script],
              AS_HELP_STRING([--enable-ld-version-script],
                             [enable linker version script (default is enabled when possible)]),
                             [have_ld_version_script=$enableval], [])
if test -z "$have_ld_version_script"; then
    AC_MSG_CHECKING([if LD -Wl,--version-script works])
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,--version-script=conftest.map"
    cat > conftest.map <<EOF
VERS_1 {
    global: sym;
};

VERS_2 {
    global: sym;
} VERS_1;
EOF
    AC_LINK_IFELSE([AC_LANG_SOURCE([int main() { return 0; }])],
                   [have_ld_version_script=yes], [have_ld_version_script=no])
    rm -f conftest.map
    LDFLAGS="$save_LDFLAGS"
    AC_MSG_RESULT($have_ld_version_script)
fi
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "$have_ld_version_script" = "yes")


################################
# Deal with debugging options
################################
AC_ARG_ENABLE(debug,
              [AS_HELP_STRING([--enable-debug],
                              [Turn on debugging including asserts [default=no]])],
              [],
              [enable_debug=no])
AM_CONDITIONAL(DEBUG,test x$enable_debug = xyes)


################################
# Variables to install plugins
################################
PLUGINS_SUBDIR=plugins
AC_SUBST(PLUGINS_SUBDIR)


################################
# Check for mandatory faux library
################################
AC_ARG_WITH(faux,
            [AS_HELP_STRING([--with-faux=DIR],
                            [Search DIR directory for faux library files [default=yes]])],
            [use_faux=$withval],
            [use_faux=yes])

AS_IF([test x$use_faux != xyes],
  [
    CPPFLAGS="-I${use_faux} ${CPPFLAGS}"
    LDFLAGS="-L${use_faux}/.libs ${LDFLAGS}"
  ]
)

AC_CHECK_HEADERS([faux/faux.h],
	[],
	[AC_MSG_ERROR([cannot find <faux/faux.h> header file])]
	)
AC_SEARCH_LIBS([faux_zmalloc], [faux],
	[],
	[AC_MSG_ERROR([cannot find working faux library])]
	)


################################
# Check for mandatory libyang library
################################
AC_ARG_WITH(libyang,
            [AS_HELP_STRING([--with-libyang=DIR],
                            [Search DIR directory for libyang library files [default=yes]])],
            [use_libyang=$withval],
            [use_libyang=yes])

AS_IF([test x$use_libyang != xyes],
  [
    CPPFLAGS="-I${use_libyang} ${CPPFLAGS}"
    LDFLAGS="-L${use_libyang}/.libs ${LDFLAGS}"
  ]
)

AC_CHECK_HEADERS([libyang/libyang.h],
	[],
	[AC_MSG_ERROR([cannot find <libyang/libyang.h> header file])]
	)
AC_SEARCH_LIBS([lysc_node_child], [yang],
	[],
	[AC_MSG_ERROR([cannot find working libyang library])]
	)


################################
# Check for mandatory sysrepo library
################################
AC_ARG_WITH(sysrepo,
            [AS_HELP_STRING([--with-sysrepo=DIR],
                            [Search DIR directory for sysrepo library files [default=yes]])],
            [use_sysrepo=$withval],
            [use_sysrepo=yes])

AS_IF([test x$use_sysrepo != xyes],
  [
    CPPFLAGS="-I${use_sysrepo} ${CPPFLAGS}"
    LDFLAGS="-L${use_sysrepo}/.libs ${LDFLAGS}"
  ]
)

AC_CHECK_HEADERS([sysrepo/xpath.h],
	[],
	[AC_MSG_ERROR([cannot find <sysrepo/xpath.h> header file])]
	)
AC_SEARCH_LIBS([sr_connect], [sysrepo],
	[],
	[AC_MSG_ERROR([cannot find working sysrepo library])]
	)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
