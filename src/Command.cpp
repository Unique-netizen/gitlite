#include "../include/Command.h"
#include "../include/Utils.h"
#include "../include/Repository.h"
#include <string>

void Command::init(){
    Repository::initialize();
}

void Command::add(const std::string& filename){
    Repository::add(filename);
}

void Command::rm(const std::string& filename){
    Repository::rm(filename);
}

void Command::commit(const std::string& message){
    Repository::commit(message);
}

void Command::log(){
    Repository::log();
}

void Command::globalLog(){
    Repository::globalLog();
}

void Command::find(const std::string& message){
    Repository::find(message);
}

void Command::checkoutFile(const std::string& filename){
    Repository::checkoutFile(filename);
}

void Command::checkoutFileInCommit(const std::string& hash, const std::string& filename){
    Repository::checkoutFileInCommit(hash, filename);
}

void Command::checkoutBranch(const std::string& branchname){
    Repository::checkoutBranch(branchname);
}

void Command::status(){
    Repository::status();
}

void Command::branch(const std::string& branchname){
    Repository::branch(branchname);
}

void Command::rmBranch(const std::string& branchname){
    Repository::rmBranch(branchname);
}

void Command::reset(const std::string& hash){
    Repository::reset(hash);
}