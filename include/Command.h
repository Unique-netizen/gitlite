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
    static void checkoutBranch(const std::string& branchname);
    static void status();
    static void branch(const std::string& branchname);
    static void rmBranch(const std::string& branchname);
    static void reset(const std::string& hash);
    static void merge(const std::string& branchname);
    static void addRemote(const std::string& remotename, const std::string& remotepath);
    static void rmRemote(const std::string& remotename);
    static void push(const std::string& remotename, const std::string& branchname);
    static void fetch(const std::string& remotename, const std::string& branchname);
    static void pull(const std::string& remotename, const std::string& branchname);
};
#endif