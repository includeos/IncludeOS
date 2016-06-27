#!/usr/bin/python
""" Will return one of fields of the JSON output supplied. Expects to get two
    command line inputs:

    sys.argv[1]: JSON data from the github releases page
                 api.github.com/repos/<repo-id>/<project>/releases
    sys.argv[2]: Name of resource to print. Could be name or download url.
"""

import sys
import json
import fileinput


def get_json():
    json_str=""

    for line in sys.argv[1]:
        json_str += line

    return json_str


json_str = get_json()

obj = json.loads(json_str);

print obj[0]["assets"][0][sys.argv[2]]
