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

#include <vector>
#include <string>
#include <sstream>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_ERRORS_H

#include <poppler/GfxFont.h>
#include <poppler/CharCodeToUnicode.h>
#include <poppler/fofi/FoFiTrueType.h>

#include "util.h"
#include "util_debug.h"
#include "font_engine.h"
#include "font.h"
#include "text.h"

namespace pdftoedn
{
    const pdftoedn::Symbol Encoding::SYMBOL_TYPE[] = {
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_MACOS_ROMAN)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_MACOS_EXPERT)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_WINANSI)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_EXPERT)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_SYMBOL)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_ZAPF_DINGBATS)),
        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_CUSTOM)),

        pdftoedn::Symbol(Encoding::get_encoding_str(Encoding::ENC_INVALID))
    };

    const double SCALING_BASE = 64.0;

    bool PdfRef::operator<(const PdfRef& r) const
    {
        if (num < r.num) {
            return true;
        }
        if (num == r.num && gen < r.gen) {
            return true;
        }
        return false;
    }

    // =============================================================================
    // code to glyph map container
    //
    CodeToGIDMap::CodeToGIDMap(uintmax_t len) :
        size(len), c2g_map( new int[len] )
    {
        memset(c2g_map, -1, len * sizeof(int));
    }

    CodeToGIDMap::CodeToGIDMap(uintmax_t len, int* code_to_GID_map) :
        size(len), c2g_map(NULL)
    {
        if (len > 0) {
            c2g_map = new int[len];
            memcpy(c2g_map, code_to_GID_map, len * sizeof(int));
        }
    }

    //
    // once all c2g mappings have been set, this needs to be called to
    // compute md5 of map
    void CodeToGIDMap::finalize()
    {
        if (c2g_map && size > 0) {
            std::stringstream c2gseq;

            for (uintmax_t ii = 0; ii < size; ++ii) {
                c2gseq << std::hex << c2g_map[ii];
            }

            c2g_md5 = util::md5(c2gseq.str());
        }
    }

    uintmax_t CodeToGIDMap::map(uint32_t code) const
    {
        if (c2g_map && code < size) {
            return c2g_map[code];
        }
        return code;
    }

    std::ostream& CodeToGIDMap::dump(std::ostream& o) const
    {
        o << "Code2GID num entries: " << size;
        return o;
    }

    // =============================================================================
    // encoding container
    //
    Encoding::Encoding(Gfx8BitFont* gfx_font) :
        e_type(ENC_INVALID)
    {
        if (gfx_font->getEncodingName()) {
            e_name = gfx_font->getEncodingName()->getCString();

            // set the type - MacRoman is set below as poppler
            // provides a flag already
            if (e_name.find("WinAnsi") != std::string::npos) {
                e_type = ENC_WINANSI;
            }
            else if (e_name.find("MacExpert") != std::string::npos) {
                e_type = ENC_MACOS_EXPERT;
            }
            else if (e_name.find("Expert") != std::string::npos) {
                e_type = ENC_EXPERT;
            }
            else if (e_name.find("Symbol") != std::string::npos) {
                e_type = ENC_SYMBOL;
            }
            else if (e_name.find("Zapf") != std::string::npos) {
                e_type = ENC_ZAPF_DINGBATS;
            }
            else if ((e_name.find("ustom") != std::string::npos) ||
                     (e_name.find("uiltin") != std::string::npos)) {
                e_type = ENC_CUSTOM;
            }
        }

        // if MacRoman, set it
        if (gfx_font->getUsesMacRomanEnc()) {
            e_type = ENC_MACOS_ROMAN;
        }

        // set the entity map if found
        const char** enc_map = (const char**) gfx_font->getEncoding();
        if (enc_map) {
            const char* entity;
            for (uint_fast16_t ii = 0; ii < 256; ++ii) {
                entity = (enc_map[ii] ? enc_map[ii] : "");
                e_map.push_back(entity);

                //                std::cerr << "  [" << ii << "]=> " << enc_map[ii] << std::endl;
            }
        }
    }

    //
    // returns the entity for the given code
    const char* Encoding::entity(uint32_t code) const
    {
        if (code < 256) {
            return e_map[code].c_str();
        }
        return NULL;
    }

    // debug
    const char* Encoding::get_encoding_str(Encoding::Type e)
    {
        switch (e)
        {
          case Encoding::ENC_MACOS_ROMAN:
              return "macos_roman";
          case Encoding::ENC_MACOS_EXPERT:
              return "macos_expert";
          case Encoding::ENC_WINANSI:
              return "winansi";
          case Encoding::ENC_EXPERT:
              return "expert";
          case Encoding::ENC_SYMBOL:
              return "symbol";
          case Encoding::ENC_ZAPF_DINGBATS:
              return "zapfdingbats";
          case Encoding::ENC_CUSTOM:
              return "custom";
          case Encoding::ENC_INVALID:
          default:
              return "INVALID";
        }
        return NULL;
    }

    std::ostream& Encoding::dump(std::ostream& o) const
    {
        o << "enc: " << e_name << " (" << get_encoding_str(e_type) << "), # entries: " << e_map.size();

#if 0
        for (uint_fast8_t ii = 0; ii < 256; ++ii) {
            if (e_map[ii].size()) {
                o << "  [" << ii << "] -> " << e_map[ii] << endl;
            }
        }
#endif
        return o;
    }



    // =============================================================================
    // contains properties of the font source (file or embedded binary
    // data). Size is not applicable here as this refers to character
    // sets, encodings, etc.
    //
    // constructor for embedded fonts - cache the font blob in a buffer
    FontSource::FontSource(GfxFont* gfx_font, FontType font_type, const std::string& font_name,
                           FT_Library lib, const uint8_t* buffer, uintmax_t len,
                           uintmax_t font_face_index) :
        ref(gfx_font->getID()),
        type(font_type),
        name(font_name),
        location(LOC_EMBEDDED),
        gfx_font_flags(gfx_font->getFlags()),
        encoding(NULL),
        code_to_gid(NULL),
        to_unicode((gfx_font->getToUnicode() != NULL) &&
                   // TODO: fix cast once poppler corrects GfxFont constness
                   ((const_cast<CharCodeToUnicode *>(gfx_font->getToUnicode()))->getLength() > 1)),
        ft_lib(lib), ft_face(NULL), face_index(font_face_index),
        font_ok(false)
    {
        // save a copy of the blob and compute its md5
        font_blob.append((const char*) buffer, len);
        blob_md5 = util::md5(font_blob);

        font_ok = load_font(gfx_font);
    }

    //
    // constructor for external fonts
    FontSource::FontSource(GfxFont* gfx_font, FontType font_type, const std::string& font_name,
                           const std::string& font_file) :
        ref(gfx_font->getID()),
        type(font_type),
        name(font_name),
        location(LOC_EXTERNAL),
        gfx_font_flags(gfx_font->getFlags()),
        encoding(NULL),
        code_to_gid(NULL),
        to_unicode((gfx_font->getToUnicode() != NULL) &&
                   // TODO: fix cast once poppler corrects GfxFont constness
                   ((const_cast<CharCodeToUnicode *>(gfx_font->getToUnicode()))->getLength() > 1)),
        ft_lib(NULL), ft_face(NULL), face_index(-1),
        filename(font_file),
        font_ok(false)
    {
#if 0
        if (type == FONT_TYPE_CIDTYPE0COT || is_cid2()) {
            GfxCIDFont* cid_font = dynamic_cast<GfxCIDFont*>(gfx_font);
            if (cid_font && cid_font->getCIDToGID()) {
                code_to_gid = new CodeToGIDMap(cid_font->getCIDToGIDLen(), cid_font->getCIDToGID());
                code_to_gid->finalize();
            }
        }
#endif
        font_ok = load_font(gfx_font);
    }


    FontSource::~FontSource()
    {
        if (ft_face) {
            FT_Done_Face(ft_face);
        }
        delete code_to_gid;
        delete encoding;
    }


    //
    // checks that the font's name is set; if not, it generates one from the refid
    void FontSource::check_name()
    {
        if (name.empty()) {
            std::stringstream name_stream;
            // NULL font name - fabricate one using ref values to identify it
            name_stream << "[" << ref << "]";
            name = name_stream.str();

            // log an error
            std::stringstream err;
            err << "NULL font name for ref " << name
                << ", type: " << util::debug::get_font_type_str(type);
            et.log_warn(ErrorTracker::ERROR_FE_FONT_READ, MODULE, err.str() );
        }
    }


    //
    // load a font
    bool FontSource::load_font(GfxFont* gfx_font)
    {
        bool status = false;

        // determine the general family type for CSS attribute generation
        if (gfx_font->isFixedWidth()) {
            family_type = FontSource::FAM_MONOSPACE;
        }
        else if (gfx_font->isSerif()) {
            family_type = FontSource::FAM_SERIF;
        }
        else {
            family_type = FontSource::FAM_SANS_SERIF;
        }

        // if it's a CID font, get the collection name
        GfxCIDFont* cid_font = NULL;
        Gfx8BitFont* g8_font = NULL;
        if ( !is_cid() )
        {
            g8_font = dynamic_cast<Gfx8BitFont*>(gfx_font);

            if (!g8_font) {
                std::stringstream err;
                err << __FUNCTION__ << " - GfxFont should be 8-bit but failed to cast: " << name;
                et.log_warn( ErrorTracker::ERROR_FE_FONT_FT, MODULE, err.str() );
                return status;
            }

            // if there looks to be an encoding, save
            if (is_type1() || (is_truetype() && g8_font->getHasEncoding())) {
                Encoding *e = new Encoding(g8_font);

                if (e->is_ok())
                    encoding = e;
                else
                    delete e;
            }

        } else {
            cid_font = dynamic_cast<GfxCIDFont*>(gfx_font);
            if (!cid_font) {
                std::stringstream err;
                err << __FUNCTION__ << " - GfxFont should be CID but failed to cast: " << name;
                et.log_warn( ErrorTracker::ERROR_FE_FONT_FT, MODULE, err.str() );
                return status;
            }

            GooString* col = cid_font->getCollection();
            if (col && (col->getLength() > 0)) {
                collection = col->getCString();
            }
        }

        if (location == LOC_EMBEDDED)
        {
            // load the font file
            if (is_type1()) {
                status = load_type1_font( g8_font );
            } else if (is_truetype()) {
                status = load_truetype_font( g8_font );
            } else if (is_cid0()) {
                status = load_cid_font( cid_font );
            } else if (is_cid2()) {
                status = load_cidtype2_font( cid_font );
            }
        } else {
            status = true;
        }
#if 0
        std::cerr << " Loading " << name << ", type: "
                  << util::debug::get_font_type_str(type);
        if (encoding) std::cerr << ", " << *encoding;
        std::cerr << std::endl;
#endif
        return status;
    }


    //
    // set up a FT font face
    bool FontSource::load_face()
    {
        if (FT_New_Memory_Face(ft_lib, reinterpret_cast<const FT_Byte *>(font_blob.c_str()), font_blob.length(),
                               face_index, &ft_face) == 0) {
            return (FT_Set_Char_Size( ft_face, 0, 16*64, 300, 300 ) == 0);
        }
        return false;
    }


    //
    // type1 fonts carry an encoding table of 256 entities
    bool FontSource::load_type1_font(Gfx8BitFont* gfx_font)
    {
        if (!gfx_font || !load_face()) {
            return false;
        }

        if (encoding && encoding->has_map()) {
            code_to_gid = new CodeToGIDMap(256);
            for (uint_fast16_t ii = 0; ii < 256; ++ii) {
                if (encoding->has_entity(ii)) {
                    code_to_gid->set_index(ii, FT_Get_Name_Index(ft_face, const_cast<FT_String *>(encoding->entity(ii))));
                }
            }
            code_to_gid->finalize();
        }

        return true;
    }

    //
    // true type fonts - use FoFi (yuk) to extract code 2 GID map
    bool FontSource::load_truetype_font(Gfx8BitFont* gfx_font)
    {
        if (!gfx_font || !load_face()) {
            return false;
        }

        FoFiTrueType *ff = FoFiTrueType::make(const_cast<char*>(font_blob.c_str()), font_blob.length());

        if (ff) {
            int* c2gmap = gfx_font->getCodeToGIDMap(ff);

            if (c2gmap) {
                // if we're substituting for a non-TrueType font, we need to mark
                // all notdef codes as "do not draw" (rather than drawing TrueType
                // notdef glyphs)
                if (gfx_font->getType() != fontTrueType && gfx_font->getType() != fontTrueTypeOT) {
                    code_to_gid = new CodeToGIDMap(256);
                }
                else {
                    code_to_gid = new CodeToGIDMap(256, c2gmap);
                }
                code_to_gid->finalize();

                gfree(c2gmap);
            }
            delete ff;
        }

        return true;
    }

    //
    // CID type 0 fonts - only open-type carries a code 2 GID map (?!)
    bool FontSource::load_cid_font(GfxCIDFont* cid_font)
    {
        if (!cid_font || !load_face()) {
            return false;
        }

        if (type == FONT_TYPE_CIDTYPE0COT) {
            if (cid_font->getCIDToGID()) {
                code_to_gid = new CodeToGIDMap(cid_font->getCIDToGIDLen(), cid_font->getCIDToGID());
                code_to_gid->finalize();
            }
        }

        return true;
    }

    //
    // CID type 2 (aka CFF)
    bool FontSource::load_cidtype2_font(GfxCIDFont* cid_font)
    {
        if (!cid_font || !load_face()) {
            return false;
        }

        if (cid_font->getCIDToGID()) {
            code_to_gid = new CodeToGIDMap(cid_font->getCIDToGIDLen(), cid_font->getCIDToGID());
        } else {
            FoFiTrueType* ff = FoFiTrueType::make((char*) font_blob.c_str(), font_blob.length());

            if (ff) {
                int n;
                int* codeToGIDMap = cid_font->getCodeToGIDMap(ff, &n);

                if (codeToGIDMap) {
                    code_to_gid = new CodeToGIDMap(n, codeToGIDMap);
                    gfree(codeToGIDMap);
                }

                delete ff;
            }
        }

        if (code_to_gid) {
            code_to_gid->finalize();
        }
        return true;
    }


    // =============================================================================
    // glyph loading via FreeType
    //
    struct FEPathBuilder
    {
        FEPathBuilder(PdfPath& p, double txt_scale = 1.0) :
            path(p), textscale(txt_scale), needs_close(false)
        { }

        PdfPath& path;
        double textscale;
        bool needs_close;
    };

    static int glyph_path_move_to(const FT_Vector *pt, void *arg)
    {
        FEPathBuilder *pb = static_cast<FEPathBuilder *>(arg);

        if (pb->needs_close) {
            pb->path.close();
            pb->needs_close = false;
        }
        double s = pb->textscale / SCALING_BASE;
        pb->path.move_to(Coord(pt->x * s, pt->y * s));
        return 0;
    }

    static int glyph_path_line_to(const FT_Vector *pt, void *arg)
    {
        FEPathBuilder *pb = static_cast<FEPathBuilder *>(arg);

        double s = pb->textscale / SCALING_BASE;
        pb->path.line_to(Coord(pt->x * s, pt->y * s));
        pb->needs_close = true;
        return 0;
    }

    static int glyph_path_conic_to(const FT_Vector *ctrl, const FT_Vector *pt,
                                   void *arg)
    {
        FEPathBuilder *pb = static_cast<FEPathBuilder *>(arg);
        Coord p0;

        if (!pb->path.get_cur_pt(p0)) {
            return 0;
        }

        double s = pb->textscale / SCALING_BASE;
        Coord pc( ctrl->x * s, ctrl->y * s );
        Coord p3( pt->x * s, pt->y * s );
        Coord p1( (1.0 / 3.0) * (p0.x + 2 * pc.x), (1.0 / 3.0) * (p0.y + 2 * pc.y) );
        Coord p2( (1.0 / 3.0) * (2 * pc.x + p3.x), (1.0 / 3.0) * (2 * pc.y + p3.y) );

        pb->path.curve_to(p1, p2, p3);
        pb->needs_close = true;
        return 0;
    }

    static int glyph_path_cubic_to(const FT_Vector *ctrl1, const FT_Vector *ctrl2,
                                   const FT_Vector *pt, void *arg)
    {
        FEPathBuilder *pb = static_cast<FEPathBuilder *>(arg);

        double s = pb->textscale / SCALING_BASE;
        pb->path.curve_to(Coord(ctrl1->x * s, ctrl1->y * s),
                          Coord(ctrl2->x * s, ctrl2->y * s),
                          Coord(pt->x * s, pt->y * s));
        pb->needs_close = true;
        return 0;
    }


    //
    // use FreeType to look up the character's glyph; then decompose
    // it into a PdfPath sequence of commands
    bool FontSource::get_glyph_path(CharCode code, PdfPath& path, const PdfTM* tm) const
    {
        static FT_Outline_Funcs outlineFuncs = {
            &glyph_path_move_to,
            &glyph_path_line_to,
            &glyph_path_conic_to,
            &glyph_path_cubic_to,
            0, 0
        };

        FT_UInt gid;
        FT_Glyph glyph;

        if (!ft_face) {
            return false;
        }

        if (tm) {
#if 0
            // set the transform if a TM matrix is given
            FT_Matrix txt_matrix = { tm->a(), tm->c(), tm->b(), tm->d() };
            FT_Vector txt_vector = { tm->e(), tm->f() };
            FT_Set_Transform(ft_face, &txt_matrix, &txt_vector);
#endif
        }

        FT_GlyphSlot slot = ft_face->glyph;
        if (code_to_gid && (code < code_to_gid->length())) {
            gid = code_to_gid->map(code);
        } else {
            gid = static_cast<FT_UInt>(code);
        }
        if (FT_Load_Glyph(ft_face, gid, FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH | FT_LOAD_NO_SCALE/*| FT_LOAD_NO_BITMAP*/)) {
            std::stringstream err;
            err << __FUNCTION__ << " - failed to load glyph for font " << name << ", code " << code;
            et.log_warn( ErrorTracker::ERROR_FE_FONT_FT, MODULE, err.str() );
            return false;
        }
        if (FT_Get_Glyph(slot, &glyph)) {
            std::stringstream err;
            err << __FUNCTION__ << " - failed to get glyph for font " << name << ", code " << code;
            et.log_error( ErrorTracker::ERROR_FE_FONT_FT, MODULE, err.str() );
            return false;
        }
        FT_OutlineGlyph o_glyph = reinterpret_cast<FT_OutlineGlyph>(glyph);
        if (FT_Outline_Check(&(o_glyph->outline))) {
            std::stringstream err;
            err << __FUNCTION__ << " - FT_Outline_Glyph failed";
            et.log_error( ErrorTracker::ERROR_FE_FONT_FT, MODULE, err.str() );
            return false;
        }

        FEPathBuilder path_builder(path);
        FT_Outline_Decompose(&(o_glyph->outline), &outlineFuncs, &path_builder);
        if (path_builder.needs_close) {
            path_builder.path.close();
        }
        FT_Done_Glyph(glyph);
        return true;
    }

} // namespace
