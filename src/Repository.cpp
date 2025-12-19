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
#include <algorithm>
#include <queue>


std::string Repository::getGitliteDir(){
    return ".gitlite";
}

//get commit hash of current HEAD
std::string Repository::getHEAD(){
    std::string head = Utils::readContentsAsString(".gitlite/HEAD");
    size_t pos = head.find("ref: ");
    if(pos != std::string::npos){
        std::string branch = head.substr(5);
        head = Utils::readContentsAsString(branch);
    }
    return head;
}
//get current commit
Commit Repository::getCurrentCommit(){
    std::string hash = getHEAD();
    return Commit(hash);
}
//get current stage
Stage Repository::getCurrentStage(){
    return Stage(Utils::readContentsAsString(".gitlite/stage"));
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

    Stage stage = getCurrentStage();

    std::vector<unsigned char> blobContent = Utils::readContents(filename);
    std::string hash = Utils::sha1(blobContent);
    Blob::createBlob(blobContent);

    //get current commit
    Commit currentCommit = getCurrentCommit();
    if(currentCommit.in_commit(filename) && currentCommit.getBlob(filename) == hash){//same as current commit
        if(stage.is_in_add(filename)){
            stage.deleteAdd(filename);
        }
        if(stage.is_in_rm(filename)){
            stage.deleteRm(filename);
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
    Stage stage = getCurrentStage();

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

    stage.writeStageFile();
}

void Repository::commit(const std::string& message, bool isMerge, std::string mergeParent){
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }
    //get parent commit from HEAD
    std::string parentHash = getHEAD();
    //construct current from parent
    Commit commit(parentHash);
    commit.setMessage(message);
    commit.setTime();
    if(mergeParent.empty()){
        commit.resetParent(parentHash);
    }else{
        commit.addParent(mergeParent);
    }
    //stage
    Stage stage = getCurrentStage();
    std::map<std::string, std::string> addition = stage.getAdd();
    std::map<std::string, int> removal = stage.getRm();
    //modify commit
    if(!isMerge && addition.empty() && removal.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }
    commit.addFiles(addition);
    commit.rmFiles(removal);
    stage.clear();
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
    std::vector<std::string> parents = commit.getParents();

    std::cout << "===\n";
    std::cout << "commit " << hash << "\n";
    if(parents.size() > 1){
        std::string p1 = parents[0].substr(0, 7);
        std::string p2 = parents[1].substr(0, 7);
        std::cout << "Merge: " << p1 << " " << p2 << "\n";
    }
    std::cout << "Date: " << outputTime << "\n";
    std::cout << message << "\n\n";
}
//helper function to output branch
void outputBranch(const std::string hash){
    Commit commit(hash);
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
        Commit commit(hash);
        formatOutput(hash, commit);
    }
}

