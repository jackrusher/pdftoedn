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

#include <cstdlib>
#include <string>
#include <sstream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/version.hpp>
#include <png.h>
#include <leptonica/allheaders.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <poppler/cpp/poppler-version.h>
#include <rapidjson/rapidjson.h>

#ifdef HAVE_LIBOPENSSL
#include <openssl/crypto.h>
#include <boost/regex.hpp>
#endif

#include "util_versions.h"
#include "util_edn.h"

namespace pdftoedn {
    namespace util {
        namespace version {

            const pdftoedn::Symbol SYMBOL_APP          = "pdftoedn";
            const pdftoedn::Symbol SYMBOL_POPPLER      = "poppler";
            const pdftoedn::Symbol SYMBOL_LIBPNG       = "libpng";
            const pdftoedn::Symbol SYMBOL_BOOST        = "boost";
            const pdftoedn::Symbol SYMBOL_FREETYPE     = "freetype";
            const pdftoedn::Symbol SYMBOL_LEPTONICA    = "leptonica";
            const pdftoedn::Symbol SYMBOL_RAPIDJSON    = "rapidjson";

            static std::string boost() {
                std::string b_v = BOOST_LIB_VERSION;
                int underscore = b_v.find('_');
                b_v.replace(underscore, 1, ".");
                return b_v;
            };

            static std::string leptonica() {
                std::string l_v;

                { // leptonica requires we free the version string
                    const char* v = getLeptonicaVersion();
                    l_v = v;
                    free((void*) v);
                }

                return l_v.substr( l_v.find('-') + 1 );
            };

            static std::string freetype() {
                std::stringstream ver;
                FT_Library ft_lib;
                FT_Int ft_maj, ft_min, ft_patch;

                if (FT_Init_FreeType(&ft_lib) == 0) {
                    FT_Library_Version(ft_lib, &ft_maj, &ft_min, &ft_patch);
                    ver << ft_maj << "." << ft_min << "." <<ft_patch;
                    FT_Done_FreeType(ft_lib);
                }
                return ver.str();
            }

#ifdef HAVE_LIBOPENSSL
            static const std::string openssl() {
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
 #define OPENSSL_VERSION_FN  OpenSSL_version(0)
#else
 #define OPENSSL_VERSION_FN  SSLeay_version(0)
#endif
                static const char* pattern = "[[:digit:]]+\\.[[:digit:]]+\\.[[:alnum:]]+";
                std::string openssl_version_str;
                boost::cmatch match;

                try
                {
                    // using boost as regex support is borked before g++ 4.9
                    if (boost::regex_search(OPENSSL_VERSION_FN, match, boost::regex(pattern))) {
                        openssl_version_str = match[0];
                    }
                } catch (const std::exception& e) {
                    openssl_version_str = OPENSSL_VERSION_FN;
                }
                return openssl_version_str;
            };
#endif

            // get versions for all libs - will include these in meta
            // output
            static const std::string PDFTOEDN_POPPLER_VERSION   = poppler::version_string();
            static const std::string PDFTOEDN_LIBPNG_VERSION    = png_libpng_ver;
            static const std::string PDFTOEDN_BOOST_VERSION     = boost();
            static const std::string PDFTOEDN_LEPTONICA_VERSION = leptonica();
            static const std::string PDFTOEDN_FREETYPE_VERSION  = freetype();
            static const std::string PDFTOEDN_RAPIDJSON_VERSION = RAPIDJSON_VERSION_STRING;

            //
            // return lib version numbers
            std::string info()
            {
                std::stringstream ver;
                ver << " poppler "   << PDFTOEDN_POPPLER_VERSION << std::endl
                    << " libpng "    << PDFTOEDN_LIBPNG_VERSION << std::endl
                    << " boost "     << PDFTOEDN_BOOST_VERSION << std::endl
                    << " freetype "  << PDFTOEDN_FREETYPE_VERSION << std::endl
                    << " leptonica " << PDFTOEDN_LEPTONICA_VERSION << std::endl
                    << " rapidjson " << PDFTOEDN_RAPIDJSON_VERSION << std::endl;
#ifdef HAVE_LIBOPENSSL
                ver << " openssl "   << openssl() << std::endl;
#endif
                return ver.str();
            }

            // version EDN hash
            util::edn::Hash& libs(util::edn::Hash& version_h)
            {
                version_h.reserve(7);
                version_h.push( SYMBOL_APP      , PDFTOEDN_VERSION );
                version_h.push( SYMBOL_POPPLER  , PDFTOEDN_POPPLER_VERSION );
                version_h.push( SYMBOL_LIBPNG   , PDFTOEDN_LIBPNG_VERSION );
                version_h.push( SYMBOL_BOOST    , PDFTOEDN_BOOST_VERSION );
                version_h.push( SYMBOL_FREETYPE , PDFTOEDN_FREETYPE_VERSION );
                version_h.push( SYMBOL_LEPTONICA, PDFTOEDN_LEPTONICA_VERSION );
                version_h.push( SYMBOL_RAPIDJSON, PDFTOEDN_RAPIDJSON_VERSION );
                return version_h;
            }
        } // version
    } // util
} // namespace
