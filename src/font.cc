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
#include <set>

#include "pdf_font_source.h"
#include "font.h"
#include "font_maps.h"
#include "util.h"
#include "util_edn.h"
#include "util_debug.h"

#if 0
#include "runtime_options.h"
#endif

namespace pdftoedn
{
    // static initializers
    const pdftoedn::Symbol PdfFont::SYMBOL_FAMILY              = "family";
    const pdftoedn::Symbol PdfFont::SYMBOL_STYLE_BOLD          = "bold";
    const pdftoedn::Symbol PdfFont::SYMBOL_STYLE_ITALIC        = "italic";
    const pdftoedn::Symbol PdfFont::SYMBOL_STYLE_ALL_CAPS      = "all_caps";
    const pdftoedn::Symbol PdfFont::SYMBOL_STYLE_SMALL_CAPS    = "small_caps";

    static const pdftoedn::Symbol SYMBOL_FAMILY                = "family";
    static const pdftoedn::Symbol SYMBOL_GENERAL_FAMILY        = "general_family";
    static const pdftoedn::Symbol SYMBOL_ORIGINAL_NAME         = "original_name";
    static const pdftoedn::Symbol SYMBOL_COLLECTION            = "collection";
    static const pdftoedn::Symbol SYMBOL_TYPE                  = "type";
    static const pdftoedn::Symbol SYMBOL_STYLE_BOLD            = "bold";
    static const pdftoedn::Symbol SYMBOL_STYLE_ITALIC          = "italic";
    static const pdftoedn::Symbol SYMBOL_STYLE_ALL_CAPS        = "all_caps";
    static const pdftoedn::Symbol SYMBOL_STYLE_SMALL_CAPS      = "small_caps";

    static const pdftoedn::Symbol SYMBOL_EMBEDDED              = "embedded";
    static const pdftoedn::Symbol SYMBOL_EXPORTED              = "exported";
    static const pdftoedn::Symbol SYMBOL_DRAW_AS_PATHS         = "output_as_path";

    static const pdftoedn::Symbol SYMBOL_WARNINGS              = "warnings";
    static const pdftoedn::Symbol SYMBOL_NO_UNICODE_MAP        = "no_unicode_map";
    static const pdftoedn::Symbol SYMBOL_HAS_UNMAPPED_GLYPHS   = "has_unmapped_glyphs";

    static const pdftoedn::Symbol SYMBOL_MAPPERS               = "mappers";
    static const pdftoedn::Symbol SYMBOL_ENCODING              = "encoding";
    static const pdftoedn::Symbol SYMBOL_CODETOGID             = "code_to_gid";

    static const pdftoedn::Symbol SYMBOL_C2G_MD5               = "c2g_md5";

    // =============================================
    // PDF font class
    //

    PdfFont::PdfFont(FontSource* const font_source, const FontData* const fnt_data) :
        utf_font_name(util::string_to_utf(font_source->font_name())),
        font_src(font_source), font_data(fnt_data),
        bold(font_data->is_bold()), italic(font_data->is_italic())
    {
        if (!font_data->ignore_fd()) {
            // set style based on data from font descriptors in case
            // it wasn't set from the font data
            bold |= font_src->is_force_bold();
            italic |= font_src->is_italic();
        }
    }

    //
    // destructor
    PdfFont::~PdfFont()
    {
        util::delete_ptr_map_elems(glyph_path_cache);
        delete font_data;
        delete font_src;
    }


    //
    // lookup glyph path for the code in the cache
    const PdfPath* PdfFont::get_glyph_path(uint32_t code) const
    {
        std::map<int16_t, PdfPath*>::const_iterator pi = glyph_path_cache.find(code);
        if (pi != glyph_path_cache.end()) {
            return pi->second;
        }

        // not cached.. build it and cache it
        PdfPath *p = new PdfPath;
        if (!font_src->get_glyph_path(code, *p)) {
            delete p;
            return nullptr;
        }

        glyph_path_cache.insert(std::pair<uint16_t, PdfPath*>(code, p));
        return p;
    }

