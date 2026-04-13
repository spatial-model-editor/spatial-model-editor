# simple Makefile to build sphinx docs locally on linux
# to ensure build requirements are installed:
# pip install -r requirements.txt

all: html

html:
	sphinx-build -M html . build

noexec:
	sphinx-build -M html . build -D nbsphinx_execute=never

clean:
	rm -rf build sme/_autosummary
