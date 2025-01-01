# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
Project wg-client
"""
__version__ = "6.5.0"
__date__ = "2024-12-31"
__reldev__ = "release"

def version() -> str:
    """ report version and release date """
    vers = f'wg-client: version {__version__} ({__reldev__}, {__date__})'
    return vers
