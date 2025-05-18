# TCP-server backend.
The TCP-server backend for our readers.
- Requires SQLite development files.
- Takes two arguments ```port``` and ```database```

## Build and compile
```sh
cmake -B build
make -C build
```
## Run with the test database
Start server on 8080.
```sh
./bin/tcp_fetch 8080 ./tests/test.db
```
```tcp_fetch <port> <path/to/db>```

On a new terminal.
```sh
telnet localhost 8080
// Once connected.
FE 5F F6 6F
```

Expected return.
```sh
En uppstoppad katt
```
Alt
```sh
telnet localhost 8080
// Once connected.
B5 B3 D5 75
// Expected
En karta över norra Kaledonien
```


**Ctr_C** will shut down the server gracefully.

### Test
```sh
./bin/tcp_fetch_test <server_ip> <port> <num_connections> [requests_per_tread]
```
**Defaults:**

```127.0.0.1 8080 50 2```

**Side-note** The test is not particularly robust but does a decent first benchmark.


## **TODO**
- Integration with the production database.
- **Parsing** if it should be handled here.
- Look over the memory management.
- **Security**?



