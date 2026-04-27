OUT_DIR ?= out/release
gn_args =
debug_flag ?= true

# Compiler selection: support 'auto', 'clang', or 'gcc'
# Default is 'auto' which prefers clang if available
COMPILER ?= auto

ifeq ($(libcxx), y)
	libcxx_flag=true
else
	libcxx_flag=false
endif
gn_args += use_custom_libcxx=$(libcxx_flag)
gn_args += strip_absolute_paths_from_debug_symbols=false

ifeq ($(debug), n)
	OUT_DIR := out/release
else
	debug_flag=true
	OUT_DIR := out/debug
# 	gn_args += symbol_level=2
endif
gn_args += is_debug=$(debug_flag)

# Dynamic compiler selection
ifeq ($(COMPILER), auto)
	# Auto-detect: prefer clang if available
	ifneq ($(shell which clang++ >/dev/null 2>&1 && echo 1),)
		gn_args += is_clang=true
	else
		gn_args += is_clang=false
	endif
else ifeq ($(COMPILER), clang)
	gn_args += is_clang=true
else ifeq ($(COMPILER), gcc)
	gn_args += is_clang=false
else
	$(error Unknown compiler: $(COMPILER). Use 'auto', 'clang', or 'gcc')
endif

all:
	gn gen $(OUT_DIR) --args="$(gn_args)" --export-compile-commands
	bear --output ../compile_commands.json -- ninja -j16 -C $(OUT_DIR)
	clang++ demo/ipc_demo.cc demo/shared_buffer.cc demo/command_line.cc -o $(OUT_DIR)/ipc_demo
ifeq ($(strip), y)
	strip $(OUT_DIR)/sparo
endif

clean:
	rm -rf out
