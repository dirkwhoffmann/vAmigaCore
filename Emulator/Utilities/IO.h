// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include <dirent.h>
#include <fcntl.h>
#include <istream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace util {

//
// Handling file names
//

// Changes the capitalization of a string
string lowercased(const string& s);
string uppercased(const string& s);

// Extracts a certain component from a path
string extractPath(const string &path);
string extractName(const string &path);
string extractSuffix(const string &path);

// Strips a certain component from a path
string stripPath(const string &path);
string stripName(const string &path);
string stripSuffix(const string &path);

// Concatenates two path segments
string appendPath(const string &path, const string &path2);

// Checks if a string starts with '/'
bool isAbsolutePath(const string &path);


//
// Handling files
//

// Returns the size of a file in bytes
isize getSizeOfFile(const string &path);

// Checks if a file exists
bool fileExists(const string &path);

// Checks if a path points to a directory
bool isDirectory(const string &path);

// Returns the number of files in a directory
isize numDirectoryItems(const string &path);

// Returns a list of files in a directory
std::vector<string> files(const string &path, const string &suffix = "");
std::vector<string> files(const string &path, std::vector <string> &suffixes);

// Checks the header signature (magic bytes) of a stream or buffer
bool matchingStreamHeader(std::istream &stream, const u8 *header, isize len);
bool matchingBufferHeader(const u8 *buffer, const u8 *header, isize len);

// Loads a file from disk
bool loadFile(const string &path, u8 **bufptr, isize *size);
bool loadFile(const string &path, const string &name, u8 **bufptr, isize *size);


//
// Handling streams
//

isize streamLength(std::istream &stream);

struct dec {
    
    i64 value;
    
    dec(i64 v) : value(v) { };
    std::ostream &operator()(std::ostream &os) const;
};

struct hex {
    
    int digits;
    u64 value;
    
    hex(int d, u64 v) : digits(d), value(v) { };
    hex(u64 v) : hex(16, v) { };
    hex(u32 v) : hex(8, v) { };
    hex(u16 v) : hex(4, v) { };
    hex(u8 v) : hex(2, v) { };
    std::ostream &operator()(std::ostream &os) const;
};

struct tab {
    
    int pads;
    const string &str;
    
    tab(int p, const string &s) : pads(p), str(s) { };
    tab(const string &s) : tab(24, s) { };
    std::ostream &operator()(std::ostream &os) const;
};

struct bol {
    
    static const string& yes;
    static const string& no;
    
    bool value;
    const string &str1, &str2;
    
    bol(bool v, const string &s1, const string &s2) : value(v), str1(s1), str2(s2) { };
    bol(bool v) : bol(v, yes, no) { };
    std::ostream &operator()(std::ostream &os) const;
};

inline std::ostream &operator <<(std::ostream &os, dec v) { return v(os); }
inline std::ostream &operator <<(std::ostream &os, hex v) { return v(os); }
inline std::ostream &operator <<(std::ostream &os, tab v) { return v(os); }
inline std::ostream &operator <<(std::ostream &os, bol v) { return v(os); }

}
