# Makefile for Sphinx documentation

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD  ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Custom targets for development

clean:
	rm -rf $(BUILDDIR)/*
	rm -rf $(SOURCEDIR)/api/
	rm -rf doxygen/

livehtml:
	sphinx-autobuild "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)

linkcheck:
	@$(SPHINXBUILD) -b linkcheck "$(SOURCEDIR)" "$(BUILDDIR)/linkcheck" $(SPHINXOPTS) $(O)

# Build and serve locally
serve: html
	@echo "Documentation built. Serving at http://localhost:8000"
	@cd $(BUILDDIR)/html && python3 -m http.server 8000

# Build with warnings as errors (for CI)
strict:
	@$(SPHINXBUILD) -W -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)

# Build PDF (requires LaTeX)
pdf: latex
	@make -C $(BUILDDIR)/latex all-pdf

# Quick build for development
quick:
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" -E $(SPHINXOPTS) $(O)

# Check for external link validity
links:
	@$(SPHINXBUILD) -b linkcheck "$(SOURCEDIR)" "$(BUILDDIR)/linkcheck" $(SPHINXOPTS) $(O)

# Show build statistics
stats:
	@echo "Documentation Statistics:"
	@echo "========================="
	@find $(SOURCEDIR) -name "*.rst" | wc -l | sed 's/^/RST files: /'
	@find $(SOURCEDIR) -name "*.md" | wc -l | sed 's/^/Markdown files: /'
	@wc -l $(SOURCEDIR)/*.rst | tail -1 | sed 's/total/Total lines:/'
	@if [ -d $(BUILDDIR)/html ]; then \
		echo "HTML files: $$(find $(BUILDDIR)/html -name "*.html" | wc -l)"; \
		echo "Build size: $$(du -sh $(BUILDDIR)/html | cut -f1)"; \
	fi