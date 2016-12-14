TOPDIR = ../../

OBJS =	$(TOPDIR)/src/lib_json/json_reader.o				\
		$(TOPDIR)/src/lib_json/json_writer.o				\
		$(TOPDIR)/src/lib_json/json_value.o					\

LIBJSON = $(TOPDIR)lib/arm-hisi3516/libjson.a

CROSS = arm-hisiv100-linux-
CC	= $(CROSS)gcc
CPP	= $(CROSS)g++
LD	= $(CROSS)ld
AR	= $(CROSS)ar

#CFLAGS += -g
CFLAGS += -I$(TOPDIR)include  -Wall -Os
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

