# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © {{year}}-present  {{author}} <{{email}}>
"""
Project wg-client
"""
__version__ = "5.10.0"
__date__ = "2024-10-20"
__reldev__ = "release"

def version() -> str:
    """ report version and release date """
    vers = f'wg-client: version {__version__} ({__reldev__}, {__date__})'
    return vers
