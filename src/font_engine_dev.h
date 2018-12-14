//
// Copyright 2016-2018 Ed Porras
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

#ifdef FE_PREPROCESS_TEXT

#include "font.h"

namespace pdftoedn
{
    class FontEngine;

    //
    // font engine device - used to pre-process the document and
    // extract the font data. This is needed because document
    // processing is done page by page so we won't know what fonts
    // used until all pages are done
    class FontEngDev : public ::OutputDev
    {
    public:
        FontEngDev(pdftoedn::FontEngine& fnt_engine) :
            font_engine(fnt_engine), cur_font(nullptr)
        { }
        virtual ~FontEngDev() { }

        // need to define these
        virtual bool useDrawChar() { return true; }
        virtual bool interpretType3Chars() { return true; }

        virtual void startPage(int pageNum, GfxState *state, XRef *xref) { }

        // but we only care about these here
        virtual void updateFont(GfxState *state);
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen);

    private:
        pdftoedn::FontEngine& font_engine;
        pdftoedn::PdfFont* cur_font;
    };

} // namespace

#endif // FE_PREPROCESS_TEXT
