## Demonstrate deadlock in nng library

* make
* ./nng-deadlock

### Sample output:

```
$ ./nng-deadlock 2 pair1
Use pair1 mode
server entered pass 0 on ipc:///tmp/repreq-bench.13824.0
client dialed after attempt 2
client entered pass 0 on ipc:///tmp/repreq-bench.13824.0
server handshaked at pass 0
client handshaked at pass 0
client pass 0: send done.
client pass 0: recv wait...
server pass 0: recv done.
server pass 0: send done.
server pass 0: free done.
client pass 0: recv done.
client pass 0: free done.
client finished pass 0
client entered pass 1 on ipc:///tmp/repreq-bench.13824.1
server finished pass 0
server entered pass 1 on ipc:///tmp/repreq-bench.13824.1
server handshaked at pass 1
client handshaked at pass 1
client pass 1: send done.
client pass 1: recv wait...
server pass 1: recv done.
server pass 1: send done.
server pass 1: free done.
server finished pass 1
SERVER SUCCESSFULLY FINISHED
^C
```

### Explanation:

* send() from server is done, but **client stops on recv()**

### Optional command line params:

* arg1 = num-loops = integer, by default is 999
* arg2 = mode = pair0 or pair1, by default is repreq
