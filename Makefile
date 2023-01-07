all: bf-interpreter bf-jit

bf-interpreter: ./src/bf-interpreter.cpp
	$(CXX) ./src/bf-interpreter.cpp -o ./bf-interpreter $(CXXFLAGS)

bf-jit: ./src/bf-libgccjit.cpp
	$(CXX) ./src/bf-libgccjit.cpp -o ./bf-jit -lgccjit -std=c++17 -DIS_JIT=1 $(CXXFLAGS)

bf-compiler: ./src/bf-libgccjit.cpp
	$(CXX) ./src/bf-libgccjit.cpp -o ./bf-compiler -lgccjit -std=c++17 $(CXXFLAGS)

test-all: test-bf-interpreter test-bf-jit test-bf-compiler

test-bf-interpreter: bf-interpreter
	./tests/bf/run_tests.sh ../../bf-interpreter

test-bf-jit: bf-jit
	./tests/bf/run_tests.sh ../../bf-jit

test-bf-compiler: bf-compiler
	./tests/bf/run_tests.sh ./bf-compiler-runner.sh

clean:
	rm ./bf-interpreter ./bf-jit
