
.PHONY: all clean install

debug ?= y
libcxx ?= n
strip ?= n
OUT_DIR = $(CURDIR)

ifeq ($(debug), y)
CXXFLAGS += -g -O0
else
CXXFLAGS += -O2
endif
ifeq ($(libcxx), y)
CXXFLAGS += -stdlib=libc++ -std=c++17
else
CXXFLAGS += -std=c++17
endif


all:
	@echo "Building with debug=$(debug) and libcxx=$(libcxx) and strip=$(strip)"
	mkdir -p $(OUT_DIR)
	cd src && make debug=$(debug) libcxx=$(libcxx) strip=$(strip)
	cp -rf src/out $(OUT_DIR)
	@echo "Copying output to $(OUT_DIR)"
	@echo "Build complete. Output is in the 'out' directory."

clean:
	rm -rf out && cd src && make clean

install:
	cd src && cp -rf out