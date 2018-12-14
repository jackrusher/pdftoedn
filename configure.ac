#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([pdftoedn_version],[0.36.3])
m4_define([min_poppler_version],[0.72.0])
m4_define([min_lept_version],[1.74])
m4_define([min_boost_version],[1.57])


AC_PREREQ([2.68])
AC_INIT([pdftoedn], [pdftoedn_version], [ed@motologic.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/base_types.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_MACRO_DIR([m4])

dnl check to enable debug flags
AX_CHECK_ENABLE_DEBUG([no])

if test "x$ax_enable_debug" = "xyes"; then
   dnl add some internal debug defines
   PDFTOEDN_BUILD_CPPFLAGS="-DCHECK_CAP_CHANGE"
fi
AC_SUBST([PDFTOEDN_BUILD_CPPFLAGS])

dnl compiler options
AX_CHECK_COMPILE_FLAG([-Wno-user-defined-warnings], [CXXFLAGS="$CXXFLAGS -Wno-user-defined-warnings"])

dnl Checks for programs.
PKG_PROG_PKG_CONFIG
AC_PROG_CXX
AC_PROG_CC

dnl define version in config.h
AC_DEFINE_UNQUOTED([PDFTOEDN_VERSION], ["pdftoedn_version"], [Explicitly named version])

dnl c++-11 is required
AX_CXX_COMPILE_STDCXX_11(noext)

dnl -----------------------------------------------
dnl Checks for C libraries
AC_LANG_PUSH(C)
dnl freetype
PKG_CHECK_MODULES([freetype2], freetype2, [], [AC_MSG_ERROR([freetype2 was not found])])

dnl libpng
PKG_CHECK_MODULES([png], [libpng], libpng_ok=yes, libpng_ok=no)

if test "x$libpng_ok" = "xyes"; then
   LIBPNG_LDFLAGS="-L`pkg-config --variable=libdir libpng`"
else
   AC_MSG_ERROR([libpng was not found])
fi

dnl leptonica
PKG_CHECK_MODULES([lept], [lept >= min_lept_version], [], [AC_MSG_ERROR([leptonica min_lept_version was not found])])

dnl openssl
AX_CHECK_OPENSSL(openssl_found=yes, openssl_found=no)

if test "x$openssl_found" = "xyes"; then
   AC_DEFINE([HAVE_LIBOPENSSL], [1], [using openssl md5])
else
   AC_MSG_NOTICE([
 openssl not found - using local md5 calculation code

 If you prefer to use an openssl installed in a non-standard path, pass the following option to this script:

   --with-openssl=<path to root openssl dir>

 (This path must contain include and lib dirs).
 ])

fi
AM_CONDITIONAL([LOCAL_MD5], [test x$openssl_found = xno])

AC_LANG_POP

dnl -----------------------------------------------
dnl Checks for c++ libraries.
AC_LANG_PUSH(C++)

dnl boost
AX_BOOST_BASE([min_boost_version],[], [AC_MSG_ERROR([boost min_boost_version was not found in your system])])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_REGEX
AX_BOOST_LOCALE

dnl poppler
PKG_CHECK_MODULES([poppler], [poppler >= min_poppler_version], [], [AC_MSG_ERROR([poppler min_poppler_version was not found])])
PKG_CHECK_MODULES([poppler_cpp], [poppler-cpp >= min_poppler_version], [], [AC_MSG_ERROR([poppler cpp lib min_poppler_version not found])])

dnl poppler doesn't scope usage of its x-pdf headers but I prefer to.
dnl This is usually not a problem unless poppler is installed in a
dnl custom location because pkg-config usually returns the path
dnl including the poppler subdirectory (e.g., /usr/some/path/include/poppler)
POPPLER_PARENT_INCLUDE=-I`pkg-config --variable=includedir poppler | sed 's/\/include\/poppler.*/\/include/'`
AC_SUBST([POPPLER_PARENT_INCLUDE])

dnl make sure poppler xpdf headers are installed
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $poppler_CFLAGS"
AC_CHECK_HEADER([GfxState.h],,[AC_MSG_ERROR([poppler headers not found. Please build poppler with -DENABLE_XPDF_HEADERS=ON])])
CPPFLAGS=$save_CPPFLAGS

dnl rapidjson
PKG_CHECK_MODULES([RapidJSON], RapidJSON, [], [AC_MSG_ERROR([
rapidjson was not found

If you've installed it manually, ensure the RapidJSON.pc file is also
installed (usually in /usr/local/lib/pkgconfig/). If you've installed
it in a custom location, set the PKG_CONFIG_PATH environment variable
to point to the pkgconfig directory.])])

AC_LANG_POP


dnl -----------------------------------------------
dnl Checks for header files.
AC_CHECK_HEADERS([unistd.h wordexp.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

dnl Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([memset setlocale strerror strstr])

dnl tests
AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT
