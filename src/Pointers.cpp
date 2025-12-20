#include "../include/Utils.h"
#include "../include/Pointers.h"
#include <string>
#include <vector>

//HEAD
bool Pointers::is_ref(){
    std::string head = Utils::readContentsAsString(".gitlite/HEAD");
    size_t pos = head.find("ref: ");
    if(pos == std::string::npos) return false;
    return true;
}
std::string Pointers::get_ref(){
    if(is_ref()){
        std::string head = Utils::readContentsAsString(".gitlite/HEAD");
        size_t pos = head.find_last_of("/");
        return head.substr(pos + 1);
    }
}
void Pointers::set_ref(const std::string& branchname, const std::string& repoPath){
    std::string path = Utils::join(repoPath, "HEAD");
    Utils::writeContents(path, "ref: .gitlite/branches/" + branchname);
}

//branches
std::vector<std::string> Pointers::getBranches(){
    return Utils::plainFilenamesIn(".gitlite/branches");
}