#ifndef COMMAND_H
#define COMMAND_H
#include <string>


class Command{
public:
    static void init();
    static void add(const std::string& filename);
    static void rm(const std::string& filename);
    static void commit(const std::string& message);
};
#endif