#ifndef STAGE_H
#define STAGE_H
#include <string>
#include <vector>
#include <map>
#include <sstream>
class Stage{
    std::string stageContent;
    std::map<std::string, std::string> addition;
    std::map<std::string, int> removal;


public:
    //constructor
    Stage(const std::string& str);

    //get
    std::map<std::string, std::string> getAdd();
    std::map<std::string, int> getRm();
    

    bool is_in_add(const std::string& filename);
    bool is_in_rm(const std::string& filename);

    void add(const std::string& filename, const std::string& hash);
    void rm(const std::string& filename);

    void deleteAdd(const std::string& filename);
    void deleteRm(const std::string& filename);


    //write stage file
    void writeStageFile();

    //clear stage
    void clear();
};
#endif