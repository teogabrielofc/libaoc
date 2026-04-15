PYTHON ?= python

.PHONY: test sdk doom psb usb all

test:
	$(PYTHON) -m pytest -q

sdk:
	bash tools/build_libaoc_wsl.sh

doom:
	bash examples/doom/build_doom_wsl.sh

psb:
	$(PYTHON) tools/make_psb.py doom-launcher

usb:
	$(PYTHON) tools/make_usb_tree.py --out dist/usb --include-psb

all: test sdk doom psb usb
