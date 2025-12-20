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
#include <queue>
#include <filesystem>


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
//get untracked files
std::map<std::string, int> Repository::getUntrackedFiles(){
    std::vector<std::string> file_names_in_workdir = Utils::plainFilenamesIn(".");
    std::map<std::string, int> untrackedfiles;
    Stage stage = getCurrentStage();
    Commit commit = getCurrentCommit();
    for(auto& name : file_names_in_workdir){
        if(stage.is_in_rm(name)){
            untrackedfiles[name] = 1;
            continue;
        }
        if(!stage.is_in_add(name) && !commit.in_commit(name)){
            untrackedfiles[name] = 1;
        }
    }
    return untrackedfiles;
}

bool Repository::is_initialized(){
    return Utils::isDirectory(".gitlite");
}
void Repository::init(){
    //check whether there is already a .gitlite dir
    if(Repository::is_initialized()){
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    //create .gitlite
    Utils::createDirectories(".gitlite");
    Utils::createDirectories(".gitlite/branches");
    Utils::writeContents(".gitlite/HEAD", "ref: .gitlite/branches/master");
    Utils::writeContents(".gitlite/stage", "");//stage for addition and removal
    Utils::createDirectories(".gitlite/commits");
    Utils::createDirectories(".gitlite/blobs");
    Utils::createDirectories(".gitlite/remotes");
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

void Repository::commit(const std::string& message, bool isMerge, const std::string& mergeParent){
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }
    //get parent commit from HEAD
    std::string parentHash = getHEAD();
    //construct current from parent
    Commit commit(parentHash);
    commit.setMessage(message);
    commit.setTime();
    commit.resetParent(parentHash);
    if(!mergeParent.empty()){
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
static std::string formatTime(time_t timestamp){
    char buffer[40];
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y %z", &timeinfo);
    return std::string(buffer);
}
//helper function for format output
static void formatOutput(const std::string& hash, const Commit& commit){
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
static void outputBranch(const std::string hash){
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
    std::vector<std::string> file_names_in_workdir = Utils::plainFilenamesIn(".");
    Commit current_commit = getCurrentCommit();

    //check untracked file
    std::map<std::string,int> untrackedfiles = getUntrackedFiles();
    bool willCover = false;
    for(auto& file : untrackedfiles){
        if(commit.in_commit(file.first)){
            willCover = true;
            break;
        }
    }
    if(willCover){
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
    }

    //delete
    for(auto& name : file_names_in_workdir){
        if(untrackedfiles.count(name)) continue;
        Utils::restrictedDelete(name);
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
    Pointers::set_ref(branchname);
}


//status
void Repository::status(){
    //branches
    std::vector<std::string> branches = Pointers::getBranches();
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
    std::map<std::string, std::string> files_in_commit = commit.getFiles();//second is hash of content
    std::vector<std::string> files_names_in_workdir= Utils::plainFilenamesIn(".");
    std::map<std::string, int> modNotStaged;//0 marks delete, 1 marks modify
    std::map<std::string, std::string> files_in_workdir;//second is hash of content
    for(auto& file : files_names_in_workdir){
        std::vector <unsigned char> content = Utils::readContents(file);
        files_in_workdir[file] = Utils::sha1(content);
    }
    for(auto& file : files_in_commit){
        std::string name = file.first;
        if(files_in_workdir.count(name) && files_in_workdir[name] != files_in_commit[name] && !stage.is_in_add(name)){
            modNotStaged[name] = 1;
        }
        if(!files_in_workdir.count(name) && !stage.is_in_rm(name)){
            modNotStaged[name] = 0;
        }
    }
    for(auto& file : addition){
        std::string name = file.first;
        if(files_in_workdir.count(name)){
            if(file.second != files_in_workdir[name]){
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
    std::map<std::string, int> untrackedFiles = getUntrackedFiles();
    std::cout<<"=== Untracked Files ===\n";
    for(auto& untrackedFile : untrackedFiles){
        std::cout<<untrackedFile.first<<"\n";
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
    Utils::simpleDelete(path);
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
static Commit getLCA(const Commit& current, const Commit& given){
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
static void compare(std::map<std::string, std::string>& LCA_files, const Commit& commit, std::map<std::string, std::string>& modify, std::map<std::string, std::string>& same, std::map<std::string, std::string>& notin, std::map<std::string, std::string>& newin){
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
static void writeConflict(const std::string& filepath, const std::string& current_content, const std::string& given_content){
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
    std::map<std::string, int> untrackedFiles = getUntrackedFiles();
    //check for cover of untracked files
    for(auto& file : LCA_files){
        std::string name = file.first;
        if(modify_in_given.count(name) && same_in_current.count(name)){//case 1
            if(untrackedFiles.count(name)) Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }else if(same_in_current.count(name) && not_in_given.count(name)){//case 6
            if(untrackedFiles.count(name)) Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }
    for(auto& file : new_in_given){
        std::string name = file.first;
        if(!new_in_current.count(name)){//case 5
            if(untrackedFiles.count(name)) Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }
    //for files in LCA
    for(auto& file : LCA_files){
        std::string name = file.first;
        if(modify_in_given.count(name) && same_in_current.count(name)){//case 1
            checkoutFileInCommit(given_commit_hash, name);//may cause cover of untracked files
            add(name);
        }else if(same_in_current.count(name) && not_in_given.count(name)){//case 6
            rm(name);//may cause cover of untracked files
        }else if(modify_in_current.count(name) && modify_in_given.count(name)){//conflicts
            std::string current_blob_hash = modify_in_current[name];
            std::string given_blob_hash = modify_in_given[name];
            if(current_blob_hash == given_blob_hash) continue;
            conflict = true;
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
            add(name);
        }else if(modify_in_current.count(name) && not_in_given.count(name)){
            conflict = true;
            std::string current_blob_hash = modify_in_current[name];
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = "";
            writeConflict(name, current_content, given_content);
            add(name);
        }else if(modify_in_given.count(name) && not_in_current.count(name)){
            conflict = true;
            std::string current_content = "";
            std::string given_blob_hash = modify_in_given[name];
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
            add(name);
        }
    }
    //for new files
    for(auto& file : new_in_given){
        std::string name = file.first;
        if(!new_in_current.count(name)){//case 5
            checkoutFileInCommit(given_commit_hash, name);//may cause cover of untracked files
            add(name);
        }else{//conflict
            std::string current_blob_hash = new_in_current[name];
            std::string given_blob_hash = new_in_given[name];
            if(current_blob_hash == given_blob_hash) continue;
            conflict = true;
            std::string current_content = Blob::readBlobContentsAsString(current_blob_hash);
            std::string given_content = Blob::readBlobContentsAsString(given_blob_hash);
            writeConflict(name, current_content, given_content);
            add(name);
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



//remote
//helper function to get all commits needed to copy
static bool getFutureCommits(const std::string& current_commit_hash, const std::string& history_commit_hash, std::map<std::string, int>& futureCommits, const std::string& repoPath){
    Commit current(current_commit_hash, repoPath);
    std::vector<std::string> parents = current.getParents();
    bool found = false;
    for(auto& parent : parents){
        if(parent == history_commit_hash){
            found = true;
            futureCommits[current_commit_hash] = 1;
        }
        if(getFutureCommits(parent, history_commit_hash, futureCommits, repoPath)){
            futureCommits[current_commit_hash] = 1;
        }
    }
    return found;
}
//helper function to copy commit files and blob files
static void copy_files(const std::map<std::string, int>& commits, const std::string& from, const std::string& to){
    for(auto& single_commit : commits){
        std::string commit_hash = single_commit.first;
        std::string from_commit_path = Utils::join(from, "commits", commit_hash);
        std::string to_commit_path = Utils::join(to, "commits", commit_hash);
        std::filesystem::copy_file(from_commit_path, to_commit_path, std::filesystem::copy_options::skip_existing);
        Commit commit(commit_hash, from);
        std::map<std::string, std::string> files_in_commit = commit.getFiles();
        for(auto& file : files_in_commit){
            std::string blob_hash = file.second;
            std::string from_blob_path = Utils::join(from, "blobs", blob_hash);
            std::string to_blob_path = Utils::join(to, "blobs", blob_hash);
            std::filesystem::copy_file(from_blob_path, to_blob_path, std::filesystem::copy_options::skip_existing);
        }
    }
}
void Repository::addRemote(const std::string& remotename, const std::string& remotepath){
    std::string remote = Utils::join(".gitlite/remotes/", remotename);
    if(Utils::isFile(remote)) Utils::exitWithMessage("A remote with that name already exists.");
    Utils::writeContents(remote, remotepath);
}
void Repository::rmRemote(const std::string& remotename){
    std::string remote = Utils::join(".gitlite/remotes/", remotename);
    if(!Utils::isFile(remote)) Utils::exitWithMessage("A remote with that name does not exist.");
    Utils::simpleDelete(remote);
}
void Repository::push(const std::string& remotename, const std::string& branchname){
    std::string remote = Utils::join(".gitlite/remotes/", remotename);
    std::string remotepath = Utils::readContentsAsString(remote);
    if(!Utils::isDirectory(remotepath)) Utils::exitWithMessage("Remote directory not found.");
    std::string remoteBranchPath = Utils::join(remotepath, "branches", branchname);
    if(Utils::isFile(remoteBranchPath)){
        std::map<std::string, int> futureCommits;
        std::string remoteBranchHead = Utils::readContentsAsString(remoteBranchPath);
        std::string current_commit_hash = getHEAD();

        getFutureCommits(current_commit_hash, remoteBranchHead, futureCommits, ".gitlite");
        if(futureCommits.empty()){
            Utils::exitWithMessage("Please pull down remote changes before pushing.");
        }

        copy_files(futureCommits, ".gitlite", remotepath);

        Utils::writeContents(remoteBranchPath, current_commit_hash);
        Pointers::set_ref(branchname, remotepath);
    }else{
        std::map<std::string, int> futureCommits;
        Commit initialCommit;//initialCommit have a fixed hash
        std::string history_commit_hash = initialCommit.getHash();
        std::string current_commit_hash = getHEAD();

        getFutureCommits(current_commit_hash, history_commit_hash, futureCommits, ".gitlite");

        copy_files(futureCommits, ".gitlite", remotepath);

        Utils::writeContents(remoteBranchPath, current_commit_hash);
        Pointers::set_ref(branchname, remotepath);
    }
}
void Repository::fetch(const std::string& remotename, const std::string& branchname){
    std::string remote = Utils::join(".gitlite/remotes/", remotename);
    std::string remotepath = Utils::readContentsAsString(remote);
    if(!Utils::isDirectory(remotepath)) Utils::exitWithMessage("Remote directory not found.");

    std::string remoteBranchPath = Utils::join(remotepath, "branches", branchname);
    if(!Utils::isFile(remoteBranchPath)) Utils::exitWithMessage("That remote does not have that branch.");

    Commit initialCommit;
    std::string history_commit_hash = initialCommit.getHash();
    std::string current_commit_hash = Utils::readContentsAsString(remoteBranchPath);
    std::map<std::string, int> futureCommits;
    
    getFutureCommits(current_commit_hash, history_commit_hash, futureCommits, remotepath);

    copy_files(futureCommits, remotepath, ".gitlite");

    std::string branch = Utils::join(".gitlite/branches", remotename, branchname);
    Utils::writeContents(branch, current_commit_hash);
}
void Repository::pull(const std::string& remotename, const std::string& branchname){
    fetch(remotename, branchname);
    std::string mergeBranch = Utils::join(remotename, branchname);
    merge(mergeBranch);
}