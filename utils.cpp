#include "utils.h"
#include <cstring>
#include <sstream>
#include <fstream>
//#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

//char* file_io(const char *file_name)
std::string file_io(const char *file_name)
{
  std::ostringstream vts;

  std::string str;
  std::ifstream ifs (file_name, std::ifstream::binary);

  // get pointer to associated buffer object
  std::filebuf* pbuf = ifs.rdbuf();

  // get file size using buffer's members
  //std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);
  //pbuf->pubseekpos (0,ifs.in);
  
  // get file size
  ifs.seekg(0, std::ios::end);
  std::size_t size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  
  
  // allocate memory to contain file data
  //char* buffer = new char[size];
  std::vector<unsigned char> buffer (std::istreambuf_iterator<char>(ifs), {});
  
  //memset(buffer, 0, size);
  //ifs.read (buffer, size);
  
  // get file data
  //pbuf->sgetn (buffer, size);
  
  std::copy (buffer.begin(), buffer.end()-1, std::ostream_iterator<char>(vts));
  vts << buffer.back();
  
  str.assign (vts.str());

  ifs.close();

  // write content to stdout
  //std::cout << "1: "<< str.length() << std::endl;
  //std::cout << str << std::endl;
  
  //std::cout.write (buffer,size);

  //delete[] buffer; //caller must do it
  return str;//buffer;
}


bool is_file_exist(std::string& str)
{
    namespace fs = std::filesystem;
    fs::path p(str);
    //std::cout << str << std::endl;
    return fs::exists(p);
}

ImageType getFileType (const char* filename)
{
   const char *ext = strrchr(filename, '.'); 

   if(ext != nullptr) 
   { 
       if(strcmp(ext, ".png") == 0) { return PNG; }

      else if(strcmp(ext, ".jpg") == 0) { return JPG; }

      else if(strcmp(ext, ".bmp") == 0) { return BMP; } 
      else if(strcmp(ext, ".tga") == 0) { return TGA; }
      else if(strcmp(ext, ".pdf") == 0) { return PDF; }
  }
  
 return PNG; 
}
