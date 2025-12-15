#ifndef COMMAND_H
#define COMMAND_H
#include <string>


class Command{
public:
    static void init();
    static void add(const std::string& filename);
    static void rm(const std::string& filename);
    static void commit(const std::string& message);
    static void log();
    static void globalLog();
    static void find(const std::string& message);
    static void checkoutFile(const std::string& filename);
    static void checkoutFileInCommit(const std::string& hash, const std::string& filename);
};
#endif