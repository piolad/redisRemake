# basic protocol

This implemntation uses prefixing the message with its length in bytes

```bash
./server
#Received: "HEY" [ 03 00 00 00 48 45 59]
```

this is a simple example of client and server sending data to each other

build with `make` and run `./client` and `./server` in seperate terminals
