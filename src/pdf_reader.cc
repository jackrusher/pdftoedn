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

#include <list>

#include <poppler/goo/GooList.h>
#include <poppler/Outline.h>
#include <poppler/Link.h>
#include <poppler/ErrorCodes.h>

#include "util.h"
#include "util_debug.h"
#include "util_edn.h"
#include "util_versions.h"
#include "pdf_reader.h"
#include "pdf_output_dev.h"
#include "link_output_dev.h"
#include "font_engine_dev.h"
#include "pdf_doc_outline.h"
#include "doc_page.h"
#include "runtime_options.h"

namespace pdftoedn
{
    static const pdftoedn::Symbol SYMBOL_PDF_FILENAME       = "filename";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_OK         = "is_ok";
    static const pdftoedn::Symbol SYMBOL_PDF_MAJ_VER        = "pdf_ver_major";
    static const pdftoedn::Symbol SYMBOL_PDF_MIN_VER        = "pdf_ver_minor";
    static const pdftoedn::Symbol SYMBOL_PDF_NUM_PAGES      = "num_pages";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_FONTS      = "doc_fonts";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_FONT_SIZES = "font_size_list";

    static const pdftoedn::Symbol SYMBOL_PDF_OUTLINE        = "outline";

    static const pdftoedn::Symbol SYMBOL_FONT_ENG_OK        = "font_engine_ok";
    static const pdftoedn::Symbol SYMBOL_FONT_ENG_FONT_WARN = "found_font_warnings";

    static const pdftoedn::Symbol SYMBOL_VERSIONS           = "versions";

    const double PDFReader::DPI_72 = 72.0;

    // helper function that returns a GooString for the password if
    // set. Used by the PDFReader constructor below
    static inline GooString* get_pdf_password(const std::string& passwd)
    {
        if (passwd.empty()) {
            // don't allocate anything
            return nullptr;
        }
        return new GooString(passwd.c_str());
    }

    //
    // opens the document and preps things for processing. throws if
    // font engine fails to init freetype (from FE constructor) or if
    // poppler fails to open the file
    PDFReader::PDFReader() :
        PDFDoc(new GooString(pdftoedn::options.pdf_filename().c_str()),
               get_pdf_password(pdftoedn::options.pdf_owner_password()),
               get_pdf_password(pdftoedn::options.pdf_user_password())),
        font_engine(getXRef()),
        eng_odev(nullptr),
        use_page_media_box(true)
    {
        if (!isOk()) {
            std::stringstream err;
            err << "Document open error: "
               << util::debug::get_poppler_doc_error_str(getErrorCode());
            throw invalid_file(err.str());
        }

        // document is open and basic meta has been read. Before
        // trying to do anything else, if a page number was given,
        // check it is within range
        if (pdftoedn::options.page_number() >= 0 &&
            pdftoedn::options.page_number() >= getNumPages()) {
            std::stringstream err;
            err << "Error: requested page number " << pdftoedn::options.page_number()
                << " is not valid (document has "
                << getNumPages() << " page";
            if (getNumPages() > 1) {
                err << "s";
            }
            err << " and value must be 0-indexed)";
            throw init_error(err.str());
        }

        // TESLA-6245: Mike P requested a way to extract only links
        // from a doc. To do this, we use a different type of
        // OutputDev that ignores everything but links
        if (pdftoedn::options.link_output_only()) {
            eng_odev = new pdftoedn::LinkOutputDev(getCatalog());
        }
        else {
#ifdef FE_PREPROCESS_TEXT
            // pre-process the doc to extract fonts first. Needed
            // if additional font data needs to be included in the
            // meta before pages are parsed
            if (pdftoedn::options.force_pre_process_fonts()) {
                pre_process_fonts();
            }
#endif
            eng_odev = new pdftoedn::OutputDev(getCatalog(), font_engine);

            // use page crop box if requested (page media box is the default)
            if (pdftoedn::options.use_page_crop_box()) {
                use_page_media_box = false;
            }

            // process outline, if needed
            if (!(pdftoedn::options.omit_outline())) {
                process_outline(outline_output);
            }
        }
    }

#ifdef FE_PREPROCESS_TEXT
    //
    // use a custom OutputDev to only read fonts from the doc
    bool PDFReader::pre_process_fonts()
    {
        // don't run through pages if poppler wasn't able to open the doc
        if (!isOk()) {
            return false;
        }

        // pre-process doc for font data
        FontEngDev fe_dev(font_engine);
        uint32_t num_pages = getNumPages();

        uintmax_t first, last;

        if (pdftoedn::options.page_number() != -1) {
            first = pdftoedn::options.page_number();
            last = first + 1;
        } else {
            first = 1;
            last = num_pages;
        }

        for (uintmax_t page = first; page <= last; page++)
        {
            // process the PDF info on this page
            process_page(&fe_dev, page);
        }

#if 0
        font_engine.dump_font_info();
#endif
        return true;
    }
#endif

