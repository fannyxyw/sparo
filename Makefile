CXX=clang++
CFLAGS= --std=c++17

ifeq ($(debug), yes)
	CFLAGS += -ggdb3
else
	CFLAGS += -Os
endif


OUT_DIR=out
.PHONY: main clean

main:
	mkdir -p $(OUT_DIR)
	$(CXX) $(CFLAGS) src/main.cc -o $(OUT_DIR)/main.o
	$(CXX) $(CFLAGS) src/example.cc -o $(OUT_DIR)/tp
	./$(OUT_DIR)/tp

clean:
	rm -rf $(OUT_DIR)