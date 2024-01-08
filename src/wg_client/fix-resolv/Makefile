#
# Makefile for wg-fix-resolv
# Support tool for wg-client which restores a copy of saved resolv.conf 
#
CF = -Wall --std=c11
.PHONY: wg-fix-resolv

wg-fix-resolv: wg-fix-resolv.c
	@echo "On install: sudo setcap cap_chown,cap_dac_override=ep $@"
	#gcc $(CF) -Wl,--no-as-needed -lcap-ng -lc -o $@ $<
	gcc $(CF) -lcap -lc  -Wl,--no-as-needed -Wl,-z,now -o $@ $<

