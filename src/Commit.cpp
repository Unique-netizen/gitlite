#include "../include/Utils.h"
#include "../include/Commit.h"
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include <map>
#include <sstream>

//constructor from parent, current time and current message
Commit::Commit(const std::string& str, std::string msg){
    message = msg;
    auto now = std::chrono::system_clock::now();
    timestamp = std::chrono::system_clock::to_time_t(now);
    std::istringstream stream(str);
    std::string first;
    while(stream >> first){
        std::string second;
        if(first == "message:"){
            stream >> second;
        }else if(first == "timestamp"){
            stream >> second;
        }else if(first == "parent:"){
            stream >> second;
            parents.push_back(second);
        }else{
            stream >> second;
            files[first] = second;
        }
    }
}
//constructor from hash
Commit::Commit(const std::string& str){
    std::istringstream stream(str);
    std::string first;
    while(stream >> first){
        std::string second;
        if(first == "message:"){
            stream >> second;
            message = second;
        }else if(first == "timestamp"){
            stream >> second;
            long long secondll = std::stoll(second);
            timestamp = static_cast<time_t>(secondll);
        }else if(first == "parent:"){
            stream >> second;
            parents.push_back(second);
        }else{
            stream >> second;
            files[first] = second;
        }
    }
}






//make all content to one string
std::string Commit::tostring(){
    std::string parents_string;
    for(int i = 0; i < parents.size(); i++){
        parents_string += ("parent: " + parents[i] + "\n");
    }
    std::string files_string;
    for(auto& file : files){
        files_string += ("\n" + file.first + " " + file.second);
    }
    std::string stringcontent = "message: " + message + "\n"
                                + "timestamp: " + std::to_string(timestamp) + "\n"
                                + parents_string
                                + files_string;
    return stringcontent;
}

//compute hash
void Commit::computeHash(){
    hash =  Utils::sha1(Commit::tostring());
}



//get
std::string Commit::getHash(){
    computeHash();//just in case
    return hash;
}
std::string Commit::getMessage() const{
    return message;
}
time_t Commit::getTimestamp() const{
    return timestamp;
}
std::string Commit::getFirstParent() const{
    return parents[0];
}
std::string Commit::getBlob(const std::string& filename){
    if(in_commit(filename)){
        return files[filename];
    }
}

//modify
void Commit::addFiles(std::map<std::string, std::string>& addition){
    for(auto& add : addition){
        files[add.first] = add.second;
    }
}
void Commit::rmFiles(std::map<std::string, int>& removal){
    for(auto& rm : removal){
        files.erase(rm.first);
    }
}

//find
bool Commit::in_commit(const std::string& filename) const{
    if(files.count(filename)) return true;
    return false;
}



//write to file
void Commit::writeCommitFile(){
    Commit::computeHash();
    std::string path = ".gitlite/commits/" + hash;
    std::vector<unsigned char> content = Utils::serialize(tostring());
    Utils::writeContents(path, content);
}