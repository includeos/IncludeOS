# Do some simple stress tests

Systems being tested:

* ICMP, using ping-flooding (`ping -f ...`)
* UDP, using manual UDP package flooding
* TCP, using `httperf`

Sucess: test.py exits with status 0
Fail: test.py exits with another status
