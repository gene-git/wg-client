# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Project wg-client
"""
__version__ = "7.1.1"
__date__ = "2026-01-06"
__reldev__ = "release"


def version() -> str:
    """ report version and release date """
    vers = f'wg-client: version {__version__} ({__reldev__}, {__date__})'
    return vers
