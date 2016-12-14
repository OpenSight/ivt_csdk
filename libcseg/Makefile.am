AUTOMAKE_OPTIONS=foreign subdir-objects

AM_CPPFLAGS = -I$(srcdir)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(srcdir)/libcseg.pc

SUBDIRS = . samples



lib_LIBRARIES=libcseg.a

libcseg_a_SOURCES =  config.h \
    min_cached_segment.c \
    min_cached_segment.h \
    libcseg.h \
    ts_muxer.c \
    ts_muxer.h \
    vf_intf.c \
    vf_intf.h \
    seg_writers/cseg_dummy_writer.c \
    seg_writers/cseg_file_writer.c  \
    seg_writers/cseg_ivr_writer.c

libcseg_a_CPPFLAGS = -I$(srcdir)/../libivtutil/http_client -I$(srcdir)/../libivtutil/cjson
                   
include_HEADERS = $(srcdir)/libcseg.h $(srcdir)/vf_intf.h


