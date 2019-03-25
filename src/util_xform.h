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
#include <sstream>

namespace pdftoedn
{
    class PdfTM;

    namespace util
    {
        //
        // defined in util_xform.cc
        namespace xform {
            enum {
                XFORM_NONE     = 0x00,
                XFORM_FLIP_H   = 0x01,
                XFORM_FLIP_V   = 0x02,
                XFORM_FLIP     = (XFORM_FLIP_H | XFORM_FLIP_V),
                XFORM_SCALE_H  = 0x04,
                XFORM_SCALE_V  = 0x08,
                XFORM_SCALE    = (XFORM_SCALE_H | XFORM_SCALE_V),
                XFORM_ROT_ORTH = 0x10,
                XFORM_ROT_ARB  = 0x20,
                XFORM_ROT      = (XFORM_ROT_ORTH | XFORM_ROT_ARB),

                XFORM_ERR      = 0xff
            };

            bool init_transform_lib();
            uint8_t transform_image(const PdfTM& ctm, std::string& blob,
                                    int& width, int& height, bool inverted_mask);
        }
    }
}
