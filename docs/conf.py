# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import subprocess

# -- Project information -----------------------------------------------------

project = "spatial-model-editor"
copyright = "2019-2026, Liam Keegan"
author = "Liam Keegan"

root_doc = "index"

# -- General configuration ---------------------------------------------------

# generate doxygen xml output
subprocess.run(["doxygen", "Doxyfile.in"], check=True)

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "breathe",
    "myst_parser",
    "nbsphinx",
    "sphinx_rtd_theme",
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.mathjax",
    "sphinx.ext.napoleon",
]

autoclass_content = "class"

autosummary_generate = True

nbsphinx_prolog = """
|colab-icon|_ `Interactive online version <https://colab.research.google.com/github/spatial-model-editor/spatial-model-editor/blob/main/docs/{{ env.doc2path(env.docname, base=None) }}>`_

.. |colab-icon| image:: https://colab.research.google.com/assets/colab-badge.svg

.. _colab-icon: https://colab.research.google.com/github/spatial-model-editor/spatial-model-editor/blob/main/docs/{{ env.doc2path(env.docname, base=None) }}

"""

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    "build",
    "README.md",
    "Thumbs.db",
    ".DS_Store",
    "sme/_autosummary/sme.ConcentrationType*.rst",
]

# breathe: generate docs from doxygen xml output
breathe_projects = {"sme": "build/xml"}
breathe_default_project = "sme"
breathe_default_members = ("members", "undoc-members")

myst_update_mathjax = False

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"
