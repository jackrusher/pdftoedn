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

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <set>
#include <list>
#include <boost/regex.hpp>

#include "util.h"
#include "font_maps.h"
#include "pdf_font_source.h"

#undef ENABLE_FONT_LOOKUP_TRACE

#ifdef ENABLE_FONT_LOOKUP_TRACE
#include "util_debug.h"
#define DBG_TRACE(cmd) cmd
#else
#define DBG_TRACE(cmd)
#endif

namespace pdftoedn
{
    const std::string EntityMap::STANDARD_MAP_NAME = "standardMap";

    // ================================================================================
    // EntityMap types handle storing entity (or hex code) to unicode pairs
    //
    bool EntityMap::find(const std::string& entity, uintmax_t& ret_val) const
    {
        auto en_it = entities.find(entity);
        if (en_it == entities.end()) {
            return false;
        }

        ret_val = en_it->second;
        return true;
    }

    bool EntityMap::add(const std::string& entity, uintmax_t unicode)
    {
        auto it = entities.find(entity);
        if (it != entities.end()) {
            std::cerr << em_name << " already has entity for " << entity << " - OVERWRITING" << std::endl;
        }

        // two type of keys for this map: standard entity strings
        // ("one", "registered", etc) or 4-nibble hex codes for
        // non-type1 fonts. Mark this as entity-based if either is not
        // the case
        if (!entity_based &&
            (entity.length() != 4 ||
             entity.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)) {
            entity_based = true;
        }

        entities[entity] = unicode;
        return true;

    }


    // ================================================================================
    // FontData constructors - this one is used when reading a list of
    // map configs from a file where all styles are set
    //


    // constructor to set the pattern of a font in a PDF and copy
    // the other properties of a FontData from the list of config
    // mappings
    FontData::FontData(const std::string& doc_font_name, uint16_t style_flags, const FontData& config_fd) :
        map_name(config_fd.map_name),
        name_pattern(doc_font_name),
        subst_font_name(config_fd.subst_font_name),
        flags(style_flags | config_fd.flags), // combine
        c2g_md5(config_fd.c2g_md5)
    {
        // run through the mapper list and add the ones that match
        // this font's requirements: if it uses entity codes, only add
        // entity-based maps; if it does not, only add hex maps
        for (const EntityMap* m : config_fd.mappers) {
            if ( (uses_entity_codes() && m->is_entity_based()) ||
                 (!uses_entity_codes() && !m->is_entity_based()) ) {
                mappers.push_back(m);
            }
        }
    }

    //
    // compares the pattern against the one in the font_map
    bool FontData::map_name_cmp(const char* pattern, bool allow_regex) const
    {
        bool match = false;
        if (use_regex()) {
            if (allow_regex) {
                boost::cmatch what;
                match = boost::regex_match(pattern, what, boost::regex(name_pattern), boost::match_default);

                DBG_TRACE(if (what[0].matched) std::cerr << "GOT A MATCH" << std::endl;);
            }
        }
        else {
            // otherwise, normal substring compare
            match = strcasestr(pattern, name_pattern.c_str());
        }

        return match;
    }


    //
    // search through the list of mappers for the given entity
    bool FontData::entity_lookup(const std::string& entity, uintmax_t& remapped) const
    {
        if (entity.length() == 0) {
            // sometimes we get undef'd entities..
            return false;
        }

        // look for the entity in each mapper in the list
        auto ii = std::find_if( mappers.begin(), mappers.end(),
                                [&](const EntityMap* m) { return m->find(entity, remapped); }
                                );
        if (ii != mappers.end()) {
            return true;
        }
        return false;
    }


    //
    // use our lookup tables to try and identify the given glyph. if
    // this fails, we fall back to path drawing
    bool FontData::remap_glyph(const FontSource* font_src, uint32_t code, uintmax_t& remapped) const
    {
        if (font_src->is_type1())
        {
            // there must be an encoding
            const Encoding* enc = font_src->get_encoding();
            if (enc) {
                return entity_lookup(enc->entity(code), remapped);
            }
        }
        else if (font_src->is_truetype() || font_src->is_cid())
        {
            // some CID and some TT fonts with custom encodings don't
            // use entities for lookup. The code is the position in
            // the table so we "simulate" entities creating a hex
            // string

            // generate an entity for the lookup into our table, using the code
            char hex_code[12];

            // try with the code value
            snprintf(hex_code, 11, "%04x", code);

            return entity_lookup(hex_code, remapped);
        }
        return false;
    }


