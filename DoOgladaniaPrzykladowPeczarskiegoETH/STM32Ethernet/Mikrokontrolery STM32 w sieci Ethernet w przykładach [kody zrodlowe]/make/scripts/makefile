EXAMPLES = $(wildcard ../ex*)
ifeq ($(HARDWARES),)
  HARDWARES=zl29arm+zl3eth
endif

all:
	for x in $(EXAMPLES); do \
	  for y in $(HARDWARES); do \
	    make hardware=$$y -C $$x; \
	  done; \
	done

clean:
	for y in $(HARDWARES); do \
	  rm -rf ../lib/$$y/*; \
	  for x in $(EXAMPLES); do \
	    make hardware=$$y -C $$x clean; \
	  done; \
	done

.PHONY : all clean
