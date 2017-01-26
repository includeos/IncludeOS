#! /usr/bin/env python
from jsonschema import Draft4Validator, validators

# Make the validator fill in defaults from the schema
# Fetched from:
# http://python-jsonschema.readthedocs.io/en/latest/faq/
def extend_with_default(validator_class):
    validate_properties = validator_class.VALIDATORS["properties"]

    def set_defaults(validator, properties, instance, schema):
        for property, subschema in properties.iteritems():
            if "default" in subschema:
                instance.setdefault(property, subschema["default"])

        for error in validate_properties(
            validator, properties, instance, schema,
        ):
            yield error

    return validators.extend(
        validator_class, {"properties" : set_defaults},
    )

import jsonschema
import json
import sys
import os
import glob

vm_schema = None
jsons = []
valid_vms = []
verbose = False

validator = extend_with_default(Draft4Validator)

package_path = os.path.dirname(os.path.realpath(__file__))
default_schema = package_path + "/vm.schema.json"

def load_schema(filename = default_schema):
  global vm_schema
  vm_schema = json.loads(open(filename).read());


def validate_vm_spec(filename):
    vm_spec = None

    # Load and parse as JSON
    try:
        vm_spec = json.loads(open(filename).read())
    except:
        raise Exception("JSON load / parse Error for " + filename)

    if (not vm_schema): load_schema()

    # Validate JSON according to schema
    validator(vm_schema).validate(vm_spec)

    return vm_spec, filename


def load_config(path):
    global valid_vms
    global jsons

    if (os.path.isfile(path)):
        jsons = [path]

    if (os.path.isdir(path)):
        jsons = glob.glob(path + "/*.json")
        jsons.sort()

    # JSON-files must conform to VM-schema
    for json in jsons:
        spec = validate_vm_spec(json)
        try:
            spec = validate_vm_spec(json)
            valid_vms.append(spec)
        except Exception as e:
            pass
    return valid_vms


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "."
    if not load_config(path):
        print "No valid config found"
        exit(-1)
