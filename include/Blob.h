#ifndef BLOB_H
#define BLOB_H
#include <string>
#include <vector>

class Blob{
public:
    static void createBlob(const std::vector<unsigned char>& blobContent);
    static std::vector<unsigned char> readBlobContents(const std::string& blobHash);
    static std::string readBlobContentsAsString(const std::string& blobHash);
};
#endif