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
#include <ostream>
#include <vector>

#include <freetype2/ft2build.h>
#include FT_TYPES_H
#include FT_FREETYPE_H

#include <poppler/Object.h>
#include <poppler/GfxFont.h>

#include "base_types.h"

namespace pdftoedn
{
    class PdfPath;

    // -------------------------------------------------------
    // objects in a PDF file are identified by a Ref type containing
    // and object number and generation number. The Ref struct is
    // defined in poppler/Object.h
    struct PdfRef : public Ref
    {
        PdfRef(int n, int g) { num = n; gen = g; }
        explicit PdfRef(const Ref* const r) { num = r->num; gen = r->gen; }
        bool operator==(const Ref& r) const { return (r.num == num && r.gen == gen); }
        bool operator<(const PdfRef& r) const;

        friend std::ostream& operator<<(std::ostream& o, const PdfRef& r) {
            o << "(" << r.num << ", " << r.gen << ")";
            return o;
        }
    };


    // =============================================================================
    // fonts carry a code to glyph map with either 256 or ~65k entries
    // (depending on the type)
    //
    class CodeToGIDMap {
    public:
        CodeToGIDMap(uintmax_t len);
        CodeToGIDMap(uintmax_t len, int* code_to_GID_map);
        ~CodeToGIDMap() { delete [] c2g_map; }

        uintmax_t length() const { return size; }
        bool has_code_to_gid_map() const { return (size > 0); }
        void set_index(uint32_t code, uintmax_t gid) { c2g_map[code] = gid; }
        const std::string& md5() const { return c2g_md5; }

        // once all c2g mappings have been set, this needs to be called to
        // compute md5 of map
        void finalize();

        uintmax_t map(uint32_t code) const;

        std::ostream& dump(std::ostream& o) const;
        friend std::ostream& operator<<(std::ostream& o, const CodeToGIDMap& m) {
            return m.dump(o);
        }

    private:
        uintmax_t size;
        int* c2g_map;
        std::string c2g_md5; // md5 of the c2g_map
    };


    // =============================================================================
    // some fonts carry an encoding table
    //
    class Encoding {
    public:
        enum Type {
            ENC_MACOS_ROMAN,
            ENC_MACOS_EXPERT,
            ENC_WINANSI,
            ENC_EXPERT,
            ENC_SYMBOL,
            ENC_ZAPF_DINGBATS,
            ENC_CUSTOM,

            ENC_INVALID
        };

        Encoding(const Gfx8BitFont* font);

        bool is_ok() const { return (e_type != ENC_INVALID); }

        const std::string& name() const { return e_name; }
        Type type() const { return e_type; }

        bool is_standard() const { return (e_type < ENC_CUSTOM); }
        bool is_alpha_type() const { return (e_type == ENC_MACOS_ROMAN || e_type == ENC_WINANSI); }
        bool has_map() const { return (!e_map.empty()); }
        bool has_entity(uint32_t code) const {
            return ((code < e_map.size()) && (e_map[code].length() > 0));
        }
        const char* entity(uint32_t code) const;

        static const char* get_encoding_str(Type e);
        std::ostream& dump(std::ostream& o) const;
        friend std::ostream& operator<<(std::ostream& o, const Encoding& e) {
            return e.dump(o);
        }

        static const pdftoedn::Symbol SYMBOL_TYPE[];

    private:
        std::string e_name;
        Type e_type;
        std::vector<std::string> e_map;

        Type determine_type(const Gfx8BitFont*) const;
    };


    // =============================================================================
    // abstract class for font sources
    //
    class FontSource {
    public:

        enum GeneralFamilyType {
            FAM_MONOSPACE,
            FAM_SERIF,
            FAM_SANS_SERIF
        };

        enum FontType {
            FONT_TYPE_UNKNOWN       = 0x00000000,
            FONT_TYPE_TYPE1         = 0x00000001,
            FONT_TYPE_TYPE1C        = 0x00000002,
            FONT_TYPE_TYPE1COT      = 0x00000004,
            FONT_TYPE_TYPE3         = 0x00000010,
            FONT_TYPE_TRUETYPE      = 0x00000100,
            FONT_TYPE_TRUETYPEOT    = 0x00000200,
            FONT_TYPE_CIDTYPE0      = 0x00001000,
            FONT_TYPE_CIDTYPE0C     = 0x00002000,
            FONT_TYPE_CIDTYPE0COT   = 0x00004000,
            FONT_TYPE_CIDTYPE2      = 0x00010000,
            FONT_TYPE_CIDTYPE2OT    = 0x00020000,

            FONT_TYPE_TYPE1_MASK    = 0x0000000f,
            FONT_TYPE_TYPE3_MASK    = 0x000000f0,
            FONT_TYPE_TRUETYPE_MASK = 0x00000f00,
            FONT_TYPE_CID0_MASK     = 0x0000f000,
            FONT_TYPE_CID2_MASK     = 0x000f0000,
            FONT_TYPE_CID_MASK      = (FONT_TYPE_CID0_MASK | FONT_TYPE_CID2_MASK),
        };

