# FreeLinX Out-of-Tree Kernel Modules Top-Level Makefile

MODULES := flx-dummy

all:
	@for mod in $(MODULES); do \
		echo "=== Building kernel module: $$mod ==="; \
		$(MAKE) -C $$mod all || exit 1; \
	done

clean:
	@for mod in $(MODULES); do \
		$(MAKE) -C $$mod clean; \
	done

install:
	@for mod in $(MODULES); do \
		echo "=== Installing kernel module: $$mod ==="; \
		$(MAKE) -C $$mod install || exit 1; \
	done

.PHONY: all clean install
