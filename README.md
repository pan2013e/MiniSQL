# MiniSQL
A simple DBMS that implements a subset of std. SQL, for educational purposes only.

## Intro.
A course project for Database Systems, Zhejiang University, 2021.

## Compile from source
### Prerequisites
* `>= C++14`
* CMake
* Boost
* GNU Readline (You may need to specify the paths of libraries to be linked in `CMakeLists.txt`)

### Compile
```bash
mkdir build && cd build
cmake ..
make
```

## TODO
* Authentication, data encryption
* Complete SQL grammar
* B+ Tree Indexing
* Web interface/API

## Credits 
* [p-ranav/argparse](https://github.com/p-ranav/argparse)
* [The GNU Readline Library](http://tiswww.case.edu/php/chet/readline/rltop.html)
* [eidheim/Simple-Web-Server](https://gitlab.com/eidheim/Simple-Web-Server)
