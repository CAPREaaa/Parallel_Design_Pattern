CC=mpicc
CFLAGS=-lm -O3 -march=native 
TARGET=./bin/actor_parallel
SOURCES=./src/actor_parallel.c ./src/pool.c ./src/utils.c

#create bin directory and compile the program
all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p ./bin
	$(CC) $(SOURCES) -o $(TARGET) $(CFLAGS)

clean:
	rm -rf ./bin/
