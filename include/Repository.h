#ifndef REPOSITORY_H
#define REPOSITORY_H

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
};
#endif // REPOSITORY_H
