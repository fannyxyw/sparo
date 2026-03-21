OUT_DIR ?= out/release
gn_args =
debug_flag ?= true

ifeq ($(libcxx), y)
	libcxx_flag=true
else
	libcxx_flag=false
endif
gn_args += use_custom_libcxx=$(libcxx_flag)

ifeq ($(debug), n)
	OUT_DIR := out/release
else
	debug_flag=true
	OUT_DIR := out/debug
endif
gn_args += is_debug=$(debug_flag)

all:
	gn gen $(OUT_DIR) --args="$(gn_args)" --export-compile-commands
	bear --output ../compile_commands.json -- ninja -j16 -C $(OUT_DIR)
	clang++ demo/ipc_demo.cc demo/shared_buffer.cc demo/command_line.cc -o $(OUT_DIR)/ipc_demo
ifeq ($(strip), y)
	strip $(OUT_DIR)/sparo
endif

clean:
	rm -rf out
