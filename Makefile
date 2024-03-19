OUT_DIR=out/release
gn_args=

ifeq ($(libcxx), yes)
	libcxx_flag=true
else
	libcxx_flag=false
endif
gn_args += use_custom_libcxx=$(libcxx_flag)

ifeq ($(debug), yes)
	debug_flag=true
else
	debug_flag=false
endif
gn_args += is_debug=$(debug_flag)

all:
	gn gen $(OUT_DIR) --args="$(gn_args)"
	ninja -C $(OUT_DIR)
ifeq ($(strip), yes)
	strip $(OUT_DIR)/sparo
endif

clean:
	rm -rf out
