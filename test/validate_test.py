#! /usr/bin/python
import jsonschema
import json
import sys
import os
import glob

vm_schema = json.loads(open("vm.schema.json").read());

def validate_vm_spec(filename):
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


def has_required_stuff(path):

  # Certain files are mandatory
  required_files = [ "Makefile", "test.py", "README.md", "*.cpp" ]
  for file in required_files:
    if not glob.glob(file):
      raise Exception("missing " + file)

  # JSON-files must conform to VM-schema
  for json in glob.glob("*.json"):
    validate_vm_spec(json)


path = sys.argv[1] if len(sys.argv) > 1 else "."
os.chdir(path)

try:
  has_required_stuff(path)
  print "\tPASS: ",os.getcwd()
except Exception as err:
  print "\tFAIL: unmet requirements in " + path, ": " , err.message
