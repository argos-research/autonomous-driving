proto_files := $(wildcard *.proto)

all: $(proto_files)
	protoc --proto_path=. --cpp_out=build $(proto_files)
