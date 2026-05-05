CC = gcc
CFLAGS = -Wall -O2 -std=c11 -Iinclude -Ischeduler
LDFLAGS = -lpcap -pthread

SRC = src/bit_vector.c \
      src/ip_trie.c \
      src/gatekeeper.c \
      src/concurrent_q.c \
      src/capture.c \
      scheduler/scheduler.c \
      main.c

OBJ = $(SRC:.c=.o)

TARGET = arborflow

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

clean:
	del /f $(subst /,\,$(OBJ)) $(TARGET).exe