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
#include "base_types.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // base links class
    //
    class PdfLink : public gemable
    {
    public:
        PdfLink() : zoom(-1), orientation(LINK_POS_UNDEF) { }

        // accessors
        bool is_set() const { return ((orientation != LINK_POS_UNDEF) || (zoom > 0)); }

        // setters
        void set_zoom(double z) { zoom = z; }
        void set_top_left(double t, double l);
        void set_bottom_right(double b, double r);

        virtual std::ostream& to_edn(std::ostream& o) const;

        enum position_e {
            LINK_POS_UNDEF,
            TOP_LEFT,
            BOTTOM_RIGHT
        };

        static const pdftoedn::Symbol SYMBOL_LINK_IDX;
        static const pdftoedn::Symbol SYMBOL_POS_TOP;
        static const pdftoedn::Symbol SYMBOL_POS_BOTTOM;
        static const pdftoedn::Symbol SYMBOL_POS_LEFT;
        static const pdftoedn::Symbol SYMBOL_POS_RIGHT;
        static const pdftoedn::Symbol SYMBOL_ZOOM;

    private:
        Coord pos;
        double zoom;
        position_e orientation;

    protected:
        virtual util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;
    };


    // -------------------------------------------------------
    // annotation links
    //
    class PdfAnnotLink : public PdfLink
    {
    public:
        enum effect_e {
            EFFECT_NONE,
            EFFECT_INVERT,
            EFFECT_OUTLINE,
            EFFECT_PUSH
        };

        enum type_e {
            ACTION_GOTO,
            ACTION_GOTOR,
            ACTION_URI,
            ACTION_LAUNCH,
        };

        // is the given point within this link? ideally should be the
        // center of a bbox
        bool encloses(const Coord& center) const {
            return (center.x >= bbox.x1() &&
                    center.x <= bbox.x2() &&
                    center.y >= bbox.y1() &&
                    center.y <= bbox.y2());
        }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_TYPE;
        static const pdftoedn::Symbol SYMBOL_EFFECT;
        static const pdftoedn::Symbol SYMBOL_EFFECTS[];
        static const pdftoedn::Symbol SYMBOL_ACTION_TYPE;
        static const pdftoedn::Symbol SYMBOL_ACTION_TYPES[];

    protected:
        // constructor - instantiated via subclasses depending on type
        PdfAnnotLink(type_e action_type,
                     double x1, double y1, double x2, double y2,
                     uint8_t link_effect) :
            type(action_type),
            bbox(x1, y1, x2-x1, y2-y1),
            effect(link_effect)
        { }

        virtual util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;

    private:
        type_e type;
        BoundingBox bbox;
        uint8_t effect;
    };

    //
    // links with a string-type destination
    class PdfAnnotLinkDest : public PdfAnnotLink
    {
    public:
        static const pdftoedn::Symbol SYMBOL_DEST;

    protected:
        PdfAnnotLinkDest(type_e action_type,
                         double x1, double y1, double x2, double y2,
                         int effect,
                         const std::string& link_dest) :
            PdfAnnotLink(action_type, x1, y1, x2, y2, effect),
            dest(link_dest)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;
        virtual util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;

    private:
        std::string dest;
    };


    //
    // Goto links w/ page number
    class PdfAnnotLinkGoto : public PdfAnnotLinkDest
    {
    public:
        PdfAnnotLinkGoto(double x1, double y1, double x2, double y2, int effect,
                         uint32_t link_page, const std::string& dest) :
            PdfAnnotLinkDest(PdfAnnotLink::ACTION_GOTO, x1, y1, x2, y2, effect, dest),
            page(link_page)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_PAGE;

    protected:
        // for use by GotoR
        PdfAnnotLinkGoto(type_e action_type,
                         double x1, double y1, double x2, double y2, int effect,
                         uintmax_t link_page, const std::string& dest) :
            PdfAnnotLinkDest(action_type, x1, y1, x2, y2, effect, dest),
            page(link_page)
        { }

        uintmax_t page;
    };

    //
    // GotoR links - subtypes of Goto; just set the type differently
    struct PdfAnnotLinkGotoR : public PdfAnnotLinkGoto
    {
        PdfAnnotLinkGotoR(double x1, double y1, double x2, double y2, int effect,
                          uintmax_t page, const std::string& dest) :
            PdfAnnotLinkGoto(PdfAnnotLink::ACTION_GOTOR, x1, y1, x2, y2, effect, page, dest)
        { }
    };

    //
    // skeleton classes for URI and launch links (based on Dest)
    struct PdfAnnotLinkURI : public PdfAnnotLinkDest
    {
        PdfAnnotLinkURI(double x1, double y1, double x2, double y2, int effect,
                        const std::string& link_uri) :
            PdfAnnotLinkDest(PdfAnnotLink::ACTION_URI, x1, y1, x2, y2, effect, link_uri)
        { }
    };

    struct PdfAnnotLinkLaunch : public PdfAnnotLinkDest
    {
        PdfAnnotLinkLaunch(double x1, double y1, double x2, double y2, int effect,
                           const std::string& link_launch) :
            PdfAnnotLinkDest(PdfAnnotLink::ACTION_LAUNCH, x1, y1, x2, y2, effect, link_launch)
        { }
    };

} // namespace
