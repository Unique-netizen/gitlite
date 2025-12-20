#include "../include/Utils.h"
#include "../include/Stage.h"

#include <string>
#include <vector>


//constructor
Stage::Stage(const std::string& str) : stageContent{str} {
    std::istringstream stream(str);
    std::string first;
    while(stream >> first){
        if(first[0] == '-'){//removal
            removal[first.substr(1)] = 1;
        }else{//addition
            std::string second;
            stream >> second;
            addition[first] = second;
        }
    }
}

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
    std::vector<unsigned char> content = Utils::serialize(stageContent);
    Utils::writeContents(".gitlite/stage", content);
}

void Stage::clear(){
    Utils::writeContents(".gitlite/stage", "");
}