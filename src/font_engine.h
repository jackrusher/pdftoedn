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

#include <string>
#include <set>
#include <map>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <poppler/poppler-config.h>
#include <poppler/OutputDev.h>
#include <poppler/GfxFont.h>

#include "pdf_font_source.h"
#include "font_maps.h"

//#define FE_PREPROCESS_TEXT // collect text usage while pre-processing doc

class GfxState;

namespace pdftoedn
{
    class PdfFont;
    class PdfPath;

    typedef std::map<PdfRef, pdftoedn::PdfFont *> FontList;
    typedef std::pair<const pdftoedn::PdfRef, pdftoedn::PdfFont *> FontListEntry;

    // -------------------------------------------------------
    // Our FE - handles font creation (via FT) and caching
    //
    class FontEngine
    {
    public:
        // constructor / destructor
        FontEngine(XRef *doc_xref);
        virtual ~FontEngine();

        bool found_font_warnings() const { return has_font_warnings; }

        // poppler > 0.24.0 passes a doc Xref on every call to startPage
        void update_document_ref(XRef* doc_xref) { xref = doc_xref; }

        // load system / embedded document font using FT
        pdftoedn::PdfFont* load_font(const GfxFont* gfx_font);

        //
        // some ops can be helped if we provide a sorted list of font
        // sizes used in a document. Since we now pre-process all text, we
        // can easily take care of this and return it as part of the
        // document meta
        void track_font_size(double font_size) { font_sizes.insert(font_size); }
        // retrieve a sorted list of the tracked font sizes
        const std::set<double>& get_font_size_list() const { return font_sizes; }
        const FontList& get_font_list() const { return fonts; }

        // remap a character code
        enum eCodeRemapStatus {
            CODE_REMAP_ERROR,
            CODE_REMAP_IGNORE,
            CODE_REMAPPED_BY_POPPLER,
            CODE_REMAPPED_BY_EDSEL
        };
        eCodeRemapStatus get_code_unicode(CharCode code, Unicode* const u, uintmax_t& unicode);

    private:
        XRef *xref; // PDF document ref for object lookup
        bool has_font_warnings;
        FontList fonts;
        std::set<double> font_sizes;
        FT_Library ft_lib;
        pdftoedn::PdfFont* cur_doc_font;

        pdftoedn::PdfFont* find_font(const GfxFont* gfx_font) const;
        static std::string sanitize_font_name(const std::string& name);
    };

} // namespace

