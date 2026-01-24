# Stage 2: Event-Based concurrency

The simplest type of concurrency is multi-threading. Redis, however, like many other high-performance systems do not use threads for concurrency.

That is because:
- threads create multiple stack -> a lot of memory usage
- A lot of overhead is needed for creating a new thread. Especially painful when the connections are short-lived.


The linux API allows for non-blocking behavior by setting the `O_NONBLOCK` flag.

```c
int flags = fcntl(fd, F_GETFL, 0);  // get the flags
flags |= O_NONBLOCK;                // modify the flags
fcntl(fd, F_SETFL, flags);          // set the flags
```

## `read()` and `write()` syscalls behavior
They do not interact with the network directly!
Instead they only operate on a kernel's buffer. 

Thus, we can compare the behavior of a blocking and non-blocking system calls:
**blocking reads**:

- 	if data is available -> copy copy from buffer
- 	if data is not available -> wait for data, blocking execution

**non-blocking reads**:
- 	if data is available -> copy from buffer 
- 	if data is not available -> return error `(errno=EAGAIN)`

**blocking writes**:
- 	if buffer not full -> copy to write buffer
- 	if buffer full -> wait with data, blocking execution

**non-blocking writes**:
- 	if buffer not full -> copy to write buffer
- 	if buffer fill -> return error `(errno=EAGAIN)`

