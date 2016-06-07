#! /usr/bin/python
import jsonschema
import json
import sys
import os
import glob

vm_schema = None
jsons = []
valid_vms = []

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
    jsonschema.validate(vm_spec, vm_schema)
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
  for json in jsons:
    validate_vm_spec(json)

if __name__ == "__main__":
  path = sys.argv[1] if len(sys.argv) > 1 else "."
  load_schema("vm.schema.json")
  os.chdir(path)
  try:
    has_required_stuff(path)
    print "<validate_test> \tPASS: ",os.getcwd()
  except Exception as err:
    print "<validate_test> \tFAIL: unmet requirements in " + path, ": " , err.message
