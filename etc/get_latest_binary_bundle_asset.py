#!/usr/bin/python
import sys
import json
import fileinput

def get_json():
    json_str=""
    
    for line in fileinput.input():
        json_str += line

    return json_str


json_str = get_json()

#print "JSON?", 

obj = json.loads(json_str);
asset_id = obj["assets"][0]["id"]

print asset_id


