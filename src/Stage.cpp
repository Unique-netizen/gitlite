#include "../include/Utils.h"
#include "../include/Stage.h"

#include <string>
#include <vector>

std::map<std::string, std::string> Stage::getAdd(){
    return addition;
}
std::map<std::string, int> Stage::getRm(){
    return removal;
}


bool Stage::is_in_add(const std::string& filename){
    if(addition.count(filename)) return true;
    return false;
}
bool Stage::is_in_rm(const std::string& filename){
    if(removal.count(filename)) return true;
    return false;
}

void Stage::add(const std::string& filename, const std::string& hash){
    addition[filename] = hash;
}
void Stage::rm(const std::string& filename){
    removal[filename] = 1;
}

void Stage::deleteAdd(const std::string& filename){
    addition.erase(filename);
}
void Stage::deleteRm(const std::string& filename){
    removal.erase(filename);
}


void Stage::writeStageFile(){
    std::string newContent;
    for(auto& add : addition){
        newContent += (add.first + " " + add.second + "\n");
    }
    for(auto& rm : removal){
        newContent += ("-" + rm.first + "\n");
    }
    stageContent = newContent;
    Utils::writeContents(".gitlite/stage", stageContent);
}

void Stage::clear(){
    Utils::writeContents(".gitlite/stage", "");
}