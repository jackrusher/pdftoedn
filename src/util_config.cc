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

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <list>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

//#define DUMP_READ_JSON_DATA
#ifdef DUMP_READ_JSON_DATA
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#endif

#include "util.h"
#include "util_config.h"
#include "font_maps.h"

namespace pdftoedn
{
    namespace util
    {
        namespace config
        {
            //
            static inline bool is_comment_key(const char* str)
            {
                static const char*   COMMENT_KEY_STR = "_comment_";
                static const uint8_t COMMENT_KEY_STR_LEN = std::strlen(COMMENT_KEY_STR);

                return (std::strncmp(str, COMMENT_KEY_STR, COMMENT_KEY_STR_LEN) == 0);
            }

            // ================================================================================
            // parses the glyph mapping portion of the config file
            //
            static bool read_glyph_maps(const rapidjson::Value& glyphmaps, DocFontMaps& maps)
            {
                for (rapidjson::Value::ConstMemberIterator ii = glyphmaps.MemberBegin();
                     ii != glyphmaps.MemberEnd(); ++ii)
                {
                    const char* map_name = ii->name.GetString();

                    // ignore maps named _comment_
                    if (is_comment_key(map_name) || (map_name[0] == '0')) {
                        continue;
                    }

                    const rapidjson::Value& ent_list = ii->value;
                    if (!ent_list.IsObject()) {
                        continue;
                    }

                    for (rapidjson::Value::ConstMemberIterator ent_ii = ent_list.MemberBegin();
                         ent_ii != ent_list.MemberEnd(); ++ent_ii)
                    {
                        const char* ent_str = ent_ii->name.GetString();

                        // ignore entities named _comment_ or empty strings
                        if (is_comment_key(ent_str) || (ent_str[0] == 0)) {
                            continue;
                        }

                        // get the value
                        const rapidjson::Value& ent_data = ent_ii->value;

                        // 1st entry should be a unicode value or a character in string form
                        if (!ent_data.IsInt() && !ent_data.IsString()) {
                            continue;
                        }

                        // only need the first item (unicode value)
                        uint32_t unicode;
                        if (ent_data.IsString()) {
                            // looks like a string but make sure it's not empty
                            if (ent_data.GetStringLength() > 0) {
                                unicode = static_cast<uint32_t>((ent_data.GetString())[0]);
                            } else {
                                // ignore it
                                continue;
                            }
                        } else {
                            unicode = ent_data.GetInt();
                        }

                        maps.add_glyph_map(map_name, ent_str, unicode);
                    }

                }
                return true;
            }