    // ================================================================================
    // font maps
    //

    //
    // delete all loaded maps
    void DocFontMaps::clear()
    {
        // delete allocated maps
        util::delete_ptr_container_elems(undef_entity_font_maps);
        undef_entity_font_maps.clear();

        util::delete_ptr_container_elems(font_maps);
        font_maps.clear();
        system_map_ptr = font_maps.end();

        util::delete_ptr_map_elems(doc_glyph_maps);
        doc_glyph_maps.clear();
    }

    //
    // extracts name from original font name as found in PDF. Removes
    // leading cruft up to a '+', and trailing "Bold", "Oblique", etc.
    static inline std::string parse_font_family_name(const std::string& pdf_font_name)
    {
        std::string font_name(pdf_font_name);
        uintmax_t start = 0;
        uintmax_t end = pdf_font_name.length();

        // lowercase it
        std::transform(font_name.begin(), font_name.end(), font_name.begin(), ::tolower);

        // search for trailing style attributes - anything following
        // these substrings will also be dropped
        static const char* style_str[] = {
            "-bold", "bold", "-italic", "italic", "-oblique", "oblique", nullptr
        };

        uintmax_t style_start;
        for (uintmax_t i = 0; style_str[i] != nullptr; i++) {
            style_start = font_name.find(style_str[i]);

            if (style_start != std::string::npos) {
                if (style_start < end) {
                    end = style_start;

                    // drop a ',' separating family and style if present
                    if (font_name[end-1] == ',')
                        end--;
                }
            }
        }

        // search for a prefix followed by a '+' (for names such as
        // 'GFAFFD+TT5Fo00')
        uintmax_t plus_pos = font_name.find("+");
        if (plus_pos != std::string::npos) {
            start = plus_pos + 1;
        }

        // return the isolated name
        return pdf_font_name.substr(start, end - start);
    }


    //
    // check if the name is in the list of web fonts
    static inline bool is_bundled_font(const std::string& family)
    {
        //
        // Translation table for font names.  These are searched in order using
        // case insensitive comparisons.
        //
        static const char* bundledFonts[] = {
            "Arial",
            "Arial Narrow",
            "Arial Black",
            "Comic Sans MS",
            "Courier",
            "Courier New",
            "Garamond",
            "Georgia",
            "Helvetica",
            "Impact", // Charcoal,
            "Lucida Console",// Monaco
            "Lucida Sans Unicode",// Lucida Grande
            "Palatino Linotype", //Book Antiqua, Palatino
            "Tahoma", //Geneva
            "Times",
            "Times New Roman", //Times New Roman, Times
            "Trebuchet MS",
            "Verdana", // Geneva
            "Symbol",
            "Webdings",
            "Wingdings",
            "Zapf Dingbats",
            "MS Sans Serif", //Geneva
            "MS Serif", //New York

            // delimiter
            nullptr
        };

        for (uint_fast8_t i = 0; bundledFonts[i] != nullptr; i++) {
            if (std::strstr(bundledFonts[i], family.c_str()) != nullptr) {
                DBG_TRACE(std::cerr << "\tmatched bundled font" << std::endl);
                return true;
            }
        }
        return false;
    }


    //
    // parses name to determine font style
    static inline uint8_t parse_font_style(const std::string& lc_pdf_font_name)
    {
        uint8_t flags = FontData::FONT_FLAGS_STYLE_NONE;
        if (lc_pdf_font_name.find("bold") != std::string::npos) {
            flags |= FontData::FONT_FLAGS_STYLE_BOLD;
        }

        if ( (lc_pdf_font_name.find("italic") != std::string::npos) ||
             (lc_pdf_font_name.find("oblique") != std::string::npos) ) {
            flags |= FontData::FONT_FLAGS_STYLE_ITALIC;
        }

        return flags;
    }



