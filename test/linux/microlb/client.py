import requests
expected_string = "#" * 1024 * 50

def validateRequest(addr):
    #response = requests.get('https://10.0.0.42:443', verify=False, timeout=5)
    response = requests.get('http://10.0.0.42', timeout=5)
    return (response.content) == str(addr) + expected_string

print "Starting test..."
assert validateRequest(6001)
assert validateRequest(6002)
assert validateRequest(6003)
assert validateRequest(6004)

assert validateRequest(6001)
assert validateRequest(6002)
assert validateRequest(6003)
assert validateRequest(6004)
