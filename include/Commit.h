#ifndef COMMIT_H
#define COMMIT_H
#include <string>
#include <ctime>
#include <vector>
#include <map>

class Commit{
    std::string hash;
    std::string message;
    time_t timestamp;
    std::vector<std::string> parents;
    std::map<std::string, std::string> files;

    //make all content to one string
    std::string tostring();

    //compute hash
    void computeHash();

public:
    Commit() : message{"initial commit"}, timestamp{0} {
        computeHash();
    }
    Commit(const std::string& str);//constructor from hash


    //get
    std::string getHash() const;
    std::string getMessage() const;
    time_t getTimestamp() const;
    std::string getFirstParent() const;
    std::vector<std::string> getParents() const;
    std::string getBlob(const std::string& filename);
    std::map<std::string, std::string> getFiles() const;
    //modify
    void setTime();
    void setMessage(const std::string& msg);
    void resetParent(const std::string& hash);
    void addParent(const std::string& parent);
    void addFiles(std::map<std::string, std::string>& addtion);
    void rmFiles(std::map<std::string, int>& removal);
    //find
    bool in_commit(const std::string& filename) const;

    //write to file
    void writeCommitFile();
};

#endif // COMMIT_H
