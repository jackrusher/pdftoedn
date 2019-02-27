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

#include <ostream>
#include <list>

#include "util.h"
#include "base_types.h"
#include "pdf_links.h"

namespace pdftoedn
{
    // ---------------------------------------------------------
    // storage for outline data collected by Poppler's PDFDoc (aka
    // PDFReader) as defined in pdf_reader.cc
    //
    class PdfOutline : public gemable {
    public:

        PdfOutline() =default;
        PdfOutline(const PdfOutline&) = delete;
        PdfOutline(PdfOutline&) = delete;
        PdfOutline& operator=(PdfOutline&) = delete;
        PdfOutline& operator=(const PdfOutline&) = delete;
        virtual ~PdfOutline() { util::delete_ptr_container_elems(entries); }

        //
        // internal struct to track an outline entry in the tree
        class Entry : public gemable
        {
        public:
            Entry(const std::wstring& entry_title) :
                title(entry_title), page(0) { }
            virtual ~Entry() { util::delete_ptr_container_elems(entries); }

            void set_page(uintmax_t p) { page = p; }
            void set_dest(const std::string& d) { dest = d; }
            pdftoedn::PdfLink& link() { return link_meta; }
            const pdftoedn::PdfLink& link() const { return link_meta; }

            std::list<Entry *>& get_entry_list() { return entries; }

            virtual std::ostream& to_edn(std::ostream& o) const;

        private:
            std::wstring title;
            uintmax_t page;
            std::string dest; // link destination (file, uri)
            pdftoedn::PdfLink link_meta;
            std::list<Entry *> entries;

            util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;
        };

        // accessor to be used by the PdfReader - unfortunately,
        // poppler's outline parser doesn't make it easy to add our
        // own data setters
        std::list<PdfOutline::Entry *>& get_entry_list() { return entries; }
        bool has_content() const { return !entries.empty(); }

        virtual std::ostream& to_edn(std::ostream& o) const;

    private:
        std::list<Entry *> entries;
    };

} // namespace