void Repository::find(const std::string& message){
    std::vector<std::string> hashes = Utils::plainFilenamesIn(".gitlite/commits");
    bool found = false;
    for(auto& hash : hashes){
        Commit commit(hash);
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
    std::vector<unsigned char> content = Blob::readBlobContents(blob);
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
        Commit commit(hash);
        if(!commit.in_commit(filename)){
            Utils::exitWithMessage("File does not exist in that commit.");
        }
        std::string blob = commit.getBlob(filename);
        std::vector<unsigned char> content = Blob::readBlobContents(blob);
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
            Commit commit(Hash);
            if(commit.in_commit(filename)){
                std::string blob = commit.getBlob(filename);
                std::vector<unsigned char> content = Blob::readBlobContents(blob);
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
//helper function to checkout a commit (check untracked file, delete and write, and clear stage)
void Repository::checkoutCommit(const std::string& hash){
    Stage stage = getCurrentStage();
    Commit commit(hash);
    std::map<std::string, std::string> files = commit.getFiles();
    std::vector<std::string> files_in_workdir = Utils::plainFilenamesIn(".");
    Commit current_commit = getCurrentCommit();

    //check untracked file
    std::map<std::string, int> untrackedFiles;
    for(auto& file : files_in_workdir){
        if(stage.is_in_rm(file)){
            untrackedFiles[file] = 1;
            continue;
        }
        if(!stage.is_in_add(file) && !current_commit.in_commit(file)){
            untrackedFiles[file] = 1;
        }
    }
    bool untracked = false;
    for(auto& untrackedFile : untrackedFiles){
        if(commit.in_commit(untrackedFile.first)){
            untracked = true;
            break;
        }
    }
    if(untracked){
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
    }

    //delete
    for(auto& file : files_in_workdir){
        if(untrackedFiles.count(file)) continue;
        Utils::restrictedDelete(file);
    }
    //write
    for(auto& file : files){
        std::vector<unsigned char> content = Blob::readBlobContents(file.second);
        Utils::writeContents(file.first, content);
    }

    stage.clear();
}
void Repository::checkoutBranch(const std::string& branchname){
    if(!Utils::isFile(".gitlite/branches/" + branchname)){
        Utils::exitWithMessage("No such branch exists.");
    }
    if(Pointers::is_ref() && Pointers::get_ref() == branchname){
        Utils::exitWithMessage("No need to checkout the current branch.");
    }

    std::string commithash = Utils::readContentsAsString(".gitlite/branches/" + branchname);
    checkoutCommit(commithash);
    Utils::writeContents(".gitlite/HEAD", "ref: .gitlite/branches/" + branchname);
}


//status
void Repository::status(){
    //branches
    std::vector<std::string> branches = Pointers::getBranches();
    std::sort(branches.begin(), branches.end());
    std::string current = "";
    if(Pointers::is_ref()){
        current = Pointers::get_ref();
    }
    std::cout<<"=== Branches ===\n";
    for(auto& branch : branches){
        if(branch == current){
            std::cout<<"*"<<branch<<"\n";
        }else{
            std::cout<<branch<<"\n";
        }
    }
    std::cout<<"\n";

    //stage
    Stage stage = getCurrentStage();
    //staged files
    std::map<std::string, std::string> addition = stage.getAdd();//map is ordered
    std::cout<<"=== Staged Files ===\n";
    for(auto& add : addition){
        std::cout<<add.first<<"\n";
    }
    std::cout<<"\n";
    //removed files
    std::map<std::string, int> removal = stage.getRm();
    std::cout<<"=== Removed Files ===\n";
    for(auto& rm : removal){
        std::cout<<rm.first<<"\n";
    }
    std::cout<<"\n";

    //Modifications Not Staged For Commit
    Commit commit = getCurrentCommit();
    std::map<std::string, std::string> files_in_commit = commit.getFiles();
    std::vector<std::string> files_in_workdir= Utils::plainFilenamesIn(".");
    std::map<std::string, int> modNotStaged;//0 marks delete, 1 marks modify
    std::map<std::string, std::string> file_contents_in_workdir;
    for(auto& file : files_in_workdir){
        std::string content = Utils::readContentsAsString(file);
        file_contents_in_workdir[file] = content;
    }
    for(auto& file : files_in_commit){
        std::string name = file.first;
        if(file_contents_in_workdir.count(name) && file_contents_in_workdir[name] != files_in_commit[name] && stage.is_in_add(name)){
            modNotStaged[name] = 1;
        }
        if(!file_contents_in_workdir.count(name) && !stage.is_in_rm(name)){
            modNotStaged[name] = 0;
        }
    }
    for(auto& file : addition){
        std::string name = file.first;
        //second is blob hash
        if(file_contents_in_workdir.count(name)){
            std::string content_in_addition = Blob::readBlobContentsAsString(addition[name]);
            if(content_in_addition != file_contents_in_workdir[name]){
                modNotStaged[name] = 1;
            }
        }else{
            modNotStaged[name] = 0;
        }
    }
    std::cout<<"=== Modifications Not Staged For Commit ===\n";
    for(auto& file : modNotStaged){
        std::cout<<file.first;
        if(file.second == 0){
            std::cout<<" (deleted)\n";
        }else{
            std::cout<<" (modified)\n";
        }
    }
    std::cout<<"\n";

    //Untracked Files
    std::vector<std::string> untrackedFiles;
    for(auto& file : files_in_workdir){
        if(stage.is_in_rm(file)){
            untrackedFiles.push_back(file);
            continue;
        }
        if(!stage.is_in_add(file) && !commit.in_commit(file)){
            untrackedFiles.push_back(file);
        }
    }
    std::sort(untrackedFiles.begin(), untrackedFiles.end());
    std::cout<<"=== Untracked Files ===\n";
    for(auto& untrackedFile : untrackedFiles){
        std::cout<<untrackedFile<<"\n";
    }
}


void Repository::branch(const std::string& branchname){
    std::string path = Utils::join(".gitlite/branches/", branchname);
    if(Utils::isFile(path)){
        Utils::exitWithMessage("A branch with that name already exists.");
    }
    std::string hash = getHEAD();
    Utils::writeContents(path, hash);
}
void Repository::rmBranch(const std::string& branchname){
    std::string path = Utils::join(".gitlite/branches/", branchname);
    if(!Utils::isFile(path)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    if(Pointers::is_ref() && Pointers::get_ref() == branchname){
        Utils::exitWithMessage("Cannot remove the current branch.");
    }
    Utils::restrictedDelete(path);
}
void Repository::reset(const std::string& hash){
    std::string commit_path = Utils::join(".gitlite/commits/", hash);
    if(!Utils::isFile(commit_path)){
        Utils::exitWithMessage("No commit with that id exists.");
    }
    checkoutCommit(hash);
    if(Pointers::is_ref()){
        std::string ref = Pointers::get_ref();
        Utils::writeContents(".gitlite/branches/" + ref, hash);
        return;
    }
    Utils::writeContents(".gitlite/HEAD", hash);
}


//merge
//helper function to get LCA
Commit getLCA(const Commit& current, const Commit& given){
    std::map<std::string, int> ancestors;
    std::queue<std::pair<std::string, int>> q;
    q.push({current.getHash(), 1});
    q.push({given.getHash(), 2});
    while(!q.empty()){
        std::string hash = q.front().first;
        int side = q.front().second;
        q.pop();
        if(ancestors.count(hash)){
            if(ancestors[hash] == side) continue;
            else return Commit(hash);
        }
        ancestors.insert({hash, side});
        Commit c(hash);
        std::vector<std::string> parents = c.getParents();
        for(auto& parent : parents){
            q.push({parent, side});
        }
    }
}
//helper function to compare changes of a commit with LCA
void compare(std::map<std::string, std::string>& LCA_files, const Commit& commit, std::map<std::string, std::string>& modify, std::map<std::string, std::string>& same, std::map<std::string, std::string>& notin, std::map<std::string, std::string>& newin){
    std::map<std::string, std::string> commit_files = commit.getFiles();
    for(auto& file : commit_files){
        std::string name = file.first;
        std::string blobHash = file.second;
        if(LCA_files.count(name)){//in LCA
            if(blobHash == LCA_files[name]) same[name] = blobHash;//same as LCA
            else modify[name] = blobHash;//modified
        }else{//new in the commit
            newin[name] = blobHash;
        }
    }
    for(auto& file : LCA_files){//check for LCA files that are not in the commit
        if(!commit_files.count(file.first)) notin[file.first] = file.second;
    }
}
//helper function to write conflict file
void writeConflict(const std::string& filepath, const std::string& current_content, const std::string& given_content){
    std::string content;
    content += "<<<<<<< HEAD\n";
    content += current_content;
    if(!current_content.empty() && current_content.back() != '\n'){
        content += "\n";
    }
    content += "=======\n";
    content += given_content;
    if(!given_content.empty() && given_content.back() != '\n'){
        content += "\n";
    }
    content += ">>>>>>>\n";
    Utils::writeContents(filepath, content);
}
void Repository::merge(const std::string& branchname){
    Stage stage = getCurrentStage();
    std::map<std::string, std::string> addition = stage.getAdd();
    std::map<std::string, int> removal = stage.getRm();
    if(!addition.empty() || !removal.empty()){
        Utils::exitWithMessage("You have uncommitted changes.");
    }
    std::string branchPath = Utils::join(".gitlite/branches/", branchname);
    if(!Utils::isFile(branchPath)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    std::string current_branch = Pointers::get_ref();
    if(branchname == current_branch){
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }

    std::string given_commit_hash = Utils::readContentsAsString(branchPath);
    Commit current = getCurrentCommit();
    Commit given(given_commit_hash);
    Commit LCA = getLCA(current, given);

    if(LCA.getHash() == given.getHash()) Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
    if(LCA.getHash() == current.getHash()){
        checkoutCommit(given_commit_hash);
        Utils::writeContents(".gitlite/branches/" + current_branch, given_commit_hash);
        Utils::exitWithMessage("Current branch fast-forwarded.");
    }

//compare current and given with LCA
    std::map<std::string, std::string> LCA_files = LCA.getFiles();
    std::map<std::string, std::string> modify_in_current, same_in_current, not_in_current, new_in_current;
    std::map<std::string, std::string> modify_in_given, same_in_given, not_in_given, new_in_given;
    compare(LCA_files, current, modify_in_current, same_in_current, not_in_current, new_in_current);
    compare(LCA_files, given, modify_in_given, same_in_given, not_in_given, new_in_given);

    bool conflict = false;
//cases (2 3 4 7 are doing nothing)
    //for files in LCA
    for(auto& file : LCA_files){
        std::string name = file.first;
        if(modify_in_given.count(name) && same_in_current.count(name)){//case 1
            checkoutFileInCommit(given_commit_hash, name);
            add(name);
        }else if(same_in_current.count(name) && not_in_given.count(name)){//case 6
            rm(name);
        }else if(modify_in_current.count(name) && modify_in_given.count(name)){//conflicts
            std::string current_blob_hash = modify_in_current[name];
            std::string given_blob_hash = modify_in_given[name];
            if(current_blob_hash == given_blob_hash) continue;
            conflict = true;
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
        }else if(modify_in_current.count(name) && not_in_given.count(name)){
            conflict = true;
            std::string current_blob_hash = modify_in_current[name];
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = "";
            writeConflict(name, current_content, given_content);
        }else if(modify_in_given.count(name) && not_in_current.count(name)){
            conflict = true;
            std::string current_content = "";
            std::string given_blob_hash = modify_in_given[name];
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
        }
    }
    //for new files
    for(auto& file : new_in_given){
        std::string name = file.first;
        if(!new_in_current.count(name)){//case 5
            checkoutFileInCommit(given_commit_hash, name);
            add(name);
        }else{//conflict
            std::string current_blob_hash = new_in_current[name];
            std::string given_blob_hash = new_in_given[name];
            if(current_blob_hash == given_blob_hash) continue;
            conflict = true;
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
        }
    }

//commit
    std::string current_commit_hash = getHEAD();
    std::string message = "Merged " + branchname + " into " + Pointers::get_ref() + ".";
    commit(message, true, given_commit_hash);

    if(conflict){
        Utils::message("Encountered a merge conflict.");
    }
}