# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © {{year}}-present  {{author}} <{{email}}>
"""
Project wg-client
"""
__version__ = "4.2.0"
__date__ = "2024-04-17"
__reldev__ = "released"

def version() -> str:
    """ report version and release date """
    vers = f'wg-tool: version {__version__} ({__reldev__}, {__date__})'
    return vers
