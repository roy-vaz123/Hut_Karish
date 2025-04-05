# app/Makefile

CXX = g++
CXXFLAGS = -Wall -std=c++17 -I../shared/netlink_client -I../shared/message_queue -I../shared/thread_safe_unordered_map -I../shared/config -Iinclude
LDFLAGS = ../build/lib/libshared.a
TARGET = ../build/app/app

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p ../build/app
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
