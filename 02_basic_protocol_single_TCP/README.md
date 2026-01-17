# basic protocol

This implemntation uses prefixing the message with its length in bytes.
- We move from sending raw strings to sending a length-prefixed message.
- We reuse a single TCP connection for multiple messages.

```bash
./server
#Received: "HEY" [ 03 00 00 00 48 45 59]
```

this is a simple example of client and server sending data to each other

build with `make` and run `./client` and `./server` in seperate terminals
