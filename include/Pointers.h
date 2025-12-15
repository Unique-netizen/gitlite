#ifndef POINTERS_H
#define POINTERS_H
#include <string>
#include <vector>

class Pointers{
public:
    //HEAD
    static bool is_ref();
    static std::string get_ref();
    static void set_head(std::string& head);
    //branches
    static std::vector<std::string> getBranches();
};
#endif