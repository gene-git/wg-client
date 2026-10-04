#
# src/data/docs/conf.py
#
import os
import sys

#
# package version from version.txt file
#
def read_version() -> str:
    """
    Get package version from version.txt file
    """
    file = '../../version.txt'
    if os.path.exists(file):
        with open(file, 'r') as fob:
            proj_vers = fob.readlines()[0]
    else:
        proj_vers = '0.1.0-unknown'
    return proj_vers

#
# Project
#
project = "wg-client"
copyright = '2023-present, Gene C'
author = 'Gene C'
release = read_version()

extensions = []


# Disable sphinx treating text blocks as python (adding weird colors)
#
highlight_language = 'none'


# ===============================
# Latex Setup
# ===============================

latex_engine = 'xelatex'
latex_use_xindy = True

latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '11pt',

    # Protrusion only to prevent XeLaTeX font expansion crashes
    # 'passoptionstopackages': r'\PassOptionsToPackage{protrusion=true}{microtype}',

    # Adjust the font size of code blocks.
    'fvset': r'\fvset{fontsize=\scriptsize}',

    'fontpkg': r'''
        \usepackage{fontspec}

        %
        % Fonts for : body (sans), headers (sans) and mono
        %
        \setmainfont{Source Sans 3}[Ligatures=TeX]
        \setsansfont{Source Sans 3}[Ligatures=TeX]
        \setmonofont{Source Code Pro}
    ''',

    'preamble': r'''
    \usepackage{parskip}

    %
    % Fix the 11pt headheight layout warnings
    %
    \setlength{\headheight}{14pt}
    \addtolength{\topmargin}{-2pt}

        %
    % List items vertical spacing
    %
    \usepackage{enumitem}
    \setlist[itemize]{
        noitemsep,
        topsep=6pt,
        parsep=0pt,
        partopsep=0pt,
        after=\vspace{0pt}
        }
    \setlist[enumerate]{
        noitemsep,
        topsep=6pt,
        parsep=0pt,
        partopsep=0pt,
        after=\vspace{0pt}
        }

    %
    % Unicode sphinx produces that latex does not understand
    %
    \usepackage{newunicodechar}
    \newunicodechar{␣}{\textvisiblespace}
    \tracinglostchars=0
    ''',
}

latex_documents = [
    (
        'index',
        'wg-client.tex',
        'wg-client Documentation ',
        'Gene C',
        'manual'
    ),
]

# ===============================
# HTML Configuration & Stylesheets
# ===============================
html_theme = 'sphinx_rtd_theme'  # Works exactly the same if using 'furo' or 'alabaster'
html_static_path = ['_static']
html_css_files = [ 'custom.css',]