    //
    // use our lookup tables to try and identify the given glyph. if
    // this fails, we fall back to path drawing
    PdfFont::eRemapStatus PdfFont::remap_glyph(uint32_t code, Unicode* const unicode, uintmax_t& remapped) const
    {
        static const uint16_t UNICODE_REPLACEMENT_CHAR = 65533;

        if (is_remapped()) {
            // we have a custom map for this font.. try remapping the code (most common case)
            if (font_data->remap_glyph(font_src, code, remapped)) {
                return REMAP_USER;
            }

            // some embedded truetype & CID fonts are mapped using the
            // unicode value if one exists and is valid
            if (is_truetype() || is_cid()) {
                if (unicode && *unicode != UNICODE_REPLACEMENT_CHAR) {
                    if (font_data->remap_glyph(font_src, *unicode, remapped)) {
                        return REMAP_USER;
                    }
                }
            }

#if 0
            if (pdftoedn::options.include_debug_info() &&
                (unmapped_codes.find(code) == unmapped_codes.end())) {
                std::stringstream msg;
                msg << "Unmapped glyph in font " << font_src->font_name()
                    << "\t(" << type_str() << "),";
                const Encoding* e = font_src->get_encoding();
                if (e)
                    msg << " enc: " << Encoding::get_encoding_str(e->type())
                        << ", ent: " << e->entity(code) << ",";
                msg << "\tcode: " << std::dec << code << " (0x" << std::hex << code
                    << ")";
                if (unicode)
                    msg << ", toUnicode: " << std::dec << *unicode
                        << " (" << (char) *unicode << ")";
                std::cerr << msg.str() << std::endl;
                unmapped_codes.insert(code);
            }
#endif
        }

        // if the font is marked as having a known encoding, it's
        // *probably* ok but poppler remaps certain values
        // incorrectly so we look up the entity ourselves
        if (font_src->has_std_encoding()) {

            const Encoding* enc = font_src->get_encoding();

            if (enc->is_alpha_type()) {
                // minor optimizations for WinAnsi and MacOSRoman
                // since the code == unicode when < 129
                if (code < 0x20) {
                    remapped = 160;
                    return REMAP_ENCODING;
                }
                if (code < 0x80) {
                    remapped = code;
                    return REMAP_ENCODING;
                }
            }

            if (doc_font_maps.search_std_map(enc->entity(code), remapped)) {
                return REMAP_ENCODING;
            }
        }

        //
        // no mapping but there's a toUnicode table
        if (unicode) {
            // if this is not an embedded font, we might be able to trust
            // the unicode value
            if (!is_embedded()) {
                if (is_type1() && code < 0x21) {
                    remapped = L' ';
                    return REMAP_UNICODE;
                } else if (unicode) {
                    remapped = *unicode;
                    return REMAP_UNICODE;
                }
            }

            // drawing straws here - check the glyph. If it does not
            // consist of subpaths, then it is whitespace. Substitute
            // it.
            const PdfPath* path = get_glyph_path(code);
            if (!path || path->length() == 0) {
                remapped = L' ';
                return REMAP_UNICODE;
            }

            // this is an embedded font with no known encoding. The
            // toUnicode value could be wrong so track it to report
            // errors
            unmapped_codes.insert(code);

            // last resort: if there's a unicode map, hope the
            // mapping provided is right
            if (*unicode != 0) {
                // return the unicode character as is unless it's non-printable
                remapped = ((*unicode < 0x21) ? L' ' : *unicode);
                return REMAP_UNICODE;
            }

            // error
            remapped = UNICODE_REPLACEMENT_CHAR;

            std::stringstream err;
            err << "Unhandled non-unicode character code (" << std::hex << code;

            if (code > 0x21) {
                err << ", '" << (char) code << "'";
            }
            err << ") in font " << utf_font_name;
            et.log_error( ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, err.str() );
        }
        else {
            // no toUnicode!
            unmapped_codes.insert(code);
        }

        return REMAP_FAIL;
    }

    //
    // Pdf Font output - only included in :meta if an option is passed
    std::ostream& PdfFont::to_edn(std::ostream& o) const
    {
        util::edn::Hash font_h;
        o << to_edn_hash(font_h);
        return o;
    }