    //
    // document meta output in EDN format
    std::ostream& PDFReader::output_meta(std::ostream& o) {
        util::edn::Hash meta_h(14);

        meta_h.push( util::version::SYMBOL_DATA_FORMAT_VERSION, util::version::data_format_version() );
        meta_h.push( SYMBOL_PDF_FILENAME                      , pdftoedn::options.pdf_filename() );
        meta_h.push( SYMBOL_PDF_DOC_OK                        , true );
        meta_h.push( SYMBOL_FONT_ENG_OK                       , true );

        if (font_engine.found_font_warnings()) {
            meta_h.push( SYMBOL_FONT_ENG_FONT_WARN            , true );
        }

        meta_h.push( SYMBOL_PDF_MAJ_VER                   , static_cast<uint8_t>(getPDFMajorVersion()) );
        meta_h.push( SYMBOL_PDF_MIN_VER                   , static_cast<uint8_t>(getPDFMinorVersion()) );

        meta_h.push( SYMBOL_PDF_NUM_PAGES                 , static_cast<uintmax_t>(getNumPages()) );

        // outline - empty hash if none
        meta_h.push( SYMBOL_PDF_OUTLINE                   , &outline_output );

        // save the sorted list of font sizes read in the
        // document - to be used in case we need to generate
        // an outline by examining page content
        const std::set<double>& font_size_list = font_engine.get_font_size_list();
        if (!font_size_list.empty()) {
            util::edn::Vector font_size_a(font_size_list.size());
            // use reverse iterator to get list backward
            std::for_each( font_size_list.rbegin(), font_size_list.rend(),
                           [&](const double& d) { font_size_a.push(d); }
                           );
            meta_h.push( SYMBOL_PDF_DOC_FONT_SIZES        , font_size_a );
        }

        // include document fonts in meta if requested
        if (pdftoedn::options.include_debug_info())
        {
            // document font list
            const FontList& fonts = font_engine.get_font_list();
            util::edn::Vector font_a(fonts.size());

            uintmax_t idx = 0;
            for (const FontListEntry& entry_pair : fonts) {
                util::edn::Hash font_h(2);

                entry_pair.second->to_edn_hash(font_h);
                font_h.push( PdfPage::SYMBOL_FONT_IDX, idx++ );

                font_a.push(font_h);
            }
            meta_h.push( SYMBOL_PDF_DOC_FONTS, font_a );
        }

        util::edn::Hash version_h;
        meta_h.push( SYMBOL_VERSIONS                          , util::version::libs(version_h));

        // if we caught errors, include them
        if (et.errors_reported()) {
            meta_h.push( ErrorTracker::SYMBOL_ERRORS          , &et );
        }
        o << meta_h;
        return o;
    }


    //
    // calls poppler's display page with the given output device and
    // page number
    void PDFReader::process_page(::OutputDev *dev, uintmax_t page_num) {
        // clear the current list of errors to only capture what is
        // generated by the page
        et.flush_errors();

        displayPage(dev, page_num, DPI_72, DPI_72, 0, false, true, false);
    }


    //
    // extract the document page data
    std::ostream& PDFReader::output_page(uintmax_t page_num, std::ostream& o)
    {
        uintmax_t num_pages = getNumPages();

        // poppler is 1-based so adjust
        page_num += 1;

        if (page_num <= num_pages) {

            // process the PDF info on this page
            process_page(eng_odev, page_num);

            const PdfPage* page = eng_odev->page_data();

            if (page) {
                o << *page;
            }
        }

        return o;
    }

    std::ostream& PDFReader::process(std::ostream& o)
    {
        // return a hash with the data in the format
        // { :meta { <meta> }, :pages [ {<page1>} {<page2>} ... {<pageN>} ] }
        static const pdftoedn::Symbol Meta("meta");
        static const pdftoedn::Symbol Pages("pages");

        // but dont store it in a hash so we write a page at a time
        o << "{" << Meta << " ";
        output_meta(o);
        o << ", " << Pages << " [";

        uintmax_t start_page, end_page;

        if (options.page_number() < 0) {
            start_page = 0;
            end_page = getNumPages();
        } else {
            start_page = options.page_number();
            end_page = start_page + 1;
        }

        for (uintmax_t ii = start_page; ii < end_page; ++ii) {
            output_page(ii, o);
        }

        o << "]}";
        return o;
    }


