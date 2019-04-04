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

#include <sstream>
#include <string>
#include <iterator>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/locale.hpp>
#include <poppler/Link.h>
#include <poppler/CharTypes.h>

#ifdef HAVE_LIBOPENSSL
#include <iomanip>
#include <openssl/md5.h>
#else
#include "external/bzflag_md5.h"
#endif

#include "graphics.h"
#include "pdf_links.h"
#include "util.h"



namespace pdftoedn
{
    namespace util
    {
        // -------------------------------------------------------------------------------
        // to UTF, etc.
        std::string md5(const std::string& blob)
        {
            // ran into problems with openssl lib & headers across
            // Ubuntu and OS X so using local code if openssl is not
            // easily found
#ifdef HAVE_LIBOPENSSL
            // use openssl to compute
            uint8_t md5_result[MD5_DIGEST_LENGTH];

            MD5(reinterpret_cast<const unsigned char*>(blob.c_str()), blob.length(), md5_result);

            std::stringstream md5_str;
            for (uintmax_t i = 0; i < MD5_DIGEST_LENGTH; i++)
                md5_str << std::hex << std::setw(2) << std::setfill('0') << (int) md5_result[i];

            return md5_str.str();
#else
            return bzflag::md5(blob);
#endif
        }

        // -------------------------------------------------------------------------------
        // to UTF, etc.
        std::string string_to_utf(const std::string& str)
        {
            return boost::locale::conv::utf_to_utf<char>(str);
        }
        std::string string_to_utf(const std::wstring& str)
        {
            return boost::locale::conv::utf_to_utf<char>(str);
        }

        std::wstring string_to_iso8859(const char* str)
        {
            return boost::locale::conv::to_utf<wchar_t>(str, "ISO-8859-1");
        }

        // -------------------------------------------------------------------------------
        // convert a wstring to a ruby string
        //
        std::string wstring_to_utfstring(std::wstring const& w_str)
        {
            return boost::locale::conv::utf_to_utf<char>(w_str);
        }

        //
        // convert poppler's Unicode to wstring
        std::wstring unicode_to_wstring(const Unicode* const u, int len)
        {
            std::wstring str;
            wchar_t c[2] = { 0, 0 };

            if (u) {
                for (int i = 0; i < len; i++) {
                    c[0] = u[i];
                    str += c;
                }
            }

            return str;
        }

        //
        // translate poppler's GfxBlendMode to the SVG equivalent, if
        // applicable
        uint8_t pdf_to_svg_blend_mode(GfxBlendMode mode)
        {
            // the poppler enums don't correspond value wise to
            // our SVG enums
            switch (mode)
            {
              case gfxBlendMultiply:
                  return GfxAttribs::MULTIPLY_BLEND;
                  break;
              case gfxBlendScreen:
                  return GfxAttribs::SCREEN_BLEND;
                  break;
              case gfxBlendDarken:
                  return GfxAttribs::DARKEN_BLEND;
                  break;
              case gfxBlendLighten:
                  return GfxAttribs::LIGHTEN_BLEND;
                  break;
              default:
                  // don't set if gfxBlendNormal or if any other types as
                  // they are not suported by SVG
                  break;
            }

            return GfxAttribs::NORMAL_BLEND;
        }


        //
        // translate a stream kind to our internal types
        StreamProps::stream_type_e poppler_stream_type_to_edn(StreamKind k)
        {
            switch (k)
            {
              case strFile:       return StreamProps::STREAM_FILE;        break;
              case strCachedFile: return StreamProps::STREAM_CACHED_FILE; break;
              case strASCIIHex:   return StreamProps::STREAM_ASCII_HEX;   break;
              case strASCII85:    return StreamProps::STREAM_ASCII85;     break;
              case strLZW:        return StreamProps::STREAM_LZW;         break;
              case strRunLength:  return StreamProps::STREAM_RUN_LENGTH;  break;
              case strCCITTFax:   return StreamProps::STREAM_CCITT_FAX;   break;
              case strDCT:        return StreamProps::STREAM_DCT;         break;
              case strFlate:      return StreamProps::STREAM_FLATE;       break;
              case strJBIG2:      return StreamProps::STREAM_JBIG2;       break;
              case strJPX:        return StreamProps::STREAM_JPX;         break;
              default: break;
            }
            return StreamProps::STREAM_TYPE_COUNT;
        }


