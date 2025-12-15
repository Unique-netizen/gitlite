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


//branches
std::vector<std::string> getBranches(){
    return Utils::plainFilenamesIn(".gitlite/branches");
}