        enum FontFlags { // see TABLE 5.19 Font flags in PDF spec
            FONT_FLAG_FIXED_PITCH = 0x00000001, // bit 1
            FONT_FLAG_SERIF       = 0x00000002, // bit 2
            FONT_FLAG_SYMBOLIC    = 0x00000004, // bit 3
            FONT_FLAG_SCRIPT      = 0x00000008, // bit 4
            // 0x00000010 - not used
            FONT_FLAG_NONSYMBOLIC = 0x00000020, // bit 6
            FONT_FLAG_ITALIC      = 0x00000040, // bit 7
            // 0x00000080-0x000080000 not used
            FONT_FLAG_ALL_CAP     = 0x00010000, // bit 17
            FONT_FLAG_SMALL_CAP   = 0x00020000, // bit 18
            FONT_FLAG_FORCE_BOLD  = 0x00040000, // bit 19
        };

        enum FontLocation {
            LOC_EXTERNAL,
            LOC_EMBEDDED
        };

        // constructor / destructor
        FontSource(const GfxFont* gfx_font, FontType font_type, const std::string& font_name,
                   FT_Library lib, const uint8_t* buffer, uintmax_t len,
                   uintmax_t font_face_index = 0);
        FontSource(const GfxFont* gfx_font, FontType font_type, const std::string& font_name,
                   const std::string& file);
        ~FontSource();

        // getters
        bool equals(const FontSource& fs) const { return (ref == fs.ref); }
        bool is_ok() const { return font_ok; }
        bool is_type1() const { return (type & FONT_TYPE_TYPE1_MASK); }
        bool is_truetype() const { return (type & FONT_TYPE_TRUETYPE_MASK); }
        bool is_cid0() const { return (type & FONT_TYPE_CID0_MASK); }
        bool is_cid2() const { return (type & FONT_TYPE_CID2_MASK); }
        bool is_cid() const { return (type & FONT_TYPE_CID_MASK); }
        bool is_embedded() const { return (location == LOC_EMBEDDED); }

        bool has_encoding() const { return (encoding != nullptr); }
        bool has_std_encoding() const { return (encoding && encoding->is_standard()); }
        bool has_to_unicode() const { return to_unicode; }
        const std::string& md5() const { return (code_to_gid ? code_to_gid->md5() : blob_md5); }

        const PdfRef& font_ref() const { return ref; }
        const std::string& font_name() const { return name; }
        FontType font_type() const { return type; }
        GeneralFamilyType font_general_family_type() const { return family_type; }
        const std::string& font_collection() const { return collection; }

        bool is_fixed_pitch() const { return (gfx_font_flags & FONT_FLAG_FIXED_PITCH); }
        bool is_symbolic() const { return (gfx_font_flags & FONT_FLAG_SYMBOLIC); }
        bool is_nonsymbolic() const { return (gfx_font_flags & FONT_FLAG_NONSYMBOLIC); }
        bool is_italic() const { return (gfx_font_flags & FONT_FLAG_ITALIC); }
        bool is_all_cap() const { return (gfx_font_flags & FONT_FLAG_ALL_CAP); }
        bool is_small_cap() const { return (gfx_font_flags & FONT_FLAG_SMALL_CAP); }
        bool is_force_bold() const { return (gfx_font_flags & FONT_FLAG_FORCE_BOLD); }

        bool has_code_to_gid() const { return ((code_to_gid != nullptr) && code_to_gid->has_code_to_gid_map()); }
        const CodeToGIDMap* get_code_to_gid() const { return code_to_gid; }

        const Encoding* get_encoding() const { return encoding; }
        bool get_glyph_path(CharCode code, PdfPath& path, const PdfTM* tm = nullptr) const;

    private:
        PdfRef ref;
        FontType type;
        std::string name;
        FontLocation location;
        GeneralFamilyType family_type;
        uint32_t gfx_font_flags; // collected from PDF GfxFont->getFlags()
        std::string collection;
        Encoding* encoding;
        CodeToGIDMap* code_to_gid;

        bool to_unicode;
        FT_Library ft_lib;
        FT_Face ft_face;
        intmax_t face_index;

        std::string font_blob;
        std::string blob_md5;
        std::string filename;
        bool font_ok;

        void check_name();
        bool load_font(const GfxFont* gfx_font);

        bool load_face();
        bool load_type1_font(const Gfx8BitFont* gfx8_font);
        bool load_truetype_font(const Gfx8BitFont* gfx8_font);
        bool load_cid_font(const GfxCIDFont * cid_font);
        bool load_cidtype2_font(const GfxCIDFont * cid_font);
    };

} // namespace
