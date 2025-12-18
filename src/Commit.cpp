#include "../include/Utils.h"
#include "../include/Commit.h"
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include <map>
#include <sstream>

//constructor from hash
Commit::Commit(const std::string& commitHash){
    std::string commitStr = Utils::readContentsAsString(".gitlite/commits/" + commitHash);
    size_t posn = commitStr.find("\n");
    message = commitStr.substr(9, posn - 9);//"message: " length is 9
    std::istringstream stream(commitStr.substr(posn + 1));
    std::string first;
    while(stream >> first){
        std::string second;
        if(first == "timestamp:"){
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
        files_string += (file.first + " " + file.second + "\n");
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
std::string Commit::getHash() const{
    //computeHash();//just in case
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
std::vector<std::string> Commit::getParents() const{
    return parents;
}
std::string Commit::getBlob(const std::string& filename){
    if(in_commit(filename)){
        return files[filename];
    }
}
std::map<std::string, std::string> Commit::getFiles() const{
    return files;
}

//modify
void Commit::setTime(){
    auto now = std::chrono::system_clock::now();
    timestamp = std::chrono::system_clock::to_time_t(now);
}
void Commit::resetParent(const std::string& hash){
    parents.clear();
    parents.push_back(hash);
}
void Commit::addParent(const std::string& parent){
    parents.push_back(parent);
}
void Commit::setMessage(const std::string& msg){
    message = msg;
}
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
    computeHash();
    std::string path = ".gitlite/commits/" + hash;
    std::vector<unsigned char> content = Utils::serialize(tostring());
    Utils::writeContents(path, content);
}