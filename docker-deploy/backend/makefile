cc = g++
cflags = -g -pthread -lpqxx -lprotobuf -lpq -Iinclude -Iproto
deps = include/util.h include/server.h
main = src/main.cpp
sources = src/app.cpp src/db.cpp
objects = $(patsubst %.cpp,build/%.o,$(notdir ${sources}))
protocc = $(wildcard proto/*.cc)
protoob = $(patsubst %.cc,build/%.o,$(notdir ${protocc}))

all: $(objects) $(protoob) $(main) $(deps)
	$(cc) -o backend $^ $(cflags)
	
build/%.o: src/%.cpp include/%.h $(deps) | build
	$(cc) -o $@ -c $< $(cflags)

build/%.pb.o: proto/%.pb.cc proto/%.pb.h
	$(cc) -o $@ -c $< $(cflags)

protoc: proto/world_ups.proto proto/a2u.proto proto/u2a.proto | proto
	protoc --proto_path=proto --cpp_out=proto $^

build:
	mkdir $@

proto:
	mkdir $@

.PHONY: clean
clean:
	rm -r build
