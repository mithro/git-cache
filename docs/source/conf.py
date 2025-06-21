# Configuration file for the Sphinx documentation builder.
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import sys
from pathlib import Path

# -- Project information -----------------------------------------------------
project = 'git-cache'
copyright = '2025, git-cache contributors'
author = 'git-cache contributors'
release = '1.0.0'
version = '1.0.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.ifconfig',
    'sphinx.ext.githubpages',
    'breathe',
    'exhale',
    'sphinx_copybutton',
    'myst_parser',
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = []

# The suffix(es) of source filenames.
source_suffix = {
    '.rst': None,
    '.md': None,
}

# The master toctree document.
master_doc = 'index'

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',
    'analytics_anonymize_ip': False,
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    'style_nav_header_background': '#2980b9',
    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Custom CSS
html_css_files = [
    'custom.css',
]

# -- Breathe configuration ---------------------------------------------------
# Tell breathe about the Doxygen output
breathe_projects = {
    "git-cache": "../doxygen/xml/"
}
breathe_default_project = "git-cache"

# -- Exhale configuration ---------------------------------------------------
# Setup the exhale extension
exhale_args = {
    # These arguments are required
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "rootFileTitle":         "API Reference",
    "doxygenStripFromPath":  "..",
    # Heavily encouraged optional argument (see docs)
    "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin":    "INPUT = ../../git-cache.h ../../github_api.h ../../git-cache.c ../../github_api.c\n"
                            "EXTRACT_ALL = YES\n"
                            "EXTRACT_PRIVATE = YES\n"
                            "EXTRACT_STATIC = YES\n"
                            "GENERATE_HTML = NO\n"
                            "GENERATE_XML = YES\n"
                            "XML_OUTPUT = xml\n"
                            "RECURSIVE = YES\n"
                            "ENABLE_PREPROCESSING = YES\n"
                            "MACRO_EXPANSION = YES\n"
                            "EXPAND_ONLY_PREDEF = NO\n"
                            "SEARCH_INCLUDES = YES\n"
                            "INCLUDE_PATH = ../../\n"
                            "PREDEFINED = \"_GNU_SOURCE=1\" \"__attribute__(x)=\" \"static=\"\n"
                            "EXCLUDE_PATTERNS = */build/* */docs/*\n"
                            "FILE_PATTERNS = *.c *.h\n"
                            "QUIET = YES\n"
                            "WARNINGS = YES\n"
                            "WARN_IF_UNDOCUMENTED = YES\n"
                            "WARN_IF_DOC_ERROR = YES\n"
}

# Tell sphinx what the primary language being documented is.
primary_domain = 'c'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'c'

# -- Extension configuration -------------------------------------------------

# -- Options for intersphinx extension ---------------------------------------
intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
}

# -- Options for todo extension ----------------------------------------------
todo_include_todos = True

# -- Options for napoleon extension ------------------------------------------
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_preprocess_types = False
napoleon_type_aliases = None
napoleon_attr_annotations = True

# -- Options for myst parser ------------------------------------------------
myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "html_admonition",
    "html_image",
    "linkify",
    "replacements",
    "smartquotes",
    "substitution",
    "tasklist",
]

# -- Options for copybutton extension ---------------------------------------
copybutton_prompt_text = r">>> |\.\.\. |\$ |In \[\d*\]: | {2,5}\.\.\.: | {5,8}: "
copybutton_prompt_is_regexp = True
copybutton_only_copy_prompt_lines = True