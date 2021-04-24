SOURCES = startup_stm32_cld.c libstm32f10x.a liblwip4.a syscalls.c \
          board_conf.c board_init.c board_led.c util_delay.c \
          util_eth_nc.c util_led.c util_lwip.c util_time.c \
          ex_httpd.c http_application.c http_parser.c http_server.c

TARGETS = httpd.elf httpd.bin

all : targets linux_image2c.elf

linux_image2c.elf : image2c.c
	gcc -Wall -O2 $^ -o $@

include ../scripts/makefile.in
