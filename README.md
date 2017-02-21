# IncludeOS TCP Performance


## Howto:

Receive data from the instance:
```
nc 10.0.0.42 1337 > bla.txt
```

Send data to the instance:
```
cat bla.txt | nc 10.0.0.42 1338
```

Configure by changing the variables at the top.