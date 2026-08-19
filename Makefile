CC = gcc
CFLAGS = -Wall -Iinclude

SRCS = src/org.c src/org_ext.c src/org_size.c src/org_dur.c src/help_data.c
OBJS = $(SRCS:.c=.o)
BIN_DIR = bin
TARGET = $(BIN_DIR)/org

.PHONY: all clean

all: $(TARGET)
	@rm -f $(OBJS) src/help_data.c

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

src/help_data.c: resources/help.txt
	cd resources && xxd -i help.txt ../src/help_data.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(OBJS) src/help_data.c
