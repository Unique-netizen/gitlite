#include "../include/Utils.h"
#include "../include/Blob.h"
#include <string>
#include <vector>

void Blob::createBlob(const std::vector<unsigned char>& blobContent){
    std::string hash = Utils::sha1(blobContent);
    std::string path = Utils::join(".gitlite/blobs/", hash);
    //if there is already a same blob
    if(Utils::isFile(path)) return;
    //write content
    Utils::writeContents(path, blobContent);
}

std::vector<unsigned char> Blob::readBlobContents(const std::string& blobHash){
    std::string path = Utils::join(".gitlite/blobs/", blobHash);
    Utils::readContents(path);
}

std::string Blob::readBlobContentsAsString(const std::string& blobHash){
    std::string path = Utils::join(".gitlite/blobs/", blobHash);
    Utils::readContentsAsString(path);
}