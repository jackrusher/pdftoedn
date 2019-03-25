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
#include <ostream>

namespace pdftoedn
{
    namespace util
    {
        namespace encode {

            bool encode_image(std::ostream& output, ImageStream* img_str, const StreamProps& properties,
                              GfxImageColorMap *colorMap);
            bool encode_rgba_image(std::ostream& output, ImageStream* img_str, ImageStream* mask_str,
                                   const StreamProps& properties,
                                   GfxImageColorMap *color_map, GfxImageColorMap *mask_color_map,
                                   bool mask_invert);
            bool encode_mask(std::ostream& output, ImageStream* img_str, const StreamProps& properties);
#if 0
            bool encode_grey_image(std::ostream& output, ImageStream* img_str, const StreamProps& properties,
                                   GfxImageColorMap *colorMap);
#endif
        }
    }
}
