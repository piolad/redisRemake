# basic protocol

This implemntation uses prefixing the message with its length in bytes.
- We move from sending raw strings to sending a length-prefixed message.
- We reuse a single TCP connection for multiple messages.

## length-prefix

```bash
./server
#Received: "HEY" [ 03 00 00 00 48 45 59]
```

Instead of ./client - you can use netcat
```bash
printf '\x03\x00\x00\x00HEY'   | nc -N 127.0.0.1 1234   | xxd
#00000000: 0d00 0000 6f6b 2e20 6d73 675f 6c65 6e3d  ....ok. msg_len=
00000010: 33   00000000: 0d00 0000 6f6b 2e20 6d73 675f 6c65 6e3d  ....ok. msg_len=
#00000010: 33   
```


build with `make` and run `./client` and `./server` in seperate terminals
