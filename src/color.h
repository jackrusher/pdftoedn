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

#include <ostream>
#include <poppler/GfxState.h>
#include "base_types.h"

namespace pdftoedn
{
    typedef uint32_t color_comp_t;

    // ----------------------------------
    //
    struct Color : gemable {
        virtual std::ostream& to_edn(std::ostream& o) const = 0;
    };

    // ----------------------------------
    // rubifies color commands
    //
    class RGBColor : public Color {
    public:
        RGBColor() : r(0), g(0), b(0) { }
        RGBColor(const GfxRGB& color) :
            r(color.r), g(color.g), b(color.b)
        { }
        RGBColor(color_comp_t red, color_comp_t green, color_comp_t blue) :
            r(red), g(green), b(blue)
        { }

        color_comp_t red() const { return colToByte(r); }
        color_comp_t green() const { return colToByte(g); }
        color_comp_t blue() const { return colToByte(b); }

        bool equals(color_comp_t red, color_comp_t green, color_comp_t blue) const {
            return (r == red && g == green && b == blue);
        }

        virtual std::ostream& to_edn(std::ostream& o) const;

    private:
        color_comp_t r, g, b;
    };

} // namespace
