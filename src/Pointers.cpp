#include "../include/Utils.h"
#include "../include/Pointers.h"
#include <string>
#include <vector>
#include <algorithm>

//HEAD
//if is ref, HEAD file starts with ref: 
//if is reference to remote branch, HEAD file starts with remote_ref: 
bool Pointers::is_ref(){
    std::string head = Utils::readContentsAsString(".gitlite/HEAD");
    size_t pos = head.find("ref: ");
    if(pos == std::string::npos) return false;
    return true;
}
std::string Pointers::get_ref(){
    if(is_ref()){
        std::string head = Utils::readContentsAsString(".gitlite/HEAD");
        std::string preffix = "ref: .gitlite/branches/";
        size_t length = preffix.size();
        return head.substr(length);
    }
}
void Pointers::set_ref(const std::string& branchname, const std::string& repoPath){
    std::string path = Utils::join(repoPath, "HEAD");
    Utils::writeContents(path, "ref: .gitlite/branches/" + branchname);
}

//branches
std::vector<std::string> Pointers::getBranches(){
    std::vector<std::string> branches =  Utils::plainFilenamesIn(".gitlite/branches");
    std::vector<std::string> remoteNames = Utils::DirnamesIn(".gitlite/branches");
    for(auto& remoteName : remoteNames){
        std::vector<std::string> remoteBranchNames = Utils::plainFilenamesIn(".gitlite/branches/" + remoteName);
        for(auto& remoteBranchName : remoteBranchNames){
            branches.push_back(remoteName + "/" + remoteBranchName);
        }
    }
    std::sort(branches.begin(), branches.end());
    return branches;
}