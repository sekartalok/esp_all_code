#pragma once
#include "SD_ADD.h"


class SD_LIB{
    public:

    SD_LIB();

    int sd_begin(fs::FS &fs);
    std::string ls();
    int nano(const std::string path,const std::string data, bool append);
    int touch(const std::string path);
    std::string echo(const std::string path);   
    void cd(const std::string path);
    int mkdir(std::string path);
    int rmdir(std::string path);
    int rf(std::string path);

    private:
    static File root;
    static fs::FS *fs_mode;


};