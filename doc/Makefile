all: INSTALL_unix plan changelog

.PHONY: all \
	INSTALL_unix plan \
	changelog.rst \
	clean spotless

INSTALL_unix: INSTALL_unix.html INSTALL_unix.tex INSTALL_unix.pdf

plan: plan.html plan.tex plan.pdf

changelog: changelog.html changelog.tex changelog.pdf

INSTALL_unix.html: INSTALL_unix.rst
	rst2html -stdg INSTALL_unix.rst > $@

INSTALL_unix.tex: INSTALL_unix.rst
	rst2latex INSTALL_unix.rst > INSTALL_unix.tex

INSTALL_unix.pdf: INSTALL_unix.tex
	pdflatex INSTALL_unix.tex

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

clean:
	rm -f *.aux *.out *.log

spotless: clean
	rm -f *.tex *.pdf
