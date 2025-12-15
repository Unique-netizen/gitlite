#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "../include/Commit.h"
#include <string>

class Repository{
    static bool is_initialized();
    static std::string getHEAD();
    static Commit getCurrentCommit();
public:
    static void initialize();
    static void add(const std::string& filename);
    static void rm(const std::string& filename);
    static void commit(const std::string& message);
    static void log();
    static void globalLog();
    static void find(const std::string& message);
    static void checkoutFile(const std::string& filename);
    static void checkoutFileInCommit(const std::string& hash, const std::string& filename);
};
#endif // REPOSITORY_H