    util::edn::Hash& PdfFont::to_edn_hash(util::edn::Hash& font_h) const
    {
        font_h.reserve(13);

        // font name
        font_h.push( SYMBOL_ORIGINAL_NAME,                 utf_font_name );
        font_h.push( PdfFont::SYMBOL_FAMILY,               font_data->output_font() );

        // family type
        static const pdftoedn::Symbol SYMBOL_FAMILY_TYPES[] = {
            "monospace", "serif", "sans_serif"
        };
        font_h.push( SYMBOL_GENERAL_FAMILY,                SYMBOL_FAMILY_TYPES[ font_src->font_general_family_type() ] );

        // font collection, if applicable
        if (font_src->is_cid()) {
            font_h.push( SYMBOL_COLLECTION,                font_src->font_collection() );
        }

        // font type
        static const pdftoedn::Symbol SYMBOL_FONT_TYPES[]        = {
            "unknown", "type1", "type1c", "type1cot", "type3", "truetype",
            "truetype0t", "cidtype0", "cidtype0c", "cidtype0cot", "cidtype2", "cidtype2ot"
        };

        // get index into SYMBOL_FONT_TYPES[]
        uint8_t font_type_idx = 0;
        switch (font_src->font_type()) {
          case FontSource::FONT_TYPE_TYPE1:       font_type_idx = 1; break;
          case FontSource::FONT_TYPE_TYPE1C:      font_type_idx = 2; break;
          case FontSource::FONT_TYPE_TYPE1COT:    font_type_idx = 3; break;
          case FontSource::FONT_TYPE_TYPE3:       font_type_idx = 4; break;
          case FontSource::FONT_TYPE_TRUETYPE:    font_type_idx = 5; break;
          case FontSource::FONT_TYPE_TRUETYPEOT:  font_type_idx = 6; break;
          case FontSource::FONT_TYPE_CIDTYPE0:    font_type_idx = 7; break;
          case FontSource::FONT_TYPE_CIDTYPE0C:   font_type_idx = 8; break;
          case FontSource::FONT_TYPE_CIDTYPE0COT: font_type_idx = 9; break;
          case FontSource::FONT_TYPE_CIDTYPE2:    font_type_idx = 10; break;
          case FontSource::FONT_TYPE_CIDTYPE2OT:  font_type_idx = 11; break;
          case FontSource::FONT_TYPE_UNKNOWN:
          default: break;
        }
        font_h.push( SYMBOL_TYPE,                          SYMBOL_FONT_TYPES[ font_type_idx ] );

        // font style attributes, if present
        if (is_bold()) {
            font_h.push( PdfFont::SYMBOL_STYLE_BOLD,       true );
        }

        if (is_italic()) {
            font_h.push( PdfFont::SYMBOL_STYLE_ITALIC,     true );
        }

        if (font_src->is_all_cap()) {
            font_h.push( PdfFont::SYMBOL_STYLE_ALL_CAPS,   true );
        }

        if (font_src->is_small_cap()) {
            font_h.push( PdfFont::SYMBOL_STYLE_SMALL_CAPS, true );
        }

        const EntityMapPtrList& mappers = font_data->mapper_list();
        if (!mappers.empty()) {
            util::edn::Vector mapper_a(mappers.size());
            for (const EntityMap* m : mappers) {
                mapper_a.push( m->name() );
            }
            font_h.push( SYMBOL_MAPPERS,                   mapper_a );
        }

        // encoding, if present
        const Encoding* enc = font_src->get_encoding();
        if (enc) {
            font_h.push( SYMBOL_ENCODING,                  Encoding::SYMBOL_TYPE[ enc->type() ] );
        }

        // if we've tagged this as a font we must export
        if (font_data->is_unknown_font()) {
            font_h.push( SYMBOL_EXPORTED,                  true );
        }
        else {
            // otherwise, report any warnings we encountered

            // if it's embedded
            if (is_embedded()) {
                font_h.push( SYMBOL_EMBEDDED,              true );

                // include the C2G table's md5 if there's one
                if (font_src->has_code_to_gid()) {
                    font_h.push( SYMBOL_C2G_MD5,           font_src->md5() );
                }
            }

            util::edn::Vector warn_a(2);

            // if we've remapped glyphs on this font
            if (has_replacement_glyph_map()) {
                // report any we've missed
                if (has_unmapped_codes()) {
                    warn_a.push( SYMBOL_HAS_UNMAPPED_GLYPHS );

                    std::stringstream err;
                    err << "Font '" << utf_font_name << "' has unmapped codes";
                    pdftoedn::et.log_warn(ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, err.str());
                }
            } else if (!font_src->has_to_unicode()) {
                warn_a.push( SYMBOL_NO_UNICODE_MAP );

                std::stringstream err;
                err << "No subst map for '" << utf_font_name << "'";
                pdftoedn::et.log_warn(ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, err.str());
            }

            // add warnings array if any found
            if (!warn_a.empty()) {
                font_h.push( SYMBOL_WARNINGS,              warn_a );
            }
        }

        return font_h;
    }


    //
    // debug
    std::string PdfFont::get_unmapped_codes_str() const
    {
        std::stringstream codes_stream;
        if (!unmapped_codes.empty()) {
            codes_stream << "[ ";
            const Encoding *e = nullptr;

            if (is_type1()) {
                e = font_src->get_encoding();
            }

            codes_stream << std::hex;

            for (uint32_t c : unmapped_codes) {
                codes_stream << "0x" << c;
                if (e) {
                    codes_stream << " (" << e->entity(c) << ")";
                }
                codes_stream << ' ';
            }
            codes_stream << "]";
        }
        return codes_stream.str();
    }

    std::ostream& PdfFont::dump(std::ostream& o) const
    {
        o << "ref: " << font_src->font_ref() << " "
          << font_data->output_font() << " "
          << (font_data->is_bold() ? "Bold " : "")
          << (font_data->is_italic() ? "Italic " : "")
          << ", type: " << type_str()
          << " - (original: " << utf_font_name << ")";
        return o;
    }

    const char* PdfFont::type_str() const {
        return util::debug::get_font_type_str(font_src->font_type());
    }

} // namespace