    //
    // looks up font entry based on pattern - allocates a FontData
    // which the Font instance must delete
    pdftoedn::FontData* DocFontMaps::check_font_map(const pdftoedn::FontSource* const font_source)
    {
        std::string pdf_font_name = font_source->font_name();

        // remove whitespace from the font name
        std::string pdf_font_name_no_ws, s;
        std::istringstream iss(pdf_font_name);

        while (iss >> s){
            pdf_font_name_no_ws.append(s);
        }

        // lower-case it. TODO: fix case-handling for regex patterns
        std::string lc_font_name_no_ws(pdf_font_name_no_ws);
        util::tolower(lc_font_name_no_ws);

        DBG_TRACE(
                  std::cerr << "-----------------------------------------------------------------------" << std::endl
                            << " looking up " << lc_font_name_no_ws << std::endl
                            << (font_source->md5().empty() ? "" : " C2G MD5: ") << font_source->md5() << std::endl
                 );

        std::string parsed_family = parse_font_family_name(pdf_font_name);

        DBG_TRACE(std::cerr << " called parse_font_family_name, got: \"" << parsed_family << "\"" << std::endl);

        bool bundled_font = is_bundled_font(parsed_family);

        const FontData* config_fd = nullptr;

        // if the font does not have a to_unicode map, we must have a map of our own
        if (!font_source->has_to_unicode() ||
            (font_source->has_to_unicode() && !font_source->has_code_to_gid()))
        {
            DBG_TRACE(std::cerr << "\tno unicode - searching alternate tables first" << std::endl);

            auto ii = std::find_if( undef_entity_font_maps.begin(), undef_entity_font_maps.end(),
                                    [&,bundled_font](const FontData* fd) { return fd->map_name_cmp(lc_font_name_no_ws.c_str(), !bundled_font); }
                                    );
            if (ii != undef_entity_font_maps.end()) {
                DBG_TRACE(std::cerr << "\t\tMATCH FOUND IN ALT TABLE" << std::endl);
                config_fd = *ii;
            }
        }

        // not found in the table of fonts without a c2g map.. try the standard table
        if (!config_fd)
        {
            DBG_TRACE(std::cerr << "\tsearching in std table" << std::endl);

            bool c2g_match = true;
            for (const FontData* fd : font_maps)
            {
                if (fd->map_name_cmp(lc_font_name_no_ws.c_str(), !bundled_font))
                {
                    // TESLA-6478: we've found documents with embedded
                    // font names but differing mappings across documents.
                    // Work-around: identify these by using the c2g md5
                    if (fd->has_c2g_md5()) {
                        if (font_source->md5() != std::string(fd->c2gmd5())) {
                            c2g_match = false;
                            continue;
                        }

                        DBG_TRACE(std::cerr << "cmp: " << fd->c2gmd5() << " vs " << font_source->md5() << std::endl);

                        c2g_match = true;
                    }

                    DBG_TRACE(std::cerr << "\tMATCH" << std::endl);

                    config_fd = fd;

                    if (!c2g_match) {
                        // tried to compare c2g md5s but didn't have an exact match - this could be a wrong map
                        std::stringstream err;
                        err << __FUNCTION__ << " found an instance of font '" << pdf_font_name
                            << "' (" << font_source->md5() << ") that may have multiple mappings. Check output.";
                        pdftoedn::et.log_warn(ErrorTracker::ERROR_FE_FONT_MAPPING_DUPLICATE, MODULE, err.str());
                    }
                    break;
                }
            }
        }

        uint16_t flags = FontData::FONT_FLAGS_STYLE_NONE;

        // mark if this font should use entities for unicode lookups
        if (font_source->has_encoding() && font_source->is_type1()) {
            flags |= FontData::FONT_FLAGS_USES_ENTITY_CODES;
        }

        // if either loop got a match
        if (config_fd)
        {
            if (!config_fd->ignore_fd()) {
                // some font names carry the style (e.g.:
                // Helvetica-Condensed-Bold) so try to parse it
                flags |= parse_font_style(lc_font_name_no_ws);
            }

            DBG_TRACE(std::cerr << "\t\tmatched with " << config_fd->font_name_pattern() << ", using: " << config_fd->output_font() << std::endl);

            // return a copy with the matching config's properties
            return new FontData(pdf_font_name, flags, *config_fd);
        }

        // parse the style from the name
        flags |= parse_font_style(lc_font_name_no_ws);

        // not found in our font mapping list..  if the name seems
        // like a bundled system font, set up a font data with the family name
        if (bundled_font) {
            return new FontData(pdf_font_name, parsed_family, flags);
        }

        // this is probably an embedded font with some random name we
        // don't recognize. Override family with the original font
        // name - this will mark it as UNKOWN
        DBG_TRACE(std::cerr << "FAILED TO MATCH" << std::endl);
        return new FontData(pdf_font_name, flags);
    }


    //
    // convert a list of map names to a list of pointers to the
    // corresponding entity maps
    bool DocFontMaps::make_entity_list(const std::list<const char*>& mapper_names,
                                       EntityMapPtrList& mappers)
    {
        if (mapper_names.empty())
            return false;

        for (const char* m : mapper_names) {
            auto jj = doc_glyph_maps.find(m);
            if (jj != doc_glyph_maps.end()) {
                mappers.push_back((jj->second));
            }
        }
        return true;
    }


