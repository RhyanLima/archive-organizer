CC = gcc
CFLAGS = -Wall -Iinclude

SRCS = src/org.c src/org_ext.c src/org_size.c src/org_dur.c src/help_data.c
OBJS = $(SRCS:.c=.o)
TARGET = org

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

src/help_data.c: resources/help.txt
	cd resources && xxd -i help.txt ../src/help_data.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) src/help_data.c
