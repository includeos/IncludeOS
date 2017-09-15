# IncludeOS TCP Performance


## Howto:

Receive data from the instance:
```
ncat 10.0.0.42 1337 --recv-only > bla.txt
```

Send data to the instance:
```
cat bla.txt | ncat 10.0.0.42 1338 --send-only
```

Configure by changing the variables at the top.
