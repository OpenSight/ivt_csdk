AUTOMAKE_OPTIONS=foreign subdir-objects
   

    
noinst_PROGRAMS = HTTPClientSample HTTPClientSampleKeepalive


HTTPClientSample_SOURCES = HTTPClientSample.h \
    HTTPClientSample.c
   
HTTPClientSample_CPPFLAGS = -I$(srcdir)/../http_client

HTTPClientSample_LDADD = $(builddir)/../libivtutil.a



HTTPClientSampleKeepalive_SOURCES = HTTPClientSampleKeepalive.h \
    HTTPClientSampleKeepalive.c
    
HTTPClientSampleKeepalive_LDADD = $(builddir)/../libivtutil.a

HTTPClientSampleKeepalive_CPPFLAGS = -I$(srcdir)/../http_client

                    



