CXX=g++

CFLAGS=-g -I/usr/local/include/ -I../../comm/
LDFLAGS=-static 

ifeq ($(BIT), 64)
CFLAGS:=$(CFLAGS) -m64
LDFLAGS:=$(LDFLAGS) -shared -fPIC
TARGET:=libeasyace_lib$(BIT).a
else
BIT:=32
CFLAGS:=$(CFLAGS) -m32
LDFLAGS:=$(LDFLAGS) -shared -fPIC -m elf_i386
TARGET:=libeasyace_lib$(BIT).a
endif

INCLUDE=
LIBS= $(LDFLAGS) -lACE

BINARY = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all:$(TARGET)

%.o:%.cpp
	$(CXX) -c $< -o $@ $(INCLUDE) $(CFLAGS)

$(TARGET):$(BINARY)
	@rm -rf $@
	ar rcs $@ $^
	ln -s $@ libeasyace.a 

clean:
	@rm -rf ${BINARY} $(TARGET)

pkg:
	tar -czvf easyace.tgz ../easyace/*.h ../easyace/*.cpp ../easyace/*.txt ../easyace/easyace.* ../easyace/makefile*
	
