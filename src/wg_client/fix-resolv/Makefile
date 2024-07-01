#
# Makefile for wg-fix-resolv
# Support tool for wg-client which restores a copy of saved resolv.conf 
# Uses libcap, openssl
# Gene C 
#
CF = -Wall --std=c11
LIBS = -lcap -lcrypto -lc
.PHONY: wg-fix-resolv

# Uncomment when in kernel (clang) and glibc (not available as of 2.39+r52)
# shadow_stack = '-fsanitize=shadow-call-stack'
shadow_stack = 

wg-fix-resolv: wg-fix-resolv.c
	@echo "On install: sudo setcap cap_chown,cap_dac_override=ep $@"
	gcc $(CF) $(shadow_stack) $(LIBS)  -Wl,--no-as-needed -Wl,-z,now -o $@ $<

