# shared/Makefile

CXX = g++
CXXFLAGS = -Wall -std=c++17 -I. -I./netlink -I./config
AR = ar
ARFLAGS = rcs
OUTDIR = ../build/lib
TARGET = $(OUTDIR)/libshared.a

# Find all .cpp files in shared/ and subdirectories
SRC = $(shell find . -name '*.cpp')
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(OUTDIR)
	$(AR) $(ARFLAGS) $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
