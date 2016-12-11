#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([ivt_csdk], [0.1.0], [public@opensight.cn])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])


AM_INIT_AUTOMAKE([tar-ustar])
AM_MAINTAINER_MODE
AM_SILENT_RULES

LT_PREREQ([2.2])
LT_INIT()
AC_PROG_LIBTOOL

AC_CONFIG_SRCDIR([libcseg/libcseg.h])
AC_CONFIG_HEADERS([config.h])


AC_ARG_WITH([libcseg], 
[AS_HELP_STRING([--with-libcseg], 
[would build the libcseg library @<:@default=yes@:>@])], 
[libcseg_library=${withval}], [libcseg_library=yes]) 

AM_CONDITIONAL([INCLUDE_LIBCSEG], [test "x$libcseg_library" = xyes])


AC_ARG_WITH([libcseg-samples],
[AS_HELP_STRING([--with-libcseg-samples], 
[would build the samples of libcseg library @<:@default=no@:>@])], 
[libcseg_samples=${withval}], [libcseg_samples=no]) 

AM_CONDITIONAL([INCLUDE_LIBCSEG_SAMPLES], [test "x$libcseg_samples" = xyes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# AC_PROG_RANLIB

# Set some default features required by ffmpeg_ivr code.
CPPFLAGS="-D_REENTRANT -D_THREAD_SAFE $CPPFLAGS"
CFLAGS="-pthread $CFLAGS"
CXXFLAGS="-pthread $CXXFLAGS"


# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([m], [floor])

export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/lib64/pkgconfig:/lib/pkgconfig:/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig


# PKG checks

if test "x$libcseg_samples" = xyes; then
  PKG_CHECK_MODULES(libavformat, [libavformat >= 56.40.100])
  PKG_CHECK_MODULES(libavutil, [libavutil >= 54.31.0])  
  PKG_CHECK_MODULES(libavcodec, [libavcodec >= 56.60.100])  
  PKG_CHECK_MODULES(libavfilter, [libavfilter >= 5.40.100])  
  PKG_CHECK_MODULES(libswscale, [libswscale >= 3.1.100])  
  PKG_CHECK_MODULES(libswresample, [libswresample >= 1.2.100])  
  PKG_CHECK_MODULES(libavdevice, [libavdevice >= 56.4.100]) 
  AC_SUBST(libavformat_LIBS)
  AC_SUBST(libavformat_CFLAGS)
  AC_SUBST(libavutil_LIBS)
  AC_SUBST(libavutil_CFLAGS)
  AC_SUBST(libavcodec_LIBS)
  AC_SUBST(libavcodec_CFLAGS)
  AC_SUBST(libavfilter_LIBS)
  AC_SUBST(libavfilter_CFLAGS)
  AC_SUBST(libswscale_LIBS)
  AC_SUBST(libswscale_CFLAGS)
  AC_SUBST(libswresample_LIBS)
  AC_SUBST(libswresample_CFLAGS)
  AC_SUBST(libavdevice_LIBS)
  AC_SUBST(libavdevice_CFLAGS)  
fi


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])
AC_CHECK_HEADERS([pthread.h], AC_DEFINE([HAVE_PTHREADS], [1], [make use of pthread in transcoding]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime floor gethostbyname gettimeofday memset pow select socket strcasecmp strchr strerror strncasecmp strrchr strstr])

AC_CONFIG_FILES([Makefile \
                 libcseg/Makefile \
                 libcseg/samples/Makefile \
                 libcseg/libcseg.pc])
AC_OUTPUT


AC_MSG_NOTICE([

Configure successfully!

libcseg included..................... : $libcseg_library
libcseg-sampels included ............ : $libcseg_samples





 'make' will now compile ivt_csdk and 'su -c make install' will install it.
 Note: On non-Linux systems you might need to use 'gmake' instead of 'make'.
])
