#! /usr/bin/python
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

def load_schema(filename):
  global vm_schema
  vm_schema = json.loads(open(filename).read());

def validate_vm_spec(filename):

  global valid_vms
  vm_spec = None

  # Load and parse as JSON
  try:
    vm_spec = json.loads(open(filename).read())
  except:
    raise Exception("JSON load / parse Error for " + filename)

  # Validate JSON according to schema
  try:
    validator(vm_schema).validate(vm_spec)
  except Exception as err:
    raise Exception("JSON schema validation failed: " + err.message)

  valid_vms.append(vm_spec)


def has_required_stuff(path):

  global jsons

  # Certain files are mandatory
  required_files = [ "Makefile", "test.py", "README.md", "*.cpp" ]
  for file in required_files:
    if not glob.glob(file):
      raise Exception("missing " + file)

  # JSON-files must conform to VM-schema
  jsons = glob.glob("*.json")
  jsons.sort()
  for json in jsons:
    validate_vm_spec(json)

def validate_path(path, verb = False):
  global verbose
  verbose = verb
  current_dir = os.getcwd()
  if not vm_schema:
    load_schema("vm.schema.json")
  os.chdir(path)
  try:
    has_required_stuff(path)
    if verbose:
      print "<validate_test> \tPASS: ",os.getcwd()
    return True
  except Exception as err:
    if verbose:
      print "<validate_test> \tFAIL: unmet requirements in " + path, ": " , err.message
  finally:
    os.chdir(current_dir)

if __name__ == "__main__":
  path = sys.argv[1] if len(sys.argv) > 1 else "."
  validate_path(path)
