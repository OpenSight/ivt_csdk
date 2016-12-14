TOPDIR = ../../

OBJS =	$(TOPDIR)/src/lib_json/json_reader.o				\
		$(TOPDIR)/src/lib_json/json_writer.o				\
		$(TOPDIR)/src/lib_json/json_value.o					\

LIBJSON = $(TOPDIR)lib/libjson.a

CROSS = sh4-linux-uclibc-

CPP	= @echo " g++ $@"; $(CROSS)g++
CC	= @echo " gcc $@"; $(CROSS)gcc
LD	= @echo " ld  $@"; $(CROSS)ld
AR  = @echo " ar  $@"; $(CROSS)ar
RM	= @echo " RM	$@"; rm -f
STRIP	= @echo " strip  $@"; $(CROSS)strip

#CFLAGS += -g
CFLAGS += -I$(TOPDIR)include  -Wall -O
CFLAGS += -D_REENTRANT
AFLAGS += -r

TARGET = $(LIBJSON) $(EXEC)
all: $(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@
.cpp.o:
	$(CPP) -c $(CFLAGS) $< -o $@


$(LIBJSON): $(OBJS)
	$(AR) $(AFLAGS) $@ $^

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	find $(TOPDIR)/src/lib_json/ -type f \(		-name 'core'		\
				-o	-name '*~'		\
				-o	-name '*.o'		\
				-o	-name '*.a'		\
				-o	-name '*.elf*'		\
				-o	-name '*.gdb'		\
			\)	-print | xargs rm -f
	rm -f $(TARGET)

