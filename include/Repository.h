#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "../include/Commit.h"
#include "../include/Stage.h"
#include <string>

class Repository{
    static bool is_initialized();
    static std::string getHEAD();
    static Commit getCurrentCommit();
    static Stage getCurrentStage();
    static void checkoutCommit(const std::string& hash);//helper function to checkout a commit
public:
    static void initialize();
    static void add(const std::string& filename);
    static void rm(const std::string& filename);
    static void commit(const std::string& message, bool is_merge = false, std::string mergeParent = "");
    static void log();
    static void globalLog();
    static void find(const std::string& message);
    static void checkoutFile(const std::string& filename);
    static void checkoutFileInCommit(const std::string& hash, const std::string& filename);
    static void checkoutBranch(const std::string& branchname);
    static void status();
    static void branch(const std::string& branchname);
    static void rmBranch(const std::string& branchname);
    static void reset(const std::string& hash);
    static void merge(const std::string& branchname);
};
#endif // REPOSITORY_H
