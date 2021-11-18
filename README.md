[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-f059dc9a6f8d3a56e377f745f24479a46679e63a5d9fe6f495e02850cd0d8118.svg)](https://classroom.github.com/online_ide?assignment_repo_id=6079542&assignment_repo_type=AssignmentRepo)

> Ivan R. Buendia Gutierrez

# Build

CMake

```
mkdir -p build && cmake -B build . && make -C build
```

Compile command

```
c++ -std=c++17 -pthread src/nqueens.cc -o nqueens
```

# Run

CMake

```
./build/nqueens -problemType all -N 5
```

Compile

```
./nqueens -problemType all -N 5
```

# Generate dot image

```
dot -Tpng solution.dot -o queens.png
```
