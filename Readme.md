# Todo App made for Windows

Dependencies:

* __make__
* __VcXsrvs__ X Server

## Compile and Run

make build
make run
touch compile.sh && chmod +x compile.sh
nano compile.sh

```sh
# compile.sh
#!/bin/bash
g++ main.cpp -o todo_app -lraylib -lsqlite3 -std=c++17 && ./todo_app
```

./compile.sh

## Run

make run
./compile.sh