    //
    // extract the outline data
    bool PDFReader::process_outline(PdfOutline& outline_output)
    {
        Outline *outline = getOutline();

        if (outline && outline->getItems()) {
            outline_level(const_cast<GooList *>(outline->getItems()), 0, outline_output.get_entry_list());
            return true;
        }
        return false;
    }


    //
    //
    // get link page number
    uintmax_t PDFReader::get_link_page_num(const LinkDest* const dest)
    {
        if (!dest)
            return 0;

        if (dest->isPageRef()) {
            Ref pageref = dest->getPageRef();
            return getCatalog()->findPage( pageref.num, pageref.gen);
        }

        return dest->getPageNum();
    }


    //
    //
    void PDFReader::outline_link_dest(const LinkDest* const dest, PdfOutline::Entry& entry)
    {
        uintmax_t page_num = get_link_page_num(dest);
        entry.set_page( page_num );
        util::copy_link_meta(entry.link(), *dest,
                             (use_page_media_box ?
                              getPageMediaHeight(page_num) :
                              getPageCropHeight(page_num)
                              )
                             );
    }


    //
    // PDF GoTo link types
    void PDFReader::outline_action_goto(const LinkGoTo* const link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {
            const LinkDest* dest = nullptr;

            if (link->getDest()) {
                dest = link->getDest()->copy();
            }
            else if (link->getNamedDest()) {
                dest = getCatalog()->findDest( link->getNamedDest() );
            }

            if (dest) {
                outline_link_dest(dest, entry);
                delete dest;
            }
        }
    }

    //
    // gotoR links pointing to other files in the FS
    void PDFReader::outline_action_goto_r(const LinkGoToR* const link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {

            // set the filename as the destination, then copy the meta info
            entry.set_dest( link->getFileName()->c_str() );

            LinkDest* dest = nullptr;
            if (link->getDest()) {
                dest = link->getDest()->copy();
            }
            else if (link->getNamedDest()) {
                dest = getCatalog()->findDest( link->getNamedDest() );
            }

            if (dest) {
                outline_link_dest(dest, entry);
                delete dest;
            }
        }
    }

    //
    // URI links pointing to URI resources
    void PDFReader::outline_action_uri(const LinkURI* const link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {
            // set the URI as the destination
            entry.set_dest( link->getURI()->c_str() );
        }
    }

    //
    //
    // process the current outline entry in the list
    void PDFReader::outline_level(const GooList* const items, int level,
                                  std::list<PdfOutline::Entry *>& entry_list)
    {
        // run through the list
        for (int i = 0; i < items->getLength(); i++)
        {
            // get the current item
            OutlineItem* item = static_cast<OutlineItem*>(items->get(i));

            if (!item) {
                continue;
            }

            // get & store the title, trimming whitespace
            std::wstring title = util::unicode_to_wstring(item->getTitle(), item->getTitleLength());
            PdfOutline::Entry* e = new PdfOutline::Entry( util::trim(title) );
            entry_list.push_back(e);

            // action shoud get LINK_GOTO
            const LinkAction* link_action = item->getAction();

            if (link_action)
            {
                // select the link type
                if (link_action->getKind() == actionGoTo) {
                    outline_action_goto( dynamic_cast<const LinkGoTo*>(link_action), *e );
                } else if (link_action->getKind() == actionGoToR) {
                    // TESLA-6168 - include GotoR link destination for Mike P.
                    outline_action_goto_r( dynamic_cast<const LinkGoToR*>(link_action), *e );
                } else if (link_action->getKind() == actionURI) {
                    // (and adding URI link destination in case)
                    outline_action_uri( dynamic_cast<const LinkURI*>(link_action), *e );
                } else {
                    std::stringstream err;
                    err << "link action kind: " << link_action->getKind();
                    et.log_warn(ErrorTracker::ERROR_UNHANDLED_LINK_ACTION, MODULE, err.str());
                }
            }

            // traverse the child nodes
            item->open();
            if (item->hasKids()) {
                outline_level(const_cast<GooList *>(item->getKids()), level + 1, e->get_entry_list());
            }
            item->close();
        }
    }

} // namespace
