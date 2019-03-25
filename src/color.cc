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

#include <ostream>
#include <iomanip>
#include <sstream>

#include "color.h"

namespace pdftoedn
{
    // =============================================
    // RGB colors

    //
    // output color components into an HTML-style color string (e.g.: #00ff00)
    std::ostream& RGBColor::to_edn(std::ostream& o) const
    {
        o << '"'
          << "#" << std::setfill('0')
          << std::setw(2) << std::hex << red()
          << std::setw(2) << std::hex << green()
          << std::setw(2) << std::hex << blue()
          << '"';
        return o;
    }
} // namespace
