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
#include <map>
#include <list>

namespace pdftoedn
{

    class FontSource;
    class DocFontMaps;

    // ================================================================
    // various types for storing maps of entity codes to unicode pairs
    //
    typedef std::map<const std::string, uintmax_t> EntityPairMap;

    class EntityMap
    {
    public:
        static const std::string STANDARD_MAP_NAME;

        EntityMap(const std::string& map_name) :
            em_name(map_name), entity_based(false)
        { }

        const std::string& name() const { return em_name; }
        bool is_entity_based() const { return entity_based; }
        bool find(const std::string& entity, uintmax_t& ret_val) const;
        bool add(const std::string& entity, uintmax_t unicode);

    private:
        std::string em_name;
        bool entity_based;

        EntityPairMap entities;
    };

    typedef std::map<std::string, EntityMap*> GlyphMap;
    typedef std::list<const EntityMap*> EntityMapPtrList;

    // ================================================================
    //
    class FontData {
    public:
        enum eFontFlags {
            FONT_FLAGS_STYLE_NONE          = 0x0000,

            // style flags
            FONT_FLAGS_STYLE_NORMAL        = 0x0001,
            FONT_FLAGS_STYLE_BOLD          = 0x0002,
            FONT_FLAGS_STYLE_ITALIC        = 0x0004,
            FONT_FLAGS_STYLE_BOLD_ITALIC   = (FONT_FLAGS_STYLE_BOLD | FONT_FLAGS_STYLE_ITALIC),

            FONT_FLAGS_STYLE_MASK          = 0x000f,

            FONT_FLAGS_USES_ENTITY_CODES   = 0x0010,

            // general font flags
            FONT_FLAGS_BUNDLED_FONT        = 0x0100, // found on all windows systems, etc.
            FONT_FLAGS_UNKNOWN_FONT        = 0x0200, // new font we couldn't identify - report warning so we can look and see if it can be mapped
            FONT_FLAGS_IGNORE_FONT         = 0x0400, // for text content we know we can ignore

            // behavior flags - set in mapping
            FONT_FLAGS_USE_REGEX           = 0x1000, // pattern value is an actual regex to match against
            FONT_FLAGS_IGNORE_FD           = 0x2000, // do not read font flags from descriptor
        };

        // constructor for loading maps from config file
        FontData(const std::string& font_map_name,
                 const std::string& font_name_pattern, const std::string& subst_font, uint16_t font_flags,
                 const EntityMapPtrList& glyphmaps, const std::string& font_c2g_md5 = "") :
            map_name(font_map_name),
            name_pattern(font_name_pattern),
            subst_font_name(subst_font),
            flags(font_flags),
            mappers(glyphmaps),
            c2g_md5(font_c2g_md5)
        { }
        // constructor to set the pattern of a font in a PDF and copy
        // the other properties of a FontData from the list of config
        // mappings
        FontData(const std::string& doc_font_name, uint16_t style_flags, const FontData& config_fd);

        // constructor for fonts we've determined to be "bundled" but we have no mapping for
        FontData(const std::string& doc_font_name, const std::string& subst_font, uint16_t style_flags) :
            name_pattern(doc_font_name),
            subst_font_name(subst_font),
            flags(style_flags | FONT_FLAGS_BUNDLED_FONT) // mark as bundled
        { }
        // constructor for unrecognized fonts
        FontData(const std::string& doc_font_name, uint16_t style_flags) :
            name_pattern(doc_font_name),
            subst_font_name(doc_font_name),
            flags(style_flags | FONT_FLAGS_UNKNOWN_FONT) // mark as UNKNOWN
        { }

        const std::string& name()  const { return map_name; }
        // name of the font to be used in output
        const std::string& font_name_pattern() const { return name_pattern; }
        const std::string& output_font() const { return subst_font_name; }
        const std::string& c2gmd5() const { return c2g_md5; }
        const EntityMapPtrList& mapper_list() const { return mappers; }

        bool has_user_map()        const { return (!mappers.empty()); }
        bool has_style_set()       const { return ((flags & FONT_FLAGS_STYLE_MASK) != FONT_FLAGS_STYLE_NONE); }
        bool is_bold()             const { return (flags & FONT_FLAGS_STYLE_BOLD); }
        bool is_italic()           const { return (flags & FONT_FLAGS_STYLE_ITALIC); }
        bool uses_entity_codes()   const { return (flags & FONT_FLAGS_USES_ENTITY_CODES); }
        bool is_system_font()      const { return (flags & FONT_FLAGS_BUNDLED_FONT); }
        bool is_unknown_font()     const { return (flags & FONT_FLAGS_UNKNOWN_FONT); }
        bool is_ignored()          const { return (flags & FONT_FLAGS_IGNORE_FONT); }
        bool use_regex()           const { return (flags & FONT_FLAGS_USE_REGEX); }
        bool ignore_fd()           const { return (flags & FONT_FLAGS_IGNORE_FD); }
        bool has_c2g_md5()         const { return !c2g_md5.empty(); }

        bool map_name_cmp(const char* pattern, bool allow_regex) const;

        bool remap_glyph(const FontSource* font_src, uint32_t code, uintmax_t& remapped) const;

    private:
        std::string map_name;
        std::string name_pattern;
        std::string subst_font_name;
        uint16_t flags;
        EntityMapPtrList mappers;
        std::string c2g_md5;

        bool entity_lookup(const std::string& entity, uintmax_t& remapped) const;
    };


    // ================================================================
    //
    class DocFontMaps {
    public:
        DocFontMaps() : system_map_ptr(font_maps.end()) { }
        DocFontMaps(const DocFontMaps&) = delete;
        ~DocFontMaps() { clear(); }

        void clear();
        bool add_font_map(const std::string& map_name,
                          const std::string& file_font_name, const std::string& subst_font_name,
                          uint16_t flags, const std::list<const char*>& glyphmaps,
                          const std::string& c2g_md5, bool pre_insert);
        bool add_undef_entity_font_map(const std::string& map_name,
                                       const std::string& file_font_name, const std::string& subst_font_name,
                                       uint16_t flags, const std::list<const char*>& glyphmaps);
        bool add_glyph_map(const std::string& map_name, const std::string& code, uintmax_t unicode);

        pdftoedn::FontData* check_font_map(const pdftoedn::FontSource* const font_source);

        bool search_std_map(const std::string& entity, uintmax_t& remapped) const;

        friend std::ostream& operator<<(std::ostream& o, const DocFontMaps& fm);

    private:

        // glyph map table
        GlyphMap doc_glyph_maps;
        std::list<FontData*> font_maps;
        std::list<FontData*> undef_entity_font_maps;
        std::list<FontData*>::iterator system_map_ptr;

        bool glyph_list_valid(const std::list<const char*>& gm) const;
        bool make_entity_list(const std::list<const char*>& mapper_names, EntityMapPtrList& mappers);
    };

    // defined in main.cc
    extern DocFontMaps doc_font_maps;
}
