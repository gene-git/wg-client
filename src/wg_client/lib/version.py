# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © {{year}}-present  {{author}} <{{email}}>
"""
Project wg-client
"""
__version__ = "6.0.0"
__date__ = "2024-12-21"
__reldev__ = "release"

def version() -> str:
    """ report version and release date """
    vers = f'wg-client: version {__version__} ({__reldev__}, {__date__})'
    return vers
