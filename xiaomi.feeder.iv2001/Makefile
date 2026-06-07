# Top-level convenience targets. See README.md for onboarding.

.PHONY: test-host bootstrap setup check-prereqs

test-host:
	$(MAKE) -C firmware/test test-host

bootstrap:
	./tools/bootstrap.sh

setup: bootstrap

check-prereqs:
	./tools/check-prereqs.sh host
	./tools/check-prereqs.sh target || true
