CC:=gcc
CXX:=g++
CFLAGS:=-Iinclude -g

TARGET_DIR:=bin

all: $(TARGET_DIR)/loader $(TARGET_DIR)/test.so

$(TARGET_DIR)/bfelf_loader.o: src/bfelf_loader.c
	$(CC) $(CFLAGS) $^ -o $@ -c

$(TARGET_DIR)/loader: src/loader.c $(TARGET_DIR)/bfelf_loader.o
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_DIR)/test.so: src/test.c
	$(CC) -shared -fPIC -nostdlib $^ -o $@
