CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = simtemp_cli
SRCS = main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
