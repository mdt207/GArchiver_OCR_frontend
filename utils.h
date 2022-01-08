#include <cstdio>
#include <stdint.h>
#include <string>
#include <filesystem>


enum ImageType
{
   PNG, JPG, BMP, TGA, PDF
};

bool  is_file_exist(std::string& str);
char* file_io(const char *file_name);

ImageType getFileType(const char *filename);
