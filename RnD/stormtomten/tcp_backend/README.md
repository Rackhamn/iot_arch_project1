# TCP-server backend.
The TCP-server backend for our readers.
- Requires SQLite development files.
- Takes two arguments "<port>" and "<database>"

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

**Ctr_C** will shut down the server gracefully.


## **TODO**
- Integration with the production database.
- **Parsing** if it should be handled here.
- Look over the memory management.
- **Security**?



