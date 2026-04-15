PYTHON ?= python3
CMD ?=
BASE ?= libaoccmd
APP ?=
OUT ?= build/app/app

.PHONY: test sdk doom app psb doompsb corepsb cmdpsb usb clean all

test:
	$(PYTHON) -m pytest -q

sdk:
	bash tools/build_libaoc_wsl.sh

doom:
	bash examples/doom/build_doom_wsl.sh

app:
	test -n "$(APP)" || (echo "use: make app APP=examples/hello/hello.c OUT=artifacts/usb/hello" >&2; exit 2)
	bash tools/build_app_wsl.sh "$(APP)" "$(OUT)"

psb:
	if [ -n "$(CMD)" ]; then \
		$(PYTHON) tools/make_psb.py command --command "$(CMD)" --base-name "$(BASE)"; \
	else \
		$(PYTHON) tools/make_psb.py doom-launcher; \
	fi

doompsb:
	$(PYTHON) tools/make_psb.py doom-launcher

corepsb:
	$(PYTHON) tools/make_psb.py core-crash

cmdpsb:
	test -n "$(CMD)" || (echo "use: make cmdpsb CMD='echo OK>/etc/core/ok.log' BASE=libaoccmd" >&2; exit 2)
	$(PYTHON) tools/make_psb.py command --command "$(CMD)" --base-name "$(BASE)"

usb:
	$(PYTHON) tools/make_usb_tree.py --out dist/usb --include-psb

clean:
	rm -rf build dist .pytest_cache tests/__pycache__ tools/__pycache__

all: test sdk doom doompsb corepsb usb
