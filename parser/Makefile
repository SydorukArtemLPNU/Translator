CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = translator
WRAPPER_TARGET = wrapper
SOURCES = main.cpp LexicalAnalyzer.cpp Parser.cpp CodeGenerator.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = LexicalAnalyzer.h Parser.h AST.h CodeGenerator.h SymbolTable.h

.PHONY: all clean run rebuild wrapper run-wrapper

all: $(TARGET)

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

wrapper: wrapper.cpp
	$(CXX) $(CXXFLAGS) -o $(WRAPPER_TARGET) wrapper.cpp

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(WRAPPER_TARGET) *.c

run: $(TARGET)
	./$(TARGET)

run-wrapper: wrapper
	./$(WRAPPER_TARGET)