        //
        // sets PDF meta values from a poppler LinkDest type
        void copy_link_meta(PdfLink& link, const LinkDest& ldest, double page_height)
        {
#if 0
            std::cerr << "top: " << ldest.getTop()
                 << ", left: " << ldest.getLeft()
                 << ", bot: "  << ldest.getBottom()
                 << ", right: " << ldest.getRight()
                 << ", zoom: " << ldest.getZoom()
                 << ", getChangeTop: " << ldest.getChangeTop()
                 << ", getChangeLef: " << ldest.getChangeLeft()
                 << ", getChangeZoom: " << ldest.getChangeZoom() << std::endl;
#endif

            switch (ldest.getKind()) {
                // TODO: figure out the meaning of each
              case destXYZ:
              case destFit:
              case destFitH:
              case destFitV:
              case destFitR:
              case destFitB:
              case destFitBH:
              case destFitBV:

                  // top, left, bottom, right
                  if (ldest.getChangeTop() ) {
                      link.set_top_left( page_height - ldest.getTop(),
                                         ldest.getLeft() );
                  } else {
                      link.set_bottom_right( page_height - ldest.getBottom(),
                                             ldest.getRight() );
                  }

                  // zoom
                  if (ldest.getChangeZoom()) {
                      link.set_zoom( ldest.getZoom() );
                  }
                  break;

              default:
                  std::stringstream err;
                  err << __FUNCTION__ << " - Unhandled link type (" << ldest.getKind() << ")";
                  et.log_error( ErrorTracker::ERROR_PAGE_DATA, MODULE, err.str() );
                  break;
            } // switch
        }

        //
        // translate poppler errors to our own
        bool poppler_error_to_edn(ErrorCategory category, const std::string& poppler_msg, int pos,
                                  ErrorTracker::error_type& err, ErrorTracker::error::level& level)
        {
            ErrorTracker::error_type e;

            switch (category)
            {
              case errSyntaxWarning:   e = ErrorTracker::ERROR_SYNTAX_WARNING; break;
              case errSyntaxError:     e = ErrorTracker::ERROR_SYNTAX_ERROR;   break;
              case errConfig:          e = ErrorTracker::ERROR_CONFIG;         break;
              case errCommandLine:     e = ErrorTracker::ERROR_COMMAND_LINE;   break;
              case errIO:              e = ErrorTracker::ERROR_IO;             break;
              case errNotAllowed:      e = ErrorTracker::ERROR_NOT_ALLOWED;    break;
              case errUnimplemented:   e = ErrorTracker::ERROR_UNIMPLEMENTED;  break;
              case errInternal:        e = ErrorTracker::ERROR_INTERNAL;       break;

              default:
                  std::stringstream err;
                  err << __FUNCTION__ << " - Unhandled poppler ErrorCategory: (" << category << ") for error '"
                      << poppler_msg << "'";
                  et.log_error( ErrorTracker::ERROR_PAGE_DATA, MODULE, err.str() );
                  return false;
            }

            // set the return values - used to include position
            // information but we don't care about this too much so,
            // in order to reduce verbosity, I'm not doing anything
            // with it for now
            err = e;
            level = (e == ErrorTracker::ERROR_SYNTAX_WARNING) ? ErrorTracker::error::L_WARNING : ErrorTracker::error::L_ERROR;
            return true;
        }

        //
        // convert poppler's font type to our own
        FontSource::FontType poppler_gfx_font_type_to_edn(GfxFontType t)
        {
            FontSource::FontType font_type;

            switch (t)
            {
              case fontType1:       font_type = FontSource::FONT_TYPE_TYPE1;       break;
              case fontType1C:      font_type = FontSource::FONT_TYPE_TYPE1C;      break;
              case fontType1COT:    font_type = FontSource::FONT_TYPE_TYPE1COT;    break;
              case fontType3:       font_type = FontSource::FONT_TYPE_TYPE3;       break;
              case fontTrueType:    font_type = FontSource::FONT_TYPE_TRUETYPE;    break;
              case fontTrueTypeOT:  font_type = FontSource::FONT_TYPE_TRUETYPEOT;  break;

              case fontCIDType0:    font_type = FontSource::FONT_TYPE_CIDTYPE0;    break;
              case fontCIDType0C:   font_type = FontSource::FONT_TYPE_CIDTYPE0C;   break;
              case fontCIDType0COT: font_type = FontSource::FONT_TYPE_CIDTYPE0COT; break;
              case fontCIDType2:    font_type = FontSource::FONT_TYPE_CIDTYPE2;    break;
              case fontCIDType2OT:  font_type = FontSource::FONT_TYPE_CIDTYPE2OT;  break;

              case fontUnknownType:
              default:
                  font_type = FontSource::FONT_TYPE_UNKNOWN;
                  break;
            }

            return font_type;
        }

        std::string expand_environment_variables( const std::string& s ) {
            std::size_t pos = s.find( "${");
            if ( pos == std::string::npos ) {
                return s;
            }

            std::string pre  = s.substr( 0, pos );
            std::string post = s.substr( pos + 2 );

            pos = post.find( '}' );
            if ( pos == std::string::npos ) {
                return s;
            }

            std::string variable = post.substr( 0, pos );
            std::string value;

            post = post.substr( pos + 1 );

            if ( getenv( variable.c_str() ) != nullptr ) {
                value = std::string( getenv( variable.c_str() ) );
            }

            return expand_environment_variables( pre + value + post );
        }

    } // util
} // namespace
