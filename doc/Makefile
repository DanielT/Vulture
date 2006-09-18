all: INSTALL_unix INSTALL_win32 plan changelog index context

posthook: INSTALL_unix.html INSTALL_win32.html plan.html index.html changelog.html changelog.txt context

.PHONY: all \
	INSTALL_unix \
	INSTALL_win32 \
	plan \
	changelog.rst \
	index \
	posthook \
	context \
	clean spotless

context:
	darcs changes --context > $@

INSTALL_unix: INSTALL_unix.html INSTALL_unix.tex INSTALL_unix.pdf

INSTALL_win32: INSTALL_win32.html INSTALL_win32.tex INSTALL_win32.pdf

index: index.html

plan: plan.html plan.tex plan.pdf

changelog: changelog.html changelog.tex changelog.pdf changelog.txt

index.html: index.rst
	rst2html -stdg index.rst > $@

INSTALL_unix.html: INSTALL_unix.rst
	rst2html -stdg INSTALL_unix.rst > $@

INSTALL_unix.tex: INSTALL_unix.rst
	rst2latex INSTALL_unix.rst > INSTALL_unix.tex

INSTALL_unix.pdf: INSTALL_unix.tex
	pdflatex INSTALL_unix.tex

INSTALL_win32.html: INSTALL_win32.rst
	rst2html -stdg INSTALL_win32.rst > $@

INSTALL_win32.tex: INSTALL_win32.rst
	rst2latex INSTALL_win32.rst > INSTALL_win32.tex

INSTALL_win32.pdf: INSTALL_win32.tex
	pdflatex INSTALL_win32.tex

plan.html: plan.rst
	rst2html -stdg plan.rst >$@

plan.tex: plan.rst
	rst2latex plan.rst >$@

plan.pdf: plan.tex
	pdflatex plan.tex

changelog.rst:
	python ../util/changelog.py > changelog.rst

changelog.html: changelog.rst
	rst2html -stdg changelog.rst >$@

changelog.tex: changelog.rst
	rst2latex changelog.rst >$@

changelog.pdf: changelog.tex
	pdflatex changelog.tex

changelog.txt:
	darcs changes > $@

clean:
	rm -f *.aux *.out *.log

spotless: clean
	rm -f *.tex *.pdf
