AUTOMAKE_OPTIONS=foreign subdir-objects



SUBDIRS = . 


lib_LIBRARIES=librtmp_ivt.a

librtmp_ivt_a_SOURCES =  src/amf.c \
    src/log.c \
    src/main.c \
    src/parseurl.c     \
    src/publishLive.c      \
    src/rtmp.c         \
    src/rtmpsrv.c      \
    src/sendFlv.c      \
    src/thread.c       \
    include/amf.h      \
    include/bytes.h    \
    include/list.h     \
    include/log.h      \
    include/publishLive.h   \
    include/rtmp.h       \
    include/rtmp_sys.h   \
    include/rtmp_vf.h    \
    include/rtmpsrv.h    \
    include/sendFlv.h    \
    include/stdint_def.h \
    include/stdint_win32.h  \
    include/thread.h  

librtmp_ivt_a_CPPFLAGS = -I$(srcdir)/include -D_IVT_CSDK_
                   
include_HEADERS = $(srcdir)/include/rtmp_vf.h


