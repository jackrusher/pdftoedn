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

#include "pdf_links.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_TOP             = "top";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_BOTTOM          = "bottom";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_LEFT            = "left";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_RIGHT           = "right";
    const pdftoedn::Symbol PdfLink::SYMBOL_ZOOM                = "zoom";

    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_TYPE           = "type";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_EFFECT         = "effect";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_EFFECTS[]      = { "none", "invert", "outline", "push" };
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_ACTION_TYPE    = "action";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_ACTION_TYPES[] = { "goto", "gotor", "uri", "launch" };

    const pdftoedn::Symbol PdfAnnotLinkDest::SYMBOL_DEST       = "dest";
    const pdftoedn::Symbol PdfAnnotLinkGoto::SYMBOL_PAGE       = "page";


    // =============================================
    // links found in outlines
    //

    //
    // TL mode
    void PdfLink::set_top_left(double t, double l)
    {
        pos.y = t;
        pos.x = l;
        orientation = TOP_LEFT;
    }

    //
    // BR mode
    void PdfLink::set_bottom_right(double b, double r)
    {
        pos.x = r;
        pos.y = b;
        orientation = BOTTOM_RIGHT;
    }

    //
    // output the link as EDN
    util::edn::Hash& PdfLink::to_edn_hash(util::edn::Hash& link_h) const
    {
        //
        // only indicate any values set
        if (orientation == TOP_LEFT) {
            if (pos.y > 0) {
                link_h.push( SYMBOL_POS_TOP, pos.y );
            }

            if (pos.x > 0) {
                link_h.push( SYMBOL_POS_LEFT, pos.x );
            }
        } else {
            if (pos.y > 0) {
                link_h.push( SYMBOL_POS_BOTTOM, pos.y );
            }
            if (pos.x > 0) {
                link_h.push( SYMBOL_POS_RIGHT, pos.x );
            }
        }

        // and zoom
        if (zoom != -1) {
            link_h.push( SYMBOL_ZOOM, zoom );
        }
        return link_h;
    }

    std::ostream& PdfLink::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(3);
        o << to_edn_hash(link_h);
        return o;
    }


    // =============================================
    // annotation links
    //
    util::edn::Hash& PdfAnnotLink::to_edn_hash(util::edn::Hash& link_h) const
    {
        PdfLink::to_edn_hash(link_h);

        link_h.push( SYMBOL_TYPE, SYMBOL_ACTION_TYPES[ type ] );
        if (effect != EFFECT_NONE) {
            link_h.push( SYMBOL_EFFECT, SYMBOL_EFFECTS[ effect ] );
        }
        link_h.push( BoundingBox::SYMBOL, bbox );
        return link_h;
    }

    std::ostream& PdfAnnotLink::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(6);
        o << to_edn_hash(link_h);
        return o;
    }


    //
    // rubify link dest
    std::ostream& PdfAnnotLinkDest::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(7);
        o << to_edn_hash(link_h);
        return o;
    }

    util::edn::Hash& PdfAnnotLinkDest::to_edn_hash(util::edn::Hash& link_h) const
    {
        PdfAnnotLink::to_edn_hash(link_h);
        if (!dest.empty()) {
            link_h.push( SYMBOL_DEST, util::wstring_to_utfstring(util::string_to_iso8859(dest.c_str())) );
        }
        return link_h;
    }

    //
    // link goto
    std::ostream& PdfAnnotLinkGoto::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(7);
        PdfAnnotLinkDest::to_edn_hash(link_h);

        if (page != -1) {
            link_h.push( SYMBOL_PAGE, page );
        }
        o << link_h;
        return o;
    }

} // namespace
