# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -I. -Icommon -Wall -std=c++11

# Libraries to link against
LIBS = -lGLEW -lglfw -lGL -lX11 -lXrandr -lXi -lXxf86vm -lXcursor -lXinerama

# Default target executable name if not provided
TARGET = starwars

# Source files
SRC = main.cpp common/shader.cpp common/tiny_obj_loader.cc

# Default rule to build and execute the program, then clean up
run: $(TARGET)
	./$(TARGET)
	make clean

# Rule to build the program
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Clean rule to remove compiled files
clean:
	rm -f $(TARGET)

