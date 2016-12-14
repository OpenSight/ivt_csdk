TOPDIR = ../../

OBJS =	$(TOPDIR)/src/lib_json/json_reader.o				\
		$(TOPDIR)/src/lib_json/json_writer.o				\
		$(TOPDIR)/src/lib_json/json_value.o					\


LIBJSON = $(TOPDIR)lib/libjson-x86.a

EXEOBJS = $(TOPDIR)src/jsontestrunner/main.o
EXEC = $(TOPDIR)lib/json_test

CROSS	= 
CC	= gcc
CPP	= g++
LD	= ld
AR	= ar

#CFLAGS += -g
CFLAGS += -I$(TOPDIR)include  -Wall -O2 -Os
CFLAGS += -D_REENTRANT
LFLAGS = -L$(TOPDIR)lib $(LIBS)
AFLAGS += -r

TARGET = $(LIBJSON) $(EXEC)
all: $(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@
.cpp.o:
	$(CPP) -c $(CFLAGS) $< -o $@


$(LIBJSON): $(OBJS)
	$(AR) $(AFLAGS) $@ $^

$(EXEC):  $(EXEOBJS)
	$(CPP) -static -o $@ $^ $(LFLAGS) -ljson-x86

clean:
	find ./ $(TOPDIR)src/lib_json -type f \(		-name 'core'		\
				-o	-name '*~'		\
				-o	-name '*.o'		\
				-o	-name '*.a'		\
				-o	-name '*.elf*'		\
				-o	-name '*.gdb'		\
			\)	-print | xargs rm -f
	rm -f $(TARGET)

