SUBDIRS = src cli

all:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install || true; \
	done

.PHONY: all clean install
