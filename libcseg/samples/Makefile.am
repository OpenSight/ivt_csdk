AUTOMAKE_OPTIONS=foreign subdir-objects


AM_CPPFLAGS = -I$(srcdir)/../ -I$(srcdir)

noinst_PROGRAMS = 

if INCLUDE_FFMPEG_SAMPLES

noinst_PROGRAMS += ffmpeg_test 

ffmpeg_test_SOURCES = ffmpeg_test.c  \
    auxiliary/get_bits.h \
    auxiliary/mpeg4audio.h  \
    auxiliary/mpeg4audio.c
    
ffmpeg_test_LDADD = $(builddir)/../libcseg.a  $(builddir)/../../libivtutil/libivtutil.a  

ffmpeg_test_CFLAGS = $(libavformat_CFLAGS) $(libavutil_CFLAGS) \
    $(libavcodec_CFLAGS) $(libavfilter_CFLAGS) $(libswscale_CFLAGS) $(libswresample_CFLAGS)  \
    $(libavdevice_CFLAGS)
ffmpeg_test_LDFLAGS = $(libavformat_LIBS) $(libavutil_LIBS) \
    $(libavcodec_LIBS) $(libavfilter_LIBS) $(libswscale_LIBS) $(libswresample_LIBS) \
    $(libavdevice_LIBS)
    
endif