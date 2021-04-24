SOURCES = startup_stm32_cld.c libstm32f10x.a liblwip4.a syscalls.c \
          board_conf.c board_init.c board_lcd_ks0108.c board_led.c \
          util_delay.c util_eth_nc.c util_lcd.c util_led.c \
          util_lwip.c util_time.c ex_send_bcast.c font5x8.c

TARGETS = send_bcast.elf send_bcast.bin

all : targets linux_recv_bcast.elf

linux_recv_bcast.elf : linux_recv_bcast.c
	gcc -Wall -O2 $^ -o $@

include ../scripts/makefile.in
