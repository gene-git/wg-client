# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © {{year}}-present  {{author}} <{{email}}>
"""
Project wg-client
"""
__version__ = "5.0.2"
__date__ = "2024-07-01"
__reldev__ = "release"

def version() -> str:
    """ report version and release date """
    vers = f'wg-tool: version {__version__} ({__reldev__}, {__date__})'
    return vers
