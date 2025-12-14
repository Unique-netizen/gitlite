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