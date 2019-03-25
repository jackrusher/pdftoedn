//
// Copyright 2016-2019 Ed Porras
//
// This file is part of pdftoedn.
//
// pdftoedn is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdftoedn is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdftoedn.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <string>
#include "base_types.h"

namespace pdftoedn
{
    class FontEngine;

    namespace util
    {
        namespace version {
            extern const pdftoedn::Symbol SYMBOL_DATA_FORMAT_VERSION;

            // see util_data_format_version.cc for info on version
            // meaning
            uintmax_t data_format_version();

            std::string info();
            util::edn::Hash& libs(util::edn::Hash& h);

            // font engine initializes freetype so we need this kludgy crap
            std::string freetype(const pdftoedn::FontEngine&);
        }
    }
}
