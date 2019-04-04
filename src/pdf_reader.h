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

//
// subclass of Poppler's PdfDoc for reading the contents of a
// PDF
//

#include <string>
#include <list>

#include <poppler/PDFDoc.h>

#include "font_engine.h"
#include "pdf_doc_outline.h"
#include "pdf_output_dev.h"

class LinkGoTo;
class LinkGoToR;
class LinkURI;
class GooList;

namespace pdftoedn
{
    //
    // Poppler PDF Doc reader
    class PDFReader : public PDFDoc
    {
    public:
        static const double DPI_72; // 72.0

        PDFReader();
        virtual ~PDFReader() { delete eng_odev; }

#ifdef FE_PREPROCESS_TEXT
        bool pre_process_fonts();
#endif
        std::ostream& process(std::ostream& o);

        friend std::ostream& operator<<(std::ostream& o, PDFReader& doc) {
            return doc.process(o);
        }

    private:
        pdftoedn::FontEngine font_engine;
        pdftoedn::EngOutputDev* eng_odev;
        pdftoedn::PdfOutline outline_output;
        bool use_page_media_box;

        bool init_font_engine();
        bool process_outline(pdftoedn::PdfOutline& outline_output);
        void outline_level(const GooList* const items, int level, std::list<PdfOutline::Entry *>& entry_list);
        void outline_action_goto(const LinkGoTo* const link, PdfOutline::Entry& entry);
        void outline_action_goto_r(const LinkGoToR* const link, PdfOutline::Entry& entry);
        void outline_action_uri(const LinkURI* const link, PdfOutline::Entry& entry);
        void outline_link_dest(const LinkDest* const dest, PdfOutline::Entry& entry);
        uintmax_t get_link_page_num(const LinkDest* const link);

        void process_page(::OutputDev* dev, uintmax_t page);

        // returns document metadata
        std::ostream& output_meta(std::ostream& o);
        std::ostream& output_page(uintmax_t page_num, std::ostream& o);
    };

} // namespace
