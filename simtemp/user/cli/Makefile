CXX := g++
CXXFLAGS := -Wall -Wextra -O2 -std=c++17
TARGET := main
SRC := main.cpp

INCLUDES := -I../../kernel

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