            //
            // parses the font mappings portion of the config
            static bool read_font_maps(const rapidjson::Value& fontmaps, pdftoedn::DocFontMaps& maps)
            {
                //
                // format is [ "map name" {
                //               "prelist": true    (optional)
                //               "entries": [
                for (rapidjson::Value::ConstMemberIterator ii = fontmaps.MemberBegin();
                     ii != fontmaps.MemberEnd(); ++ii)
                {
                    const char* map_name = ii->name.GetString();

                    const rapidjson::Value& map_data = ii->value;
                    if (!map_data.IsObject()) {
                        continue;
                    }

                    // go through the hash for the map
                    bool pre_insert = false;
                    bool undef_entity_map = false;

                    for (rapidjson::Value::ConstMemberIterator m_ii = map_data.MemberBegin();
                         m_ii != map_data.MemberEnd(); ++m_ii)
                    {
                        const char* m_key = m_ii->name.GetString();

                        if (std::strcmp(m_key, "entries") == 0)
                        {
                            const rapidjson::Value& fm_list = m_ii->value;
                            if (!fm_list.IsArray()) {
                                continue;
                            }

                            for (rapidjson::SizeType i = 0; i < fm_list.Size(); ++i)
                            {
                                // each map is an array of at least 2 elements
                                const rapidjson::Value& fm_item = fm_list[i];
                                if (!fm_item.IsArray() && (fm_item.Size() < 2)) {
                                    continue;
                                }

                                // ignore any "_comment_" entries
                                if (is_comment_key(fm_item[0].GetString())) {
                                    continue;
                                }

                                // third item is an array of properties
                                uint16_t flags = FontData::FONT_FLAGS_STYLE_NONE;
                                if ((fm_item.Size() > 2) && fm_item[2].IsArray()) {
                                    const rapidjson::Value& src_flags = fm_item[2];

                                    // parse properties from the array
                                    for (rapidjson::SizeType j = 0; j < src_flags.Size(); ++j) {
                                        if (!src_flags[j].IsString())
                                            continue;

                                        if (std::strcmp(src_flags[j].GetString(), "bold") == 0) {
                                            flags |= FontData::FONT_FLAGS_STYLE_BOLD;
                                        } else if (std::strcmp(src_flags[j].GetString(), "italic") == 0) {
                                            flags |= FontData::FONT_FLAGS_STYLE_ITALIC;
                                        } else if (std::strcmp(src_flags[j].GetString(), "regex") == 0) {
                                            flags |= FontData::FONT_FLAGS_USE_REGEX;
                                        } else if (std::strcmp(src_flags[j].GetString(), "ignore") == 0) {
                                            flags |= FontData::FONT_FLAGS_IGNORE_FONT;
                                        } else if (std::strcmp(src_flags[j].GetString(), "ignore_fd") == 0) {
                                            flags |= FontData::FONT_FLAGS_IGNORE_FD;
                                        }
                                    }

                                    // if neither bold nor oblique, set to normal
                                    if  ((flags & FontData::FONT_FLAGS_STYLE_MASK) == FontData::FONT_FLAGS_STYLE_NONE) {
                                        flags |= FontData::FONT_FLAGS_STYLE_NORMAL;
                                    }
                                }

                                //
                                // array of glyphmap names to use for this font (optional)
                                std::list<const char*> glyphmaps;

                                if (fm_item.Size() > 3 && fm_item[3].IsArray()) {
                                    const rapidjson::Value& glyph_maps = fm_item[3];
                                    for (rapidjson::SizeType j = 0; j < glyph_maps.Size(); ++j) {
                                        if (glyph_maps[j].GetStringLength() > 0) {
                                            glyphmaps.push_back(glyph_maps[j].GetString());
                                        }
                                    }
                                }

                                // c2g MD5 (optional) - found cases where
                                // embedded font have the same name across
                                // multiple documents but use different
                                // mappings
                                const char* c2g_md5 = "";
                                if ((fm_item.Size() > 4) && fm_item[4].IsString()) {
                                    c2g_md5 = fm_item[4].GetString();
                                }

                                if (undef_entity_map) {
                                    maps.add_undef_entity_font_map(map_name, fm_item[0].GetString(), fm_item[1].GetString(),
                                                                   flags, glyphmaps);
                                } else {
                                    maps.add_font_map(map_name, fm_item[0].GetString(), fm_item[1].GetString(),
                                                      flags, glyphmaps, c2g_md5, pre_insert);
                                }
                            }
                        } else if (std::strcmp(m_key, "insertAtHead") == 0) {
                            const rapidjson::Value& m_val = m_ii->value;
                            if (m_val.IsBool())
                                pre_insert = m_val.GetBool();
                        } else if (std::strcmp(m_key, "undefEntities") == 0) {
                            const rapidjson::Value& m_val = m_ii->value;
                            if (m_val.IsBool())
                                undef_entity_map = m_val.GetBool();
                        }
                    }
                }
                return true;
            }


            //
            // parses a font map configuration in JSON format
            bool read_map_config(DocFontMaps& maps, const char* const data)
            {
                rapidjson::Document d;
                d.Parse(data);

#ifdef DUMP_READ_JSON_DATA
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                d.Accept(writer);

                std::cerr << buffer.GetString() << std::endl;
#endif

                if (d.HasParseError()) {
                    std::stringstream err;
                    err << "Fontmap JSON parse error: " << GetParseError_En(d.GetParseError());
                    throw invalid_file(err.str());
                }

                if (!d.IsObject()) {
                    throw invalid_file("Invalid fontmap format - expecting an object at the root level");
                }

                // parse the glyph & font maps to build the lookup
                // tables. Do glyph maps first so we can confirm
                // references to them from the font maps are valid
                if (d.HasMember("glyphMaps")) {
                    const rapidjson::Value& glyphmaps = d["glyphMaps"];
                    if (!read_glyph_maps(glyphmaps, maps)) {
                        throw invalid_file("Invalid config format - error parsing glyph list");
                    }
                }

                if (!d.HasMember("fontMaps")) {
                    throw invalid_file("Invalid config file - no \"fontMaps\" entry found");
                }

                const rapidjson::Value& fontmaps = d["fontMaps"];
                if (!read_font_maps(fontmaps, maps)) {
                    throw invalid_file("Invalid config format - error parsing font map list");
                }
                return true;
            }
        }
    }
}
