LIBS = -lssl -lcrypto -lboost_thread -lboost_system

all: ripplegen

ripplegen:
	$(CXX) $(CXX_FLAGS) -o ripplegen ripplegen.cpp \
	$(LIBS)

clean:
	rm -f ripplegen
