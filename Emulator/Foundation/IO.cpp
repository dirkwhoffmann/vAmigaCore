// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "IO.h"

namespace util {

string lowercased(const string& s)
{
    string result;
    for (auto c : s) { result += tolower(c); }
    return result;
}

string uppercased(const string& s)
{
    string result;
    for (auto c : s) { result += toupper(c); }
    return result;
}

string extractPath(const string &s)
{
    auto idx = s.rfind('/');
    auto pos = 0;
    auto len = idx != string::npos ? idx + 1 : 0;
    return s.substr(pos, len);
}

string extractName(const string &s)
{
    auto idx = s.rfind('/');
    auto pos = idx != string::npos ? idx + 1 : 0;
    auto len = string::npos;
    return s.substr(pos, len);
}

string extractSuffix(const string &s)
{
    auto idx = s.rfind('.');
    auto pos = idx != string::npos ? idx + 1 : 0;
    auto len = string::npos;
    return s.substr(pos, len);
}

string stripPath(const string &s)
{
    auto idx = s.rfind('/');
    auto pos = idx != string::npos ? idx + 1 : 0;
    auto len = string::npos;
    return s.substr(pos, len);
}

string stripName(const string &s)
{
    auto idx = s.rfind('/');
    auto pos = 0;
    auto len = idx != string::npos ? idx : 0;
    return s.substr(pos, len);
}

string stripSuffix(const string &s)
{
    auto idx = s.rfind('.');
    auto pos = 0;
    auto len = idx != string::npos ? idx : string::npos;
    return s.substr(pos, len);
}

bool isDirectory(const string &path)
{
    return isDirectory(path.c_str());
}

bool isDirectory(const char *path)
{
    struct stat fileProperties;
    
    if (path == nullptr)
        return -1;
        
    if (stat(path, &fileProperties) != 0)
        return -1;
    
    return S_ISDIR(fileProperties.st_mode);
}

isize numDirectoryItems(const string &path)
{
    return numDirectoryItems(path.c_str());
}

isize numDirectoryItems(const char *path)
{
    isize count = 0;
    
    if (DIR *dir = opendir(path)) {
        
        struct dirent *dp;
        while ((dp = readdir(dir))) {
            if (dp->d_name[0] != '.') count++;
        }
    }
    
    return count;
}

isize
getSizeOfFile(const string &path)
{
    return getSizeOfFile(path.c_str());
}

isize
getSizeOfFile(const char *path)
{
    struct stat fileProperties;
        
    if (stat(path, &fileProperties) != 0)
        return -1;
    
    return (isize)fileProperties.st_size;
}

bool matchingStreamHeader(std::istream &stream, const u8 *header, isize len)
{
    stream.seekg(0, std::ios::beg);
    
    for (isize i = 0; i < len; i++) {
        int c = stream.get();
        if (c != (int)header[i]) {
            stream.seekg(0, std::ios::beg);
            return false;
        }
    }
    stream.seekg(0, std::ios::beg);
    return true;
}

bool
matchingBufferHeader(const u8 *buffer, const u8 *header, isize len)
{
    assert(buffer != nullptr);
    assert(header != nullptr);
    
    for (isize i = 0; i < len; i++) {
        if (header[i] != buffer[i])
            return false;
    }

    return true;
}

bool
loadFile(const char *path, u8 **buffer, isize *size)
{
    assert(path != nullptr);
    assert(buffer != nullptr);
    assert(size != nullptr);

    *buffer = nullptr;
    *size = 0;
    
    // Get file size
    isize bytes = getSizeOfFile(path);
    if (bytes == -1) return false;
    
    // Open file
    FILE *file = fopen(path, "r");
    if (file == nullptr) return false;
     
    // Allocate memory
    u8 *data = new u8[bytes];
    if (data == nullptr) { fclose(file); return false; }
    
    // Read data
    for (isize i = 0; i < bytes; i++) {
        int c = fgetc(file);
        if (c == EOF) break;
        data[i] = (u8)c;
    }
    
    fclose(file);
    *buffer = data;
    *size = bytes;
    return true;
}

bool
loadFile(const char *path, const char *name, u8 **buffer, isize *size)
{
    assert(path != nullptr);
    assert(name != nullptr);

    char *fullpath = new char[strlen(path) + strlen(name) + 2];
    strcpy(fullpath, path);
    strcat(fullpath, "/");
    strcat(fullpath, name);
    
    return loadFile(fullpath, buffer, size);
}

isize
streamLength(std::istream &stream)
{
    auto cur = stream.tellg();
    stream.seekg(0, std::ios::beg);
    auto beg = stream.tellg();
    stream.seekg(0, std::ios::end);
    auto end = stream.tellg();
    stream.seekg(cur, std::ios::beg);
    
    return (isize)(end - beg);
}

}
