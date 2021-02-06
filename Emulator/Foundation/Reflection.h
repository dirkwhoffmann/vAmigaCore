// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"

#include <stdio.h>
#include <map>
#include <string>

#define assert_enum(e,v) assert(e##Enum::isValid(v))

struct EnumParseError : public std::exception
{
    std::string description;
    EnumParseError(const std::string &s) : description(s) { }
    const char *what() const throw() override { return  description.c_str(); }
};

template <class T, typename E> struct Reflection {

    // Returns the shortened key as a C string
    static const char *key(long nr) { return T::key((E)nr); }

    // Collects all key / value pairs
    static std::map <std::string,long> pairs(long min = 1) {
        
        std::map <std::string,long> result;
                
        for (isize i = 0;; i++) {
            if (T::isValid(i)) {
                result.insert(std::make_pair(key(i), i));
            } else {
                if (i >= min) break;
            }
        }
        
        return result;
    }

    // Returns a list in form of a colon seperated string
    static std::string keyList(bool prefix = false) {
        
        std::string result;
        
        auto p = pairs();
        for(auto it = std::begin(p); it != std::end(p); ++it) {
            if (it != std::begin(p)) result += ", ";
            if (prefix && T::prefix()) result += T::prefix();
            result += it->first;
        }
        
        return result;
    }
    
    // Parses a string
    static E parse(const std::string& key) {
          
        std::string upperKey;
        for (auto c : key) { upperKey += toupper(c); }
        
        auto p = pairs();
        
        auto it = p.find(upperKey);
        if (it == p.end()) throw EnumParseError(keyList());
        
        return (E)it->second;
    }
};
