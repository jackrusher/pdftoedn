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

#include <iomanip>
#include <fstream>
#include <string>

#include <poppler/GfxFont.h>

#include "font_engine.h"
#include "pdf_font_source.h"
#include "pdf_output_dev.h"
#include "font.h"
#include "text.h"
#include "util_debug.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // FontEngine
    //

    //
    // destructor
    FontEngine::~FontEngine()
    {
        util::delete_ptr_map_elems(fonts);
        if (ft_lib) {
            FT_Done_FreeType(ft_lib);
        }
    }

    //
    // init freetype
    FontEngine::FontEngine(XRef *doc_xref) :
        xref(doc_xref), has_font_warnings(false),
        ft_lib(NULL), cur_doc_font(NULL)
    {
        FT_Library ftl;

        // set up freetype
        if (FT_Init_FreeType(&ftl) != 0) {
            throw init_error("Error initializing FreeType");
        }

        ft_lib = ftl;
    }

    //
    // look up the font in our cache
    pdftoedn::PdfFont* FontEngine::find_font(GfxFont* gfx_font) const
    {
        FontList::const_iterator fi = fonts.find(PdfRef(gfx_font->getID()));
        if (fi != fonts.end()) {
            return fi->second;
        }
        return NULL;
    }

    //
    // select and clean up the returned name
    std::string FontEngine::sanitize_font_name(const std::string& font_name)
    {
        std::string clean_name;

        if (!font_name.empty()) {
            // TESLA-7240: sanitize font names
            for (char c : font_name) {
                // ignore any non-printable characters and replace
                // spaces with '-'
                if (std::isgraph(c)) {
                    clean_name += c;
                } else if (std::isspace(c)) {
                    clean_name += '-';
                }
            }
        }
        return clean_name;
    }


    //
    // looks up a font in the Font Cache.. if not found, allocates an entry
    PdfFont* FontEngine::load_font(GfxFont* gfx_font)
    {
        GfxFontLoc *gfx_font_loc = NULL;
        FontSource* font_src = NULL;

        do
        {
            if (cur_doc_font) {
                // if we are already looking at a font, check if we
                // encountered any potential issues with it before
                // updating it so we can flag the document
                if (cur_doc_font->has_warnings()) {
                    has_font_warnings = true;
                }

                // unset the current font - it'll get re-set below
                cur_doc_font = NULL;
            }

            // can't handle Type 3 fonts
            GfxFontType font_type = gfx_font->getType();
            if (font_type == fontType3 || font_type == fontUnknownType) {
                std::stringstream err;
                err << "Unsupported font type (" << util::debug::get_font_type_str(font_type) << ") for ref " << PdfRef(gfx_font->getID());

                if (font_type == fontType3)
                    et.log_warn(ErrorTracker::ERROR_FE_FONT_READ_UNSUPPORTED, MODULE, err.str() );
                else
                    et.log_error(ErrorTracker::ERROR_FE_FONT_READ_UNSUPPORTED, MODULE, err.str() );
                return NULL;
            }

            // look up the font
            PdfFont* font = find_font(gfx_font);
            if (!font)
            {
                // not cached.. locate the font in the document
                if (!(gfx_font_loc = gfx_font->locateFont(xref, NULL))) {
                    std::stringstream err;
                    err << "locateFont failed for ref " << PdfRef(gfx_font->getID());
                    et.log_error(ErrorTracker::ERROR_FE_FONT_READ, MODULE, err.str() );
                    break;
                }

                std::string font_name = sanitize_font_name( (gfx_font->getName() ? gfx_font->getName()->getCString() : "") );

                // is the font embedded?
                if (gfx_font_loc->locType == gfxFontLocEmbedded) {

                    // check the embedded font name
                    std::string embedded_name = sanitize_font_name( (gfx_font->getEmbeddedFontName() ?
                                                                     gfx_font->getEmbeddedFontName()->getCString() :
                                                                     "" ) );

                    // poppler modifies embedded font names to clean
                    // them up by removing the garbled prefix
                    // (e.g. AFXEAD+Helvetica) and other unwanted
                    // characters so we generally want the embedded
                    // name to properly identify an embedded
                    // font. However, docs have carried fonts where
                    // the returned embedded name does not include a
                    // suffix we also use to make it unique.
                    //
                    // So, we only substitute the font_name with the
                    // embedded_name if:
                    //
                    // 1. font_name is ""
                    //
                    // 2. There is a '+' in the embedded name at index
                    // 6 but there's not one in font_name (sometimes
                    // font_name and embedded_name return different
                    // garbled characters - we keep font_name in that
                    // case)
                    //
                    // 3. The font_name starts with the embedded name
                    // but the embedded name is a longer string (e.g.:
                    // font_name is 'TTA203EA48t00_2' but
                    // embedded_name is 'TTA203EA48t00')
                    if ( font_name.empty() ||
                         (!embedded_name.empty() &&
                          font_name != embedded_name &&
                          (
                           ((font_name.find('+') != 6) && (embedded_name.find('+') == 6)) ||
                           ((font_name.find(embedded_name) == 0) && font_name.length() < embedded_name.length()) )
                          )
                         ) {
                        font_name = embedded_name;
                    }

                    // yes, read it into a buffer
                    int buf_len;
                    uint8_t *buf = (uint8_t*) gfx_font->readEmbFontFile(xref, &buf_len);

                    if (!buf) {
                        std::stringstream err;
                        err << "readEmbFontFile failed for " << PdfRef(gfx_font->getID());
                        et.log_error(ErrorTracker::ERROR_FE_FONT_READ, MODULE, err.str());
                        break;
                    }
#if 0
                    if (gfx_font->isCIDFont()) {
                        std::cerr << "export font " << font_name << ", type: " << util::debug::get_font_type_str(font_type) << std::endl;

                        std::ofstream file;
                        std::string path = util::debug::expand_environment_variables("${HOME}") + "Desktop/" + font_name + util::debug::get_font_file_extension(font_type);
                        file.open(path.c_str());
                        file.write((const char*) buf, buf_len);
                        file.close();
                    }
#endif

                    // create a buffer instance to manage this data
                    font_src = new FontSource(gfx_font,
                                              util::poppler_gfx_font_type_to_edn(font_type),
                                              font_name, ft_lib, buf, buf_len);

                    // galloc'd by GfxFont::readEmbFontFile() call to Stream::toUnsignedChars()
                    gfree((void*)buf);

                } else {
                    // no.. it's a system font (gfxFontLocExternal)

                    // create a file instance of the system font.
                    // notice that font type is overridden from the
                    // gfx_font_loc data!!
                    font_src = new FontSource(gfx_font,
                                              util::poppler_gfx_font_type_to_edn(font_type), // TODO: use system type? gfx_font_loc->fontType
                                              font_name, gfx_font_loc->path->getCString());

                    #if 0
                    if (font_src->is_cid() && !font_src->has_code_to_gid()) {
                        std::stringstream err;
                        err << "Document font \"" << font_name
                            << "\" indicates it is not embedded but has type: " << util::debug::get_font_type_str(font_src->font_type())
                            << " and lacks map table";
                        et.log_warn(ErrorTracker::ERROR_FE_FONT_READ, MODULE, err.str());
                    }
                    #endif
                }

                // check that the font was read correctly
                if (!font_src->is_ok()) {
                    std::stringstream err;
                    err << "couldn't create PdfFont entry for '" << font_name
                        << "' - type: " << util::debug::get_font_type_str(font_type);
                    et.log_error(ErrorTracker::ERROR_FE_FONT_READ, MODULE, err.str());
                    break;
                }

                // create a new font instance; try to lookup the font
                // in our known list to see if we can do any glyph
                // remapping
                font = new PdfFont(font_src, doc_font_maps.check_font_map(font_src));

                fonts.insert( FontListEntry(font_src->font_ref(), font) );
            }

            // update the current font pointer
            cur_doc_font = font;

            // clean up
            delete gfx_font_loc;

            // and exit, returning the scaled font
            return font;

        } while (0);

        // error encountered.. clean up
        delete gfx_font_loc;
        delete font_src;

        return NULL;
    }


    //
    // lookup the output unicode value either from the table carried
    // by the font or our own map tables
    FontEngine::eCodeRemapStatus FontEngine::get_code_unicode(CharCode code, Unicode* const u,
                                                              uintmax_t& unicode_r)
    {
        if (!cur_doc_font) {
            unicode_r = L' ';
            return CODE_REMAP_ERROR;
        }

        //
        // some docs have text data we can ignore if we want
        if (cur_doc_font->is_ignored()) {
            return CODE_REMAP_IGNORE;
        }

        // if we've mapped this font, try our maps along with standard
        // mappings, etc.
        if (cur_doc_font->remap_glyph(code, u, unicode_r) != PdfFont::REMAP_FAIL) {
            return CODE_REMAPPED_BY_EDSEL;
        }

        std::stringstream warn;
        warn << __FUNCTION__ << " encountered font '" << cur_doc_font->name() << "' that may need mappings or be exported";
        et.log_error(ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, warn.str());
        unicode_r = code;
        return CODE_REMAP_ERROR;
    }

} // namespace
