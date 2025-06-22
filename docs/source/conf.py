# Configuration file for the Sphinx documentation builder.
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import sys
from pathlib import Path

# Check if we're building on ReadTheDocs
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

# Dynamically discover source files for API documentation
import glob

def find_source_files():
    """Find C source and header files dynamically"""
    if on_rtd:
        # ReadTheDocs: check both current directory and project root
        print(f"ReadTheDocs current directory: {os.getcwd()}")
        print(f"Files in current directory: {os.listdir('.')}")
        
        # Try current directory first
        c_files = glob.glob('*.c')
        h_files = glob.glob('*.h')
        
        # If no files found, try parent directories
        if not c_files and not h_files:
            # Try going up to project root
            for parent_level in ['..', '../..', '../../..']:
                test_path = os.path.join(parent_level, '*.c')
                test_c = glob.glob(test_path)
                if test_c:
                    c_files = glob.glob(os.path.join(parent_level, '*.c'))
                    h_files = glob.glob(os.path.join(parent_level, '*.h'))
                    print(f"Found files in {parent_level}: {len(c_files + h_files)} files")
                    break
        
        base_path = '.'
    else:
        # Local development - files relative to docs/source
        base_path = os.path.join(os.path.dirname(__file__), '../..')
        c_files = glob.glob(os.path.join(base_path, '*.c'))
        h_files = glob.glob(os.path.join(base_path, '*.h'))
        # Convert to relative paths for Doxygen
        c_files = [os.path.relpath(f, os.path.dirname(__file__)) for f in c_files]
        h_files = [os.path.relpath(f, os.path.dirname(__file__)) for f in h_files]
    
    print(f"Raw files found: C={c_files}, H={h_files}")
    
    # Filter out test files and other non-API files
    source_files = []
    for f in c_files + h_files:
        basename = os.path.basename(f)
        # Exclude only standalone test files, not core files with "test" in name
        if not basename.startswith('test_') and not basename.startswith('example_') and basename != 'github_test.c':
            source_files.append(f)
            print(f"Including file: {f}")
        else:
            print(f"Excluding file: {f}")
    
    return source_files, base_path

source_files, project_root = find_source_files()
source_files_exist = len(source_files) > 0

print(f"ReadTheDocs build: {on_rtd}")
print(f"Found {len(source_files)} source files: {source_files}")
print(f"Project root: {project_root}")

if not source_files_exist:
    print(f"Current directory: {os.getcwd()}")
    print(f"Files in current directory: {os.listdir('.')}")

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
    'sphinx_copybutton',
    'myst_parser',
]

# Only add API documentation extensions if source files exist
if source_files_exist:
    extensions.extend(['breathe', 'exhale'])
    print("API documentation enabled - source files found")
else:
    print("API documentation disabled - source files not found")

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

# -- Conditional API documentation configuration -----------------------------
if source_files_exist:
    # -- Breathe configuration -----------------------------------------------
    # Tell breathe about the Doxygen output
    breathe_projects = {
        "git-cache": "../doxygen/xml/"
    }
    breathe_default_project = "git-cache"

    # -- Exhale configuration ------------------------------------------------
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
        "exhaleDoxygenStdin":    f"INPUT = {' '.join(source_files)}\n"
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
                                f"INCLUDE_PATH = {project_root}\n"
                                "PREDEFINED += \"_GNU_SOURCE=1\" \"__attribute__(x)=\" \"static=\"\n"
                                "EXCLUDE_PATTERNS = */build/* */docs/* */.git/* */test_*\n"
                                "FILE_PATTERNS = *.c *.h\n"
                                "QUIET = YES\n"
                                "WARNINGS = NO\n"
                                "WARN_IF_UNDOCUMENTED = NO\n"
                                "WARN_IF_DOC_ERROR = NO\n"
    }
else:
    print("Skipping API documentation - no source files found")

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