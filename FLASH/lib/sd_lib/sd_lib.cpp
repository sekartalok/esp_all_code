#include "sd_lib.h"

File SD_LIB::root;
fs::FS *SD_LIB::fs_mode;

SD_LIB::SD_LIB(){}  

int SD_LIB::sd_begin(fs::FS &fs){
 fs_mode = &fs;
 SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS) ;
 if(!SD.begin(SD_CS)){
  return 1;

 }
 return 0;
}

std::string SD_LIB::ls(){

  std::string result;
  unsigned int i;
 
  
  if(!root){
    return "No root dir";
  }
  File file = root.openNextFile();
  if(!file){
    return "no dir";
  }
  i=0;
  while(i < 500 && file){
    result += file.name();
    result += "\n";
    file = root.openNextFile();
    i++;
  }
  file.close();

  return result;
  
} 

int SD_LIB::nano(const std::string path,const std::string data, bool append){
  File file ;
  if(!append){
    file = fs_mode->open(path.c_str(),FILE_WRITE);
  }
  else{
    file = fs_mode->open(path.c_str(),"a");
  }
 
  if(!file){
    return 1;
  }
  file.println(data.c_str());
  file.close();
  return 0;
}

int SD_LIB::touch(const std::string path){
  if(fs_mode->exists(path.c_str())){
    return 2;
  }
  File file = fs_mode->open(path.c_str(),FILE_WRITE);
  if(!file){
    return 1;
  }
  return 0;
}

std::string SD_LIB::echo(const std::string path){
  File file = fs_mode->open(path.c_str());
  if(!file){
    return "file not exist";
  }
  if(file.isDirectory()){
    return "its a directory";
  }
  return std::string(file.readString().c_str());
}

File SD_LIB::cd(File root,const std::string path){
  File file = fs_mode->open(path.c_str());
  if(!file){
    return root;
    
  }
  root.close();
  return file;
}

int SD_LIB::rmdir(std::string path){

  if(!fs_mode->exists(path.c_str())){
    return 2;

  }
  if(!fs_mode->rmdir(path.c_str())){
    return 1;
  }
  return 0;
}

int SD_LIB::mkdir(std::string path){
  if(fs_mode->exists(path.c_str())){
    return 2;
  }

  if(!fs_mode->mkdir(path.c_str())){
    return 1;
  }

  return 0;
}




int SD_LIB::rf(std::string path){

  if(!fs_mode->exists(path.c_str())){
    return 2;

  }
  if(!fs_mode->remove(path.c_str())){
    return 1;
  }
  return 0;
}

