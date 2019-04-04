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

#include <poppler/Error.h>
#include <poppler/GfxState.h>
#include <poppler/GfxFont.h>

namespace pdftoedn
{
    namespace util
    {
        namespace debug {
            std::string get_poppler_doc_error_str(int errCode);
            const char* get_poppler_error_str(ErrorCategory e);
            const char* get_stream_kind_str(StreamKind sk);
            const char* get_font_type_str(FontSource::FontType type);
            const char* get_font_type_str(GfxFontType type);
            const char* get_blend_mode_str(GfxBlendMode mode);
            void save_blob_to_disk(const std::ostringstream& blob, const std::string& name_pfx, int id);
            const char* get_font_file_extension(GfxFontType type);
        }

    }
}
