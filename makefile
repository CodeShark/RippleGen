LIBS = -lssl -lcrypto -lboost_thread -lboost_system

CXX_FLAGS = -Wall -O3 -march=native

all: ripplegen

ripplegen:
	$(CXX) $(CXX_FLAGS) -o ripplegen ripplegen.cpp \
	$(LIBS)

clean:
	rm -f ripplegen
