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
#include <climits>
#include <cstdlib>
#include <list>
#include <vector>
#include "base_types.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // abstract gfx-type for output commands
    //
    class PdfGfxCmd : public gemable
    {
    public:
        static const pdftoedn::Symbol SYMBOL_TYPE;

        PdfGfxCmd(const pdftoedn::Symbol& gfx_cmd) :
            cmd(gfx_cmd) { }

        virtual std::ostream& to_edn(std::ostream&) const;

    protected:
        const pdftoedn::Symbol& cmd;

        virtual util::edn::Vector& to_edn_vector(util::edn::Vector&) const;
    };


    // -------------------------------------------------------
    // subpath commands (commands with coordinates)
    //
    class PdfSubPathCmd : public PdfGfxCmd
    {
    public:
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_symbol) :
            PdfGfxCmd(cmd_symbol) {}
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_symbol,
                      const Coord& c) :
            PdfSubPathCmd(cmd_symbol) {
            coords.push_back(c);
        }
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_name,
                      const Coord& c1,
                      const Coord& c2,
                      const Coord& c3);

        virtual bool is_curved() const { return false; }
        bool get_cur_pt(Coord& c) const;
        bool equals(const PdfSubPathCmd& c) const { return (coords == c.coords); }

        virtual std::ostream& to_edn(std::ostream&) const;

    protected:
        std::list<Coord> coords;
    };


    // -------------------------------------------------------
    // path building
    //
    class PdfPath : public PdfGfxCmd {
    public:
        enum eShape { UNKNOWN, IRREGULAR, RECTANGULAR };

        static const pdftoedn::Symbol SYMBOL_TYPE_PATH;
        static const pdftoedn::Symbol SYMBOL_COMMAND_LIST;

        // constructors - stroke or fill paths
        PdfPath() : PdfGfxCmd(SYMBOL_TYPE_PATH), shape(UNKNOWN) { }
        PdfPath(const pdftoedn::Symbol& cmd_name) : PdfGfxCmd(cmd_name), shape(UNKNOWN) { }
        virtual ~PdfPath();

        bool equals(const PdfPath& p2) const;

        // subpath commands
        void move_to(const Coord& c);
        void curve_to(const Coord& c1, const Coord& c2, const Coord& c3);
        void line_to(const Coord& c);
        void close();

        uintmax_t length() const { return cmds.size(); }
        bool is_rectangular() const { return (shape == RECTANGULAR); }
        bool get_cur_pt(Coord& c) const;
        BoundingBox bounding_box() const { return bounds.bounding_box(); }

        virtual std::ostream& to_edn(std::ostream&) const;

    protected:
        Bounds bounds;
        eShape shape;
        std::list<PdfSubPathCmd *> cmds;

        virtual util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;
    };


    // -------------------------------------------------------
    // graphics attributes
    //
    struct GfxAttribs
    {
        static const pdftoedn::Symbol SYMBOL_STROKE_COLOR_IDX;
        static const pdftoedn::Symbol SYMBOL_STROKE_OPACITY;
        static const pdftoedn::Symbol SYMBOL_FILL_COLOR_IDX;
        static const pdftoedn::Symbol SYMBOL_FILL_OPACITY;
        static const pdftoedn::Symbol SYMBOL_STROKE_OVERPRINT;
        static const pdftoedn::Symbol SYMBOL_FILL_OVERPRINT;
        static const pdftoedn::Symbol SYMBOL_LINE_WIDTH;
        static const pdftoedn::Symbol SYMBOL_MITER_LIMIT;
        static const pdftoedn::Symbol SYMBOL_LINE_CAP;
        static const pdftoedn::Symbol SYMBOL_LINE_CAP_STYLE[];
        static const pdftoedn::Symbol SYMBOL_LINE_JOIN;
        static const pdftoedn::Symbol SYMBOL_LINE_JOIN_STYLE[];
        static const pdftoedn::Symbol SYMBOL_DASH_VECTOR;
        static const pdftoedn::Symbol SYMBOL_BLEND_MODE;
        static const pdftoedn::Symbol SYMBOL_BLEND_MODE_TYPES[];
        static const pdftoedn::Symbol SYMBOL_OVERPRINT_MODE_TYPES[];

        //
        // line cap styles
        enum LineCapStyles {
            BUTT_CAP,
            ROUND_CAP,
            PROJECTING_SQUARE_CAP,

            LINE_CAP_STYLE_COUNT // delimiter
        };

        enum LineJoinStyles {
            MITER_JOIN,
            ROUND_JOIN,
            BEVEL_JOIN,

            LINE_JOIN_STYLE_COUNT // delim
        };

        enum BlendModes {
            NORMAL_BLEND,
            MULTIPLY_BLEND,
            SCREEN_BLEND,
            DARKEN_BLEND,
            LIGHTEN_BLEND,

            BLEND_MODE_COUNT // delim
        };

        enum OverprintMode {
            STANDARD_OVERPRINT,
            ILLUSTRATOR_OVERPRINT,

            OVERPRINT_MODE_COUNT // delim
        };

        //
        // stroke / fill common attributes
        struct StrokeFill {
            // constructor
            StrokeFill(intmax_t color_index = -1, double _opacity = 0) :
                color_idx(color_index), opacity(_opacity),
                overprint(false)
            { }

            bool operator==(const StrokeFill& sf) const {
                return ( (color_idx == sf.color_idx) &&
                         (opacity == sf.opacity) &&
                         (overprint == sf.overprint) );
            }
            bool operator!=(const StrokeFill& sf) const {
                return ( (color_idx != sf.color_idx) ||
                         (opacity != sf.opacity) ||
                         (overprint != sf.overprint) );
            }

            intmax_t color_idx;
            // int gray_idx; // ??
            double opacity;
            bool overprint;

            std::ostream& dump(std::ostream& o) const;
            friend std::ostream& operator<<(std::ostream& o, const StrokeFill& sf);
        }; // StrokeFill


        //
        // GfxAttribs constructor
        //
        GfxAttribs() :
            miter_limit(0),
            line_cap(-1), line_join(-1),
            blend_mode(NORMAL_BLEND),
            overprint_mode(STANDARD_OVERPRINT),
            clip_idx(-1),
            l_width(0)
        { }

        bool operator!=(const GfxAttribs& a2) const { return !equals(a2); }

        // data
        StrokeFill stroke;
        StrokeFill fill;
        double miter_limit;
        int8_t line_cap;
        int8_t line_join;
        uint8_t blend_mode;
        uint8_t overprint_mode;
        intmax_t clip_idx;

        void update_ctm(const PdfTM& CTM) { ctm = CTM; }
        void update_line_width(double width) { l_width = width; }
        void update_line_dash(const std::vector<double>& line_dash) { l_dash = line_dash; }

        double line_width() const;
        const std::vector<double> line_dash() const;

        void clear_line_dash() { l_dash.clear(); }

        std::ostream& dump(std::ostream& o) const;
        friend std::ostream& operator<<(std::ostream& o, const GfxAttribs& a);

    private:
        PdfTM ctm; // not included in comparisons or output
        // output needs to be transformed for these
        double l_width;
        std::vector<double> l_dash;

        bool equals(const GfxAttribs& a2) const;
    };


    // -------------------------------------------------------
    // path found in PDF content. Carries graphic attribs
    //
    class PdfDocPath : public PdfPath
    {
    public:
        static const pdftoedn::Symbol SYMBOL_PATH_TYPE;
        static const pdftoedn::Symbol SYMBOL_PATH_TYPES[]; // clip, fill, stroke
        static const pdftoedn::Symbol SYMBOL_ATTRIBS;
        static const pdftoedn::Symbol SYMBOL_EVEN_ODD;
        static const pdftoedn::Symbol SYMBOL_ID;
        static const pdftoedn::Symbol SYMBOL_CLIP_TO;

        enum Type {
            STROKE,
            FILL,
            CLIP,
            //            CLIP_TO_STROKE // ??
        };

        enum EvenOddRule {
            EVEN_ODD_RULE_DISABLED,
            EVEN_ODD_RULE_ENABLED
        };

        // constructor
        PdfDocPath(Type doc_path_type,
                   const GfxAttribs& gfx_attribs,
                   EvenOddRule even_odd_flag) :
            path_type(doc_path_type),
            attribs(gfx_attribs),
            even_odd(even_odd_flag),
            clip_id(-1),
            link_idx(-1)
        { }

        // accessors
        Type type() const { return path_type; }
        intmax_t id() const { return clip_id; }
        bool equals(const PdfDocPath& p2) const;

        void set_clip_id(intmax_t id) { clip_id = id; }
        void set_link_idx(intmax_t id) { link_idx = id; }
        void clip_bounds(const BoundingBox& clip_bbox) { bounds.clip(clip_bbox); }

        virtual std::ostream& to_edn(std::ostream& o) const;

    protected:
        Type path_type;
        GfxAttribs attribs;
        EvenOddRule even_odd; // applies only to fill & clip

        // if this is a CLIP Path, this is the id to be used in
        // output; if STROKE or FILL path, this is the id of the clip
        // path to clip to (when != -1)
        intmax_t clip_id;

        // if path is contained within a linked annotation, this
        // points to it
        intmax_t link_idx;

        util::edn::Hash& attribs_to_edn_hash(util::edn::Hash& h) const;
    };

} // namespace
