# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
Project wg-client
"""
__version__ = "6.6.0"
__date__ = "2025-02-27"
__reldev__ = "release"

def version() -> str:
    """ report version and release date """
    vers = f'wg-client: version {__version__} ({__reldev__}, {__date__})'
    return vers
