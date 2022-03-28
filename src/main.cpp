#include "interpreter.h"
#include "buffer.h"
#include "gateway.h"
#include "argparse/argparse.hpp"

BufferPool catalog_buf(64);
BufferPool data_buf(1024 * 16);

using namespace std;

void init() {
    #ifdef _WIN32
    mkdir(D_DIR);
    mkdir(I_DIR);
    mkdir(T_DIR);
    #else
    mkdir(D_DIR, 0755);
    mkdir(M_PREFIX, 0755);
    chdir(M_PREFIX);
    mkdir(__SCHEMA, 0755);
    mkdir(__INDEX, 0755);
    chdir("..");
    #endif
}

int main(int argc, const char* argv[]) {
    init();
    argparse::ArgumentParser program("MiniSQL");
    program.add_argument("--interface")
        .default_value(std::string("console"))
        .required()
        .help("display the square of a given integer");
    
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        cerr << err.what() << std::endl;
        cerr << program;
        exit(1);
    }
    if (program.get("--interface") == "web") { 
        init_web_interface();
    } else {
        std::ios::sync_with_stdio(false);
        Session s;
        s.start();
        s.listen();
    }
    return 0;
}