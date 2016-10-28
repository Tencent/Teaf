CXX=g++
CFLAGS=
INCLUDE=-I../comm/ -I./
LIBS=-static -L/data/lib/lib -lACE -lnsl -lm -lz -lc -ldl -lpthread -lrt 

BINARY = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET = client

all:$(TARGET)

$(TARGET):$(BINARY)
	$(CXX) $? -o $@ $(INCLUDE) $(CFLAGS) $(LIBS)
%.o:%.cpp
	$(CXX) -c $< -o $@ $(INCLUDE) $(CFLAGS)

clean:
	@rm -rf $(BINARY) $(TARGET)
