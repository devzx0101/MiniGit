# Mini-Git

A minimal, educational implementation of Git's core internal architecture written in C++.

## Project Overview
This project explores how version control systems work under the hood, covering:
- Repository initialization (`.minigit/`)
- Content hashing and object storage
- Blobs, trees, and commit architecture
- Staging area / index manipulation

## Build Instructions
```bash
# Direct compilation with g++
g++ -std=c++14 -Wall src/main.cpp -o minigit.exe
```
