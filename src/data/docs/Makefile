#
# wg_tool:
# src/data/docs/Makefile
#
PKG = wg-client

SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

SOURCES = conf.py ../../version.txt Makefile 
SOURCES += Appendix.rst Changes-recent.rst Config.rst 
SOURCES += README.rst dns.rst index.rst Options.rst

.PHONY: help clean html latexpdf

HTML_EXCL   = --exclude _sources/ --exclude .buildinfo  --exclude .doctrees/

html: $(BUILDDIR)/html/index.html
latexpdf: $(PKG).pdf

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

$(BUILDDIR)/html/index.html: $(SOURCES)
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)
	@rsync -a $(HTML_EXCL) _build/html .
	@echo "HTML doc build complete. Path: $@"

$(PKG).pdf: $(SOURCES)
	@$(SPHINXBUILD) -b latex "$(SOURCEDIR)" "$(BUILDDIR)/latex" $(SPHINXOPTS) $(O)
	@echo "Running xelatex ..."
	$(MAKE) -C "$(BUILDDIR)/latex" all-pdf LATEXOPTS="-interaction=nonstopmode" PDFLATEX=xelatex
	$(MAKE) -C "$(BUILDDIR)/latex" all-pdf LATEXOPTS="-interaction=nonstopmode" PDFLATEX=xelatex
	@rsync -av _build/latex/$(PKG).pdf .
	@echo "PDF doc build complete. Path: $@"

clean:
	@rm -rf $(BUILDDIR)/*
	@rm -rf $(PKG).pdf
	@rm -rf html

