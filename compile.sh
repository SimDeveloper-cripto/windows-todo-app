#!/bin/bash
g++ main.cpp -o todo_app -lraylib -lsqlite3 -std=c++17 && ./todo_app
