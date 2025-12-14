#include "../include/Utils.h"
#include "../include/Repository.h"
#include "../include/Pointers.h"
#include "../include/Stage.h"
#include "../include/Commit.h"
#include "../include/Blob.h"

#include <string>
#include <map>

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
    Commit commit(Utils::readContentsAsString(".gitlite/commits/" + HEAD), message);
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