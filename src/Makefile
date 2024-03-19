CXX = clang++
CXXFLAGS = -I./third-party/robin-map/include
main: main.o json_test.o
	$(CXX) -o main main.o json_test.o $(CXXFLAGS)

clean:
	rm *.o