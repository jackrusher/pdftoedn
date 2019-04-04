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

#ifdef FE_PREPROCESS_TEXT

#include <poppler/GfxFont.h>

#include "font_engine.h"
#include "font_engine_dev.h"
#include "pdf_output_dev.h"
#include "font.h"
#include "graphics.h"
#include "util_debug.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // FontEngine Device - for preprocessing doc and extrating font data
    //
    void FontEngDev::updateFont(GfxState *state)
    {
        GfxFont *gfx_font = state->getFont();
        if (!gfx_font) {
            return;
        }

        // sanity-check the font size - skip anything larger than 10 inches
        // (this avoids problems allocating memory for the font cache)
        if (EngOutputDev::state_font_size_not_sane(state)) {
            return;
        }

        //        std::cerr << __FUNCTION__ << " - loading font " << gfx_font->getName()->getCString() << ", type: " << util::debug::get_font_type_str(gfx_font->getType()) << std::endl;

        // load / update the current font
        cur_font = font_engine.load_font(gfx_font);

        if (cur_font) {
            // track the usage of this font size
            font_engine.track_font_size(EngOutputDev::get_transformed_font_size(state));
        }
    }


    void FontEngDev::drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen)
    {
        if (!cur_font) {
            return;
        }

        // if this is not an embedded font, don't bother with any look up tables, etc.
        if (!cur_font->is_embedded() && !cur_font->is_cid()) {
            return;
        }

        // if there's no toUnicode entry
        if (!cur_font->has_to_unicode()) {

            if (!cur_font->is_code_used(code)) {
                // TODO: render glyph & compare
                PdfPath path;
                if (cur_font->get_glyph_path(code, path)) {
                    // if not whitespace
                    if (path.length() > 0) {
                        // mark the code as used for this font
                        cur_font->mark_code_used(code);
                    }
                }
            }
        }
    }
} // namespace

#endif // FE_PREPROCESS_TEXT
