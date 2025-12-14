#include "../include/Utils.h"
#include "../include/Blob.h"
#include <string>

void Blob::createBlob(const std::vector<unsigned char>& blobContent){
    std::string hash = Utils::sha1(blobContent);
    std::string path = ".gitlite/blobs/" + hash;
    //if there is already a same blob
    if(Utils::isFile(path)) return;
    //write content
    Utils::writeContents(path, blobContent);
}