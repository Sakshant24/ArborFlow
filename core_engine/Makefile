CC = gcc
CFLAGS = -Wall -O2 -std=c11 -Iinclude -Ischeduler
LDFLAGS = -lpcap -pthread

# Original (requires libpcap + live network)
SRC = src/bit_vector.c \
      src/ip_trie.c \
      src/gatekeeper.c \
      src/concurrent_q.c \
      src/capture.c \
      scheduler/scheduler.c \
      main.c

# CSV version (reads from dataset - no libpcap needed)
SRC_CSV = src/bit_vector.c \
          src/ip_trie.c \
          src/gatekeeper.c \
          scheduler/scheduler.c \
          main_csv.c

OBJ = $(SRC:.c=.o)
OBJ_CSV = $(SRC_CSV:.c=.o)

TARGET = arborflow
TARGET_CSV = arborflow_csv

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

csv: $(TARGET_CSV)

$(TARGET_CSV): $(OBJ_CSV)
	$(CC) $(CFLAGS) -o $(TARGET_CSV) $(OBJ_CSV)

clean:
	del /f $(subst /,\,$(OBJ)) $(TARGET).exe
	del /f $(subst /,\,$(OBJ_CSV)) $(TARGET_CSV).exe