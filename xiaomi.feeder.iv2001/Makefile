# Top-level convenience targets. See README.md for onboarding.

.PHONY: test-host test-web bootstrap setup check-prereqs setup-flashtool uart-console

test-host:
	$(MAKE) -C firmware/test test-host

test-web:
	$(MAKE) -C tools/web test-web

bootstrap:
	./tools/bootstrap.sh

setup: bootstrap

setup-flashtool:
	./tools/setup-flashtool.sh

uart-console:
	@echo "Usage: ./tools/uart-console.sh /dev/ttyUSB0"

check-prereqs:
	./tools/check-prereqs.sh host
	./tools/check-prereqs.sh target || true
