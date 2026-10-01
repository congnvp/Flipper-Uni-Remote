UFBT ?= ufbt

.PHONY: build lint format launch clean check

check:
	python3 tools/check_version.py

build: check
	$(UFBT)

lint: check
	$(UFBT) lint

format:
	$(UFBT) format

launch: check
	$(UFBT) launch

clean:
	$(UFBT) clean
