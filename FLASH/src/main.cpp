#include "FS.h"
#include "LittleFS.h"

using namespace std;

void rf(fs::FS &fs, const string path){
  if(!fs.exists(path.c_str())){
    Serial.println("not exists");
    return;

  }
  if(!fs.remove(path.c_str())){
    Serial.println("fail to remove file or not a file");
    return;
  }
}

void rmdir(fs::FS &fs, const string path){
  if(!fs.exists(path.c_str())){
    Serial.println("not exists");
    return;

  }
  if(!fs.rmdir(path.c_str())){
    Serial.println("fail to remove dir or not a dir");
    return;
  }
}
void mkdir(fs::FS &fs, const string path){

  if(fs.exists(path.c_str())){
    Serial.println("already exists");
    return;
  }

  if(!fs.mkdir(path.c_str())){
    Serial.println("fail to create dir");
    return;
  }

  Serial.print(path.c_str());
  Serial.println(" dir created");

}

void ls(fs::FS &fs ,File root){
  root.rewindDirectory();
  File file = root.openNextFile();
  if (!file){
    Serial.println("no file / dir");
  }
  while(file){
    if (file.isDirectory()){
      Serial.print("DIR: ");
      Serial.println(file.name());
    }else{
      Serial.print("FILE: ");
      Serial.println(file.name());
    }
   
   file = root.openNextFile();

  }
  file.close();
}
void touch(fs::FS &fs, const string name , const string content){
  auto file = fs.open(name.c_str(), "w");
  file.println(content.c_str());
  file.close();

}

void echo(fs::FS &fs,const string name){
  auto file = fs.open(name.c_str(), "r");
  if(!file || file.isDirectory()){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("File Content:");
  Serial.println(file.readString());
  file.close();
}

void nano(fs::FS &fs,const string name, const string content){
  auto file = fs.open(name.c_str(), "a+");
  Serial.println("content :");
  file.seek(0, SeekSet); // Move to the beginning before reading
  Serial.println(file.readString());
  Serial.println("append content :");
  Serial.println(content.c_str());
  file.println(content.c_str());
  file.seek(0, SeekSet); // Move to the beginning before reading new content
  Serial.println("new content :");
  Serial.println(file.readString());
  file.close();
}


void setup() {
  Serial.begin(115200);
  delay(1000);
    if (!LittleFS.begin()) {
    Serial.println("Mounting LittleFS failed. Formatting...");
    
    // If mounting fails, format the partition
    if (LittleFS.format()) {
      Serial.println("File system formatted successfully. Please reset the device.");
      // After formatting, you should restart the ESP
      ESP.restart();
    } else {
      Serial.println("Failed to format file system.");
      return; // Stop execution if formatting fails
    }
  }
  
  File root = LittleFS.open("/");
// touch(LittleFS,"/hello.txt","Hello World");
// ls(LittleFS,root);
// echo(LittleFS,"/hello.txt");
//  nano(LittleFS,"/hello.txt","append ballsa");
  //mkdir(LittleFS,"/mydirs");
  //ls(LittleFS,root);
  //rmdir(LittleFS,"/mydir");
  //ls(LittleFS,root);
  //rf(LittleFS,"/hello.txt");
  //ls(LittleFS,root);
  

}
void loop() {
  // Do nothing. This empty loop prevents the format command
  // from running repeatedly and allows for easy re-uploading.
}