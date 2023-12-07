# Autor Lukáš Ježek xjezek19

CXX = g++
CXXFLAGS = -std=c++11 -Wall -static-libstdc++

TARGET = sim
SOURCES = main.cpp
HEADERFILES = main.h
OBJECTS = $(SOURCES:.cpp=.o)
LIBS = -lsimlib

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) $(HEADERFILES)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LIBS)
	rm -f $(OBJECTS)

.PHONY: clean

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run:
	./$(TARGET)
