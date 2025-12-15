#include "../include/Utils.h"
#include "../include/Repository.h"
#include "../include/Pointers.h"
#include "../include/Stage.h"
#include "../include/Commit.h"
#include "../include/Blob.h"

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <ctime>

//get commit hash of current HEAD
std::string Repository::getHEAD(){
    std::string head = Utils::readContentsAsString(".gitlite/HEAD");
    size_t pos = head.find("ref: ");
    if(pos != std::string::npos){
        std::string branch = head.substr(4);
        head = Utils::readContentsAsString(branch);
    }
    return head;
}
//get current commit
Commit Repository::getCurrentCommit(){
    std::string hash = getHEAD();
    std::string str = Utils::readContentsAsString(".gitlite/commits/" + hash);
    return Commit(str);
}


bool Repository::is_initialized(){
    return Utils::isDirectory(".gitlite");
}
void Repository::initialize(){
    //check whether there is already a .gitlite dir
    if(Repository::is_initialized()){
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    //create .gitlite
    Utils::createDirectories(".gitlite");
    Utils::createDirectories(".gitlite/branches");
    Utils::writeContents(".gitlite/HEAD", "ref: .gitlite/branches/master");
    Utils::writeContents(".gitlite/stage", "");//staged for addition and removal
    Utils::createDirectories(".gitlite/commits");
    Utils::createDirectories(".gitlite/blobs");
    //init commit
    Commit initialCommit;
    initialCommit.writeCommitFile();
    Utils::writeContents(".gitlite/branches/master", initialCommit.getHash());
}


void Repository::add(const std::string& filename){
    if(!Utils::isFile(filename)){
        Utils::exitWithMessage("File does not exist.");
    }

    Stage stage(Utils::readContentsAsString(".gitlite/stage"));

    std::vector<unsigned char> blobContent = Utils::readContents(filename);
    std::string hash = Utils::sha1(blobContent);
    Blob::createBlob(blobContent);

    //get current commit
    Commit currentCommit = getCurrentCommit();
    if(currentCommit.in_commit(filename) && currentCommit.getBlob(filename) == hash){//same as current commit
        if(stage.is_in_add(filename)){
            stage.deleteAdd(filename);
        }
    }else if(stage.is_in_rm(filename)){
        stage.deleteRm(filename);
        stage.add(filename, hash);
    }else{
        stage.add(filename, hash);
    }

    stage.writeStageFile();
}
void Repository::rm(const std::string& filename){
    Stage stage(Utils::readContentsAsString(".gitlite/stage"));

    //get current commit
    Commit currentCommit = getCurrentCommit();
    if(currentCommit.in_commit(filename)){//in current commit
        if(stage.is_in_add(filename)) stage.deleteAdd(filename);
        stage.rm(filename);
        Utils::restrictedDelete(filename);
    }else if(stage.is_in_add(filename)){
        stage.deleteAdd(filename);
    }else{
        Utils::exitWithMessage("No reason to remove the file.");
    }
}

void Repository::commit(const std::string& message){
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }
    //get parent commit from HEAD
    std::string HEAD = getHEAD();
    //construct current from parent
    std::string parentStr = Utils::readContentsAsString(".gitlite/commits/" + HEAD);
    Commit commit(parentStr, message);
    //stage
    Stage stage(Utils::readContentsAsString(".gitlite/stage"));
    std::map<std::string, std::string> addition = stage.getAdd();
    std::map<std::string, int> removal = stage.getRm();
    stage.clear();
    //modify commit
    if(addition.empty() && removal.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }
    commit.addFiles(addition);
    commit.rmFiles(removal);
    //writefile
    commit.writeCommitFile();
    //reset HEAD
    if(Pointers::is_ref()){
        std::string branch = Pointers::get_ref();
        //change branch only
        Utils::writeContents(".gitlite/branches/" + branch, commit.getHash());
    }else{
        Utils::writeContents(".gitlite/HEAD", commit.getHash());
    }
}

//log
//helper function to get format time
std::string formatTime(time_t timestamp){
    char buffer[40];
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y %z", &timeinfo);
    return std::string(buffer);
}
//helper function for format output
void formatOutput(const std::string& hash, const Commit& commit){
    std::string message = commit.getMessage();
    time_t timestamp = commit.getTimestamp();
    std::string outputTime = formatTime(timestamp);

    std::cout << "===\n";
    std::cout << "commit " << hash << "\n";
    std::cout << "Date: " << outputTime << "\n";
    std::cout << message << "\n\n";
}
//helper function to output branch
void outputBranch(const std::string hash){
    std::string str = Utils::readContentsAsString(".gitlite/commits/" + hash);
    Commit commit(str);
    formatOutput(hash, commit);
    if(commit.getMessage() == "initial commit") return;
    std::string nextHash = commit.getFirstParent();
    outputBranch(nextHash);
}
void Repository::log(){
    std::string hash = getHEAD();
    outputBranch(hash);
}
void Repository::globalLog(){
    std::vector<std::string> hashes = Utils::plainFilenamesIn(".gitlite/commits");
    for(auto& hash : hashes){
        std::string str = Utils::readContentsAsString(".gitlite/commits/" + hash);
        Commit commit(str);
        formatOutput(hash, commit);
    }
}

void Repository::find(const std::string& message){
    std::vector<std::string> hashes = Utils::plainFilenamesIn(".gitlite/commits");
    bool found = false;
    for(auto& hash : hashes){
        std::string str = Utils::readContentsAsString(".gitlite/commits/" + hash);
        Commit commit(str);
        if(commit.getMessage() == message){
            std::cout<<hash<<"\n";
            if(!found) found = true;
        }
    }
    if(!found){
        Utils::exitWithMessage("Found no commit with that message.");
    }
}


//checkout
void Repository::checkoutFile(const std::string& filename){
    Commit commit = getCurrentCommit();
    if(!commit.in_commit(filename)){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    std::string blob = commit.getBlob(filename);
    std::vector<unsigned char> content = Utils::readContents(".gitlite/blobs/" + blob);
    Utils::writeContents(filename, content);
}
void Repository::checkoutFileInCommit(const std::string& hash, const std::string& filename){
    //normal commit id
    if(hash.size() == 40){
        //whether commit exist
        if(!Utils::isFile(".gitlite/commits/" + hash)){
            Utils::exitWithMessage("No commit with that id exists.");
        }
        //whether have filename
        std::string str = Utils::readContentsAsString(".gitlite/commits/" + hash);
        Commit commit(str);
        if(!commit.in_commit(filename)){
            Utils::exitWithMessage("File does not exist in that commit.");
        }
        std::string blob = commit.getBlob(filename);
        std::vector<unsigned char> content = Utils::readContents(".gitlite/blobs/" + blob);
        Utils::writeContents(filename, content);
        return;
    }
    //short commit id
    std::vector<std::string> Hashes = Utils::plainFilenamesIn(".gitlite/commits");
    size_t length = hash.length();
    bool found = false;
    for(auto& Hash : Hashes){
        std::string shortHash = Hash.substr(0, length);
        if(shortHash == hash){
            if(!found) found = true;
            std::string str = Utils::readContentsAsString(".gitlite/commits/" + Hash);
            Commit commit(str);
            if(commit.in_commit(filename)){
                std::string blob = commit.getBlob(filename);
                std::vector<unsigned char> content = Utils::readContents(".gitlite/blobs/" + blob);
                Utils::writeContents(filename, content);
                return;
            }
        }
    }
    if(found){
        Utils::exitWithMessage("File does not exist in that commit.");
    }else{
        Utils::exitWithMessage("No commit with that id exists.");
    }
}