    //
    // add an entry to the font map list
    bool DocFontMaps::add_font_map(const std::string& map_name,
                                   const std::string& file_font_name, const std::string& subs_font_name,
                                   uint16_t flags, const std::list<const char*>& glyphmap_names,
                                   const std::string& c2g_md5, bool pre_insert)
    {
        // ensure the glyph maps referenced are registered
        if (!glyph_list_valid(glyphmap_names)) {
            return false;
        }

        // instantiate a font data entry
        EntityMapPtrList mappers;
        make_entity_list(glyphmap_names, mappers);

        FontData* fd = new pdftoedn::FontData(map_name,
                                              file_font_name, subs_font_name,
                                              flags, mappers, c2g_md5);

        // the font map list is unsorted following the order specified
        // in the config. However, certain maps may be specified to
        // pre-pend the default list so certain fot names are handled
        // prior to a generic name For this reason, system_map_ptr is
        // set to the first entry added to the list (from the default
        // config file).
        if (pre_insert && system_map_ptr != font_maps.end()) {
            // insert before the standardMap
            font_maps.insert(system_map_ptr, fd);
        }
        else {
            // insert it at the end of the list
            font_maps.push_back(fd);

            // once the first entry is added, set system_map_ptr to
            // the head of the list
            if (font_maps.size() == 1) {
                system_map_ptr = font_maps.begin();
            }
        }
        return true;
    }


    //
    // some fonts have undefined entities for glyph lookups. We have
    // separate maps for these
    bool DocFontMaps::add_undef_entity_font_map(const std::string& map_name,
                                                const std::string& file_font_name, const std::string& subst_font_name,
                                                uint16_t flags, const std::list<const char*>& glyphmap_names)
    {
        // ensure the glyph maps referenced are registered
        if (!glyph_list_valid(glyphmap_names)) {
            return false;
        }

        // instantiate a font data entry and store it
        EntityMapPtrList mappers;
        make_entity_list(glyphmap_names, mappers);

        undef_entity_font_maps.push_back( new pdftoedn::FontData(map_name,
                                                                 file_font_name, subst_font_name,
                                                                 flags, mappers) );
        return true;
    }


    //
    // add entries to the glyph map
    bool DocFontMaps::add_glyph_map(const std::string& map_name, const std::string& code, uintmax_t unicode)
    {
        EntityMap* em = nullptr;
        auto em_it = doc_glyph_maps.find(map_name);

        if (em_it == doc_glyph_maps.end()) {
            em = new EntityMap(map_name);
            doc_glyph_maps[em->name()] = em;
        } else {
            em = em_it->second;
        }

        return em->add(code, unicode);
    }


    //
    // check that the entries in the glyph map list are registered
    bool DocFontMaps::glyph_list_valid(const std::list<const char*>& gm) const
    {
        // make sure the glyphmaps are valid (they should exist in the document map list)
        if (std::any_of( gm.begin(), gm.end(),
                         [&](const char* c) {
                             if (doc_glyph_maps.find(c) == doc_glyph_maps.end()) {
                                 std::cerr << "glyph map " << c << " does not exist" << std::endl;
                                 return true;
                             }
                             return false;
                         }
                         )) {
            return false;
        }
        return true;
    }


    //
    // searches for an entity in the standard map
    bool DocFontMaps::search_std_map(const std::string& entity, uintmax_t& remapped) const
    {
        auto std_map_it = doc_glyph_maps.find(EntityMap::STANDARD_MAP_NAME);

        if (std_map_it != doc_glyph_maps.end()) {
            return (std_map_it->second)->find(entity, remapped);
        }
        return false;
    }


    //
    // output the list of loaded maps
    static void output_font_list(std::ostream& o, const std::list<FontData*>& fdl)
    {
        for (const FontData* fd : fdl) {
            o << std::setw(40) << fd->font_name_pattern()
              << " "
              << fd->output_font() << std::endl;
        }
    }


    std::ostream& operator<<(std::ostream& o, const DocFontMaps& fm)
    {
        if (!fm.font_maps.empty()) {
            output_font_list(o, fm.font_maps);
        }

        if (!fm.undef_entity_font_maps.empty()) {
            output_font_list(o, fm.undef_entity_font_maps);
        }
        return o;
    }

}
