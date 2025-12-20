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

void Command::merge(const std::string& branchname){
    Repository::merge(branchname);
}

void Command::addRemote(const std::string& remotename, const std::string& remotepath){
    Repository::addRemote(remotename, remotepath);
}

void Command::rmRemote(const std::string& remotename){
    Repository::rmRemote(remotename);
}

void Command::push(const std::string& remotename, const std::string& branchname){
    Repository::push(remotename, branchname);
}

void Command::fetch(const std::string& remotename, const std::string& branchname){
    Repository::fetch(remotename, branchname);
}

void Command::pull(const std::string& remotename, const std::string& branchname){
    Repository::pull(remotename, branchname);
}