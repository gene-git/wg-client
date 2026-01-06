#
# Makefile for wg-fix-resolv
# Support tool for wg-client which restores a copy of saved resolv.conf 
# Uses libcap, openssl
# Gene C 
#
CF = -march=x86-64-v3 -mtune=generic -mshstk -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=3 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection -std=gnu18 -Wall
LF = -Wl,-z,now,-z,shstk,-z,cet-report=error
LIBS = -lcap -lcrypto -lc

.PHONY: wg-fix-resolv

wg-fix-resolv: wg-fix-resolv.c
	@echo "On install: sudo setcap cap_chown,cap_dac_override=ep $@"
	gcc $(CF) $(LF) $(LIBS)  -o $@ $<

