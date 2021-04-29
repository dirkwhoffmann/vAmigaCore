// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Script.h"
#include "Amiga.h"
#include "IO.h"

#include <sstream>

Script::Script()
{
}

bool
Script::isCompatiblePath(const string &path)
{
    string suffix = util::extractSuffix(path);
    return suffix == "ini" || suffix == "INI";
}

bool
Script::isCompatibleStream(std::istream &stream)
{
    return true;
}

void
Script::execute(class Amiga &amiga)
{
    string s((char *)data, size);
    try { amiga.retroShell.execScript(s); } catch (util::Exception &e) { }
}
