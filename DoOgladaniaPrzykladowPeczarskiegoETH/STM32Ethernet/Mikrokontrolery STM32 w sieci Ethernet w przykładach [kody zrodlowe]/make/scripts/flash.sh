#!/bin/ksh

if [ $# -eq 0 ]; then
  file=*.bin
elif [ $# -eq 1 ]; then
  file=$1
else
  echo "Usage:"
  echo $0 "the_name_of_the_file_with_the_binary_flash_image"
  exit
fi

nc -t 127.0.0.1 4444 |&

read -p -d "> " x
if [ $? -ne 0 ]; then
  echo "Could not connect to OpenOCD server.  Check if it is started."
  exit
fi
print "$x"

print -p "reset halt"

read -p -d "> " x
print "$x"

print -p "flash write_image erase" $(pwd)/$file "0x8000000"

read -p -d "> " x
print "$x"

print -p "reset"

read -p -d "> " x
print "$x"

print -p "exit"

read -p x
print "$x"
