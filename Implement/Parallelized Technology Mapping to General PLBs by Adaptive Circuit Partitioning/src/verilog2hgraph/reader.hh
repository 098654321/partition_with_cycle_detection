#ifndef READER_HH
#define READER_HH


#include "config.hh"


namespace parser {

class Reader {
public:
    Reader() = default;

    Module json2module(const std::string& filename);
    void test_read();

private:
    Module _module;
};

}


#endif  // READER_HH
