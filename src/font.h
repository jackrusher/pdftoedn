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
#include <set>

#include "base_types.h"
#include "pdf_font_source.h"
#include "font_maps.h"

struct Ref;

namespace pdftoedn
{
    class PdfPath;

    // -------------------------------------------------------
    // document fonts
    //
    class PdfFont : public gemable {
    public:

        // constructors
        PdfFont(FontSource* const font_source, const FontData* const fnt_data);
        ~PdfFont();

        const std::string& name() const { return utf_font_name; }
        const std::string& family() const { return font_data->output_font(); }
        const FontSource* src() const { return font_src; }

        bool equals(const PdfFont& f) const { return (font_src->equals(*f.font_src)); }
        bool has_warnings() const { return (!font_src->has_to_unicode() || has_unmapped_codes()); }
        bool is_bold() const { return bold; }
        bool is_italic() const { return italic; }
        bool has_replacement_glyph_map() const { return font_data->has_user_map(); }
        bool is_remapped() const { return font_data->has_user_map(); }
        bool is_ignored() const { return font_data->is_ignored(); }
        bool is_embedded() const { return font_src->is_embedded(); }
        bool is_unknown() const { return font_data->is_unknown_font(); }
        bool is_type1() const { return font_src->is_type1(); }
        bool is_truetype() const { return font_src->is_truetype(); }
        bool is_cid() const { return font_src->is_cid(); }

        bool has_to_unicode() const { return font_src->has_to_unicode(); }

        enum eRemapStatus { REMAP_FAIL, REMAP_USER, REMAP_ENCODING, REMAP_UNICODE };

        eRemapStatus remap_glyph(uint32_t code, Unicode* const unicode, uintmax_t& remapped) const;
        bool has_unmapped_codes() const { return (!unmapped_codes.empty()); }
        std::string get_unmapped_codes_str() const;

        const PdfPath* get_glyph_path(uint32_t code) const;

        // for comparing font pointers using their names
        struct lt {
            bool operator()(const PdfFont* a, const PdfFont* b) const {
                return (a->name() < b->name());
            }
        };

        virtual std::ostream& to_edn(std::ostream& o) const;
        util::edn::Hash& to_edn_hash(util::edn::Hash& font_h) const;

        static const pdftoedn::Symbol SYMBOL_FAMILY;
        static const pdftoedn::Symbol SYMBOL_STYLE_BOLD;
        static const pdftoedn::Symbol SYMBOL_STYLE_ITALIC;
        static const pdftoedn::Symbol SYMBOL_STYLE_ALL_CAPS;
        static const pdftoedn::Symbol SYMBOL_STYLE_SMALL_CAPS;

        // debug
        virtual std::ostream& dump(std::ostream& o) const;

        const char* type_str() const;

        void clear_unmapped_codes() const { unmapped_codes.clear(); }

    private:
        std::string utf_font_name;
        FontSource* font_src;
        const FontData* font_data;
        bool bold;
        bool italic;
        mutable std::set<uint32_t> unmapped_codes;
        mutable std::map<int16_t, PdfPath*> glyph_path_cache;
    };

} // namespace
