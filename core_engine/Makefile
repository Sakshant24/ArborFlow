CC = gcc
CFLAGS = -Wall -O2 -std=c11 -Iinclude -Ischeduler
LDFLAGS = -pthread

SRC = src/bit_vector.c \
      src/ip_trie.c \
      src/splay_tree.c \
      src/gatekeeper.c \
      src/concurrent_q.c \
      src/capture.c \
      scheduler/scheduler.c \
      main.c

OBJ = $(SRC:.c=.o)

TARGET = arborflow
TEST_CSV_TARGET = test_csv

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# Test target for CSV mode
test_csv: src/bit_vector.o src/ip_trie.o src/splay_tree.o src/gatekeeper.o src/concurrent_q.o src/capture.o scheduler/scheduler.o test_csv.o
	$(CC) $(CFLAGS) -o $(TEST_CSV_TARGET) $^ $(LDFLAGS)

test_csv.o: test_csv.c
	$(CC) $(CFLAGS) -c -o test_csv.o test_csv.c

clean:
	rm -f $(OBJ) test_csv.o $(TARGET) $(TEST_CSV_TARGET)