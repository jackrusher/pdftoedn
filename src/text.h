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

#pragma once

#include <ostream>
#include <list>
#include "util.h"
#include "base_types.h"
#include "graphics.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // common text attributes that must be tracked per char / span
    //
    struct TextAttribs
    {
        enum { YPOS_THRESHOLD = 5 }; // percentage

        TextAttribs() : font_idx(-1), font_size(0), baseline_threshold(0), invisible(false), link_idx(-1) {}

        // check if font attributes are equal
        bool operator==(const TextAttribs& a) const {
            return ( (font_idx  == a.font_idx) &&
                     (font_size == a.font_size) &&
                     (invisible == a.invisible) &&
                     (link_idx == a.link_idx)
                     );
        }
        bool operator!=(const TextAttribs& a) const {
            return !(*this == a);
        }

        bool are_valid() const { return ((font_idx != -1) && (font_size != 0)); }
        void update_font(intmax_t font_index, double font_size);

        intmax_t font_idx;
        double font_size;
        double baseline_threshold;
        bool invisible;
        intmax_t link_idx;
    };

    //
    // tracked per character
    struct TextMetrics
    {
        TextMetrics(double char_leading, double char_rise,
                    double char_spacing, double char_h_scaling) :
            leading(char_leading), rise(char_rise),
            char_space(char_spacing), horiz_scaling(char_h_scaling)
        { }

        double leading;
        double rise;
        double char_space;
        double horiz_scaling;
    };

    // -------------------------------------------------------
    // Pdf unicode character. Used to form PdfText spans.
    //
    class PdfChar : public PdfBoxedItem {
    public:
        PdfChar(const BoundingBox& bbox,
                const PdfTM& text_ctm, uintmax_t unicode_c,
                const TextAttribs& txt_attribs, const GfxAttribs& g_attribs,
                const TextMetrics& txt_metrics,
                intmax_t char_glyph_idx, intmax_t clip_id) :
            PdfBoxedItem(bbox, text_ctm),
            attribs(txt_attribs, g_attribs, clip_id), metrics(txt_metrics),
            glyph_idx(char_glyph_idx)
        { unicode = unicode_c; }

        const std::wstring& wstr() const { return unicode; }
        bool is_space() const { return std::iswspace(unicode[0]); }

        // checks if this character is "adjacent" to another. That is,
        // this->spans(previous_char)?
        bool spans(const PdfChar& pre) const;
        intmax_t get_glyph_index() const { return glyph_idx; }

        double width() const { return bbox.width(); }

        // coordinate of x-most vertex for the given position, taking
        // in account rotation.
        double left() const;
        double right() const;
        double top() const;
        double bottom() const;

        // attributes common to both PdfChar and PdfText
        struct Attribs
        {
            TextAttribs txt;
            GfxAttribs gfx;
            intmax_t clip_path_id;

            Attribs() : clip_path_id(-1) {}
            Attribs(const TextAttribs& ta, const GfxAttribs& ga, intmax_t cp_id) :
                txt(ta), gfx(ga), clip_path_id(cp_id)
            {}

            bool operator!=(const Attribs& a2) const {
                return ( (txt != a2.txt) ||
                         (gfx.fill != a2.gfx.fill) ||
                         (clip_path_id != a2.clip_path_id) );
            }
        };

    private:
        Attribs attribs;
        TextMetrics metrics;
        std::wstring unicode;
        intmax_t glyph_idx;

        friend class PdfText;

        virtual std::ostream& to_edn(std::ostream& o) const { return o; }
    };


    // -------------------------------------------------------
    // pdf text sequence. Collects characters in a list and computes
    // bounding box with call to finalize() (done when the span is
    // ready to be inserted)
    //
    class PdfText : public PdfBoxedItem {
    public:

        PdfText() : overlap_pred(nullptr) { }
        PdfText(const PdfTM& ctm) : PdfBoxedItem(ctm), overlap_pred(nullptr) { }
        virtual ~PdfText() { util::delete_ptr_container_elems(chars); delete overlap_pred; }

        // accessors, setters
        uintmax_t length() const { return chars.size(); }
        double font_size() const { return attribs.txt.font_size; }
        bool spans(const PdfChar& c) const { return c.spans( *(chars.back()) ); }
        bool push_back(PdfChar* c);
        void whiteout(const BoundingBox& wo_region); // remove characters from the span covered by the region
        void finalize();
        intmax_t clip_id() const { return attribs.clip_path_id; }

        virtual bool is_positioned_before(const PdfBoxedItem* i) const;

        // OverlapPred to check if a text span "overwrites" an existing one
        static const double SPAN_OVERLAP_THRESHOLD;

        struct OverlapPred
        {
            const BoundingBox& ps_bbox; // pending span bbox

            OverlapPred(const PdfText& span) :
                ps_bbox(span.bounding_box())
            {}

            bool operator()(PdfBoxedItem* const bi);
        };

        const OverlapPred& overlap_predicate() const;

        std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_TYPE_SPAN;
        static const pdftoedn::Symbol SYMBOL_ORIGIN;
        static const pdftoedn::Symbol SYMBOL_GLYPH_IDX;

    private:
        PdfChar::Attribs attribs;
        mutable std::list<PdfChar *> chars;
        // used for checking text overlaps
        mutable OverlapPred *overlap_pred;

        void trim();
    };


    // -------------------------------------------------------
    // pdf characters without mappings - must be drawn using paths
    // TODO: FIX
    class PdfGlyph : public PdfPath {
    public:
        PdfGlyph(uintmax_t glyph_font_idx, uintmax_t glyph_unicode) :
            //            PdfPath(PdfGlyph::SYMBOL),
            font_idx(glyph_font_idx), unicode(glyph_unicode)
        { }

        bool equals(uintmax_t glyph_font_idx, uintmax_t glyph_unicode) const {
            return ((glyph_font_idx == font_idx) && (glyph_unicode == unicode));
        }

        static const pdftoedn::Symbol SYMBOL;

        //        virtual Rice::Object to_ruby() const;

    private:
        uintmax_t font_idx;
        uintmax_t unicode;
    };

} // namespace
