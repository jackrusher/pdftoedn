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

#include <sstream>
#include <fstream>
#include <string>

#include <poppler/Error.h>
#include <poppler/ErrorCodes.h>
#include <poppler/GfxState.h>
#include <poppler/GfxFont.h>

#include "util.h"
#include "util_debug.h"
#include "util_fs.h"

#define CASE_STRINGIFY(c)     case c: return #c;
#define DEFAULT_STRINGIFY(c) default: return "Unhandled case in: "#c;

namespace pdftoedn
{
    namespace util
    {
        namespace debug
        {
            // ==================================================
            // stringifiers

            //
            // error types
            std::string get_poppler_doc_error_str(int errCode)
            {
                // see poppler/ErrorCodes.h
                switch (errCode) {
                  case errNone:
                      return "no error"; break;
                  case errOpenFile:
                      return "couldn't open the PDF file"; break;
                  case errBadCatalog:
                      return "couldn't read the page catalog"; break;
                  case errDamaged:
                      return "document does not look like a valid PDF"; break;
                  case errEncrypted:
                      return "file was encrypted and password was incorrect or not supplied"; break;
                  case errHighlightFile:
                      return "nonexistent or invalid highlight file"; break;
                  case errBadPrinter:
                      return "invalid printer"; break;
                  case errPrinting:
                      return "error during printing"; break;
                  case errPermission:
                      return "PDF file doesn't allow that operation"; break;
                  case errBadPageNum:
                      return "invalid page number"; break;
                  case errFileIO:
                      return "file I/O error"; break;
                  default: break;
                }

                std::stringstream err;
                err << "Unknown error code: " << errCode;
                return err.str();
            }

            const char* get_poppler_error_str(ErrorCategory e)
            {
                switch (e) {
                    CASE_STRINGIFY(errSyntaxWarning);
                    CASE_STRINGIFY(errSyntaxError);
                    CASE_STRINGIFY(errConfig);
                    CASE_STRINGIFY(errCommandLine);
                    CASE_STRINGIFY(errIO);
                    CASE_STRINGIFY(errNotAllowed);
                    CASE_STRINGIFY(errUnimplemented);
                    CASE_STRINGIFY(errInternal);
                    DEFAULT_STRINGIFY(e);
                }
                return NULL;
            }

            //
            // StreamKind
            const char* get_stream_kind_str(StreamKind sk)
            {
                switch (sk)
                {
                    CASE_STRINGIFY(strFile);
                    CASE_STRINGIFY(strCachedFile);
                    CASE_STRINGIFY(strASCIIHex);
                    CASE_STRINGIFY(strASCII85);
                    CASE_STRINGIFY(strLZW);
                    CASE_STRINGIFY(strRunLength);
                    CASE_STRINGIFY(strCCITTFax);
                    CASE_STRINGIFY(strDCT);
                    CASE_STRINGIFY(strFlate);
                    CASE_STRINGIFY(strJBIG2);
                    CASE_STRINGIFY(strJPX);
                    CASE_STRINGIFY(strWeird);
                    DEFAULT_STRINGIFY(sk);
                }
                return NULL;
            }

            //
            // FontType and GfxFontType
            // FontType
            const char* get_font_type_str(FontSource::FontType type)
            {
                switch (type)
                {
                    CASE_STRINGIFY(FontSource::FONT_TYPE_UNKNOWN);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TYPE1);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TYPE1C);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TYPE1COT);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TYPE3);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TRUETYPE);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_TRUETYPEOT);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_CIDTYPE0);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_CIDTYPE0C);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_CIDTYPE0COT);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_CIDTYPE2);
                    CASE_STRINGIFY(FontSource::FONT_TYPE_CIDTYPE2OT);
                    DEFAULT_STRINGIFY(type);
                }
            }

            const char* get_font_type_str(GfxFontType type)
            {
                return get_font_type_str(poppler_gfx_font_type_to_edn(type));
            }

            //
            // blend mode
            const char* get_blend_mode_str(GfxBlendMode mode)
            {
                switch (mode)
                {
                    CASE_STRINGIFY(gfxBlendNormal);
                    CASE_STRINGIFY(gfxBlendMultiply);
                    CASE_STRINGIFY(gfxBlendScreen);
                    CASE_STRINGIFY(gfxBlendOverlay);
                    CASE_STRINGIFY(gfxBlendDarken);
                    CASE_STRINGIFY(gfxBlendLighten);
                    CASE_STRINGIFY(gfxBlendColorDodge);
                    CASE_STRINGIFY(gfxBlendColorBurn);
                    CASE_STRINGIFY(gfxBlendHardLight);
                    CASE_STRINGIFY(gfxBlendSoftLight);
                    CASE_STRINGIFY(gfxBlendDifference);
                    CASE_STRINGIFY(gfxBlendExclusion);
                    CASE_STRINGIFY(gfxBlendHue);
                    CASE_STRINGIFY(gfxBlendSaturation);
                    CASE_STRINGIFY(gfxBlendColor);
                    CASE_STRINGIFY(gfxBlendLuminosity);
                    DEFAULT_STRINGIFY(mode);
                }
                return NULL;
            }

            //
            // debug - dump to disk using a unique name
            void save_blob_to_disk(const std::ostringstream& blob, const std::string& name_pfx, int id)
            {
                // writes the files to disk
                std::ostringstream name;
                name << util::expand_environment_variables("${HOME}") << "/Desktop/"
                     << name_pfx << "-" << id << ".png";

                util::fs::write_image_to_disk(name.str(), blob.str());
            }


            //
            //
            const char* get_font_file_extension(GfxFontType type)
            {
                switch (type) {
                  case fontType1:        return ".pfb"; break;

                  case fontType1COT:
                  case fontType1C:       return ".cff"; break;

                  case fontCIDType0COT:
                  case fontCIDType2:
                  case fontCIDType2OT:
                  case fontCIDType0C:    return ".cid"; break;

                  case fontType3:        return ".pf3"; break;

                  case fontTrueType:
                  case fontTrueTypeOT:   return ".ttf"; break;

                  case fontCIDType0:     return ".cidtype0"; break;

                  default:
                      break;
                }
                return "";
            }

        } // namespace debug
    } // namespace util
} // namespace
