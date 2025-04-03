# daemon/Makefile

CXX = g++
CXXFLAGS = -Wall -std=c++17 \
    -Iinclude \
    -I../shared/netlink_cleint \
	-I../shared/message_queue \
    -I../shared/config

LDFLAGS = ../build/lib/libshared.a

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = ../build/daemon/portmon_daemon

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p ../build/daemon
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
