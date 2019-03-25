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

#include <ostream>
#include <list>

#include "graphics.h"
#include "pdf_links.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // move_to
    //
    struct PdfMoveTo : public PdfSubPathCmd {
        static const pdftoedn::Symbol SYMBOL_COMMAND;

        PdfMoveTo(const Coord& c) :
            PdfSubPathCmd(SYMBOL_COMMAND, c)
        { }
    };

    // -------------------------------------------------------
    // curve_to
    //
    struct PdfCurveTo : public PdfSubPathCmd {
        static const pdftoedn::Symbol SYMBOL_COMMAND;

        PdfCurveTo(const Coord& c1, const Coord& c2, const Coord& c3) :
            PdfSubPathCmd(SYMBOL_COMMAND, c1, c2, c3)
        { }

        virtual bool is_curved() const { return true; }
     };

    // -------------------------------------------------------
    // line_to
    //
    struct PdfLineTo : public PdfSubPathCmd {
        static const pdftoedn::Symbol SYMBOL_COMMAND;

        PdfLineTo(const Coord& c) :
            PdfSubPathCmd(SYMBOL_COMMAND, c)
        { }
    };

    // -------------------------------------------------------
    // close_path
    //
    struct PdfClosePath : public PdfSubPathCmd {
        static const pdftoedn::Symbol SYMBOL_COMMAND;

        PdfClosePath() :
            PdfSubPathCmd(SYMBOL_COMMAND)
        { }
    };


    // const statics
    const pdftoedn::Symbol PdfGfxCmd::SYMBOL_TYPE               = "type";

    const pdftoedn::Symbol GfxAttribs::SYMBOL_STROKE_COLOR_IDX  = "stroke_color_idx";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_STROKE_OPACITY    = "stroke_opacity";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_FILL_COLOR_IDX    = "fill_color_idx";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_FILL_OPACITY      = "fill_opacity";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_STROKE_OVERPRINT  = "stroke_overprint_mode";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_FILL_OVERPRINT    = "fill_overprint_mode";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_LINE_WIDTH        = "line_width";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_MITER_LIMIT       = "miter_limit";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_LINE_CAP          = "line_cap";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_LINE_CAP_STYLE[]  = { "butt", "round", "square", "" /* delim */ };
    const pdftoedn::Symbol GfxAttribs::SYMBOL_LINE_JOIN         = "line_join";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_LINE_JOIN_STYLE[] = { "miter", "round", "bevel", "" /* delim */ };
    const pdftoedn::Symbol GfxAttribs::SYMBOL_DASH_VECTOR       = "dash_array";
    const pdftoedn::Symbol GfxAttribs::SYMBOL_BLEND_MODE        = "blend_mode";
    // only ones supported by SVG at the moment
    const pdftoedn::Symbol GfxAttribs::SYMBOL_BLEND_MODE_TYPES[] = { "normal", "multiply", "screen", "darken", "lighten", "" /* delim */ };
    // not supported by SVG ATM
    const pdftoedn::Symbol GfxAttribs::SYMBOL_OVERPRINT_MODE_TYPES[] = { "standard", "illustrator" };

    const pdftoedn::Symbol PdfPath::SYMBOL_TYPE_PATH            = "path";
    const pdftoedn::Symbol PdfPath::SYMBOL_COMMAND_LIST         = "commands";

    const pdftoedn::Symbol PdfDocPath::SYMBOL_PATH_TYPE         = "path_type";
    const pdftoedn::Symbol PdfDocPath::SYMBOL_PATH_TYPES[]      = { "stroke", "fill", "clip", "clip_to_stroke" };
    const pdftoedn::Symbol PdfDocPath::SYMBOL_ATTRIBS           = "attribs";
    const pdftoedn::Symbol PdfDocPath::SYMBOL_EVEN_ODD          = "even_odd";
    const pdftoedn::Symbol PdfDocPath::SYMBOL_ID                = "id";
    const pdftoedn::Symbol PdfDocPath::SYMBOL_CLIP_TO           = "clip_path";

    const pdftoedn::Symbol PdfMoveTo::SYMBOL_COMMAND            = "move_to";
    const pdftoedn::Symbol PdfCurveTo::SYMBOL_COMMAND           = "curve_to";
    const pdftoedn::Symbol PdfLineTo::SYMBOL_COMMAND            = "line_to";
    const pdftoedn::Symbol PdfClosePath::SYMBOL_COMMAND         = "close_path";


    // -------------------------------------------------------
    // base gfx command class
    //
    util::edn::Vector& PdfGfxCmd::to_edn_vector(util::edn::Vector& cmd_a) const
    {
        cmd_a.push( cmd );
        return cmd_a;
    }

    std::ostream& PdfGfxCmd::to_edn(std::ostream& o) const
    {
        util::edn::Vector cmd_h(1);
        o << to_edn_vector(cmd_h);
        return o;
    }

    // -------------------------------------------------------
    // coordinate-op commands
    //

    //
    //
    PdfSubPathCmd::PdfSubPathCmd(const pdftoedn::Symbol& cmd_name,
                                 const Coord& c1,
                                 const Coord& c2,
                                 const Coord& c3) :
        PdfGfxCmd(cmd_name)
    {
        coords.push_back(c1);
        coords.push_back(c2);
        coords.push_back(c3);
    }

    //
    // returns the last coordinate added
    bool PdfSubPathCmd::get_cur_pt(Coord& c) const
    {
        if (coords.empty()) {
            return false;
        }

        c = coords.back();
        return true;
    }

    //
    // command EDN output
    std::ostream& PdfSubPathCmd::to_edn(std::ostream& o) const
    {
        util::edn::Vector cmds_v(2);
        PdfGfxCmd::to_edn_vector(cmds_v);

        if (coords.size() == 1) {
            // if there's only a single coordinate, store it on its
            // own
            cmds_v.push( &coords.back() );
        }
        else if (coords.size() > 1) {
            // there are more than one (or, zero, I guess but that
            // would be odd). Wrap the coords in an array
            util::edn::Vector coords_a(coords.size());
            for ( const Coord& c : coords ) {
                coords_a.push( c );
            }
            cmds_v.push( coords_a );
        }
        o << cmds_v;
        return o;
    }


    // -------------------------------------------------------
    // gfx attributes helper class
    //
    bool GfxAttribs::equals(const GfxAttribs& a2) const
    {
        return (
                (stroke == a2.stroke) &&
                (fill == a2.fill) &&
                (l_width == a2.l_width) &&
                (miter_limit == a2.miter_limit) &&
                (line_cap == a2.line_cap) &&
                (line_join == a2.line_join) &&
                (blend_mode == a2.blend_mode)
                );
    }

    //
    // transform the line wdith using the state's CTM for output
    double GfxAttribs::line_width() const
    {
        return ctm.transform_line_width(l_width);
    }

    //
    // similar for line dashes
    const std::vector<double> GfxAttribs::line_dash() const
    {
        std::vector<double> xformed_dash;
        for (double d : l_dash) {
            xformed_dash.push_back(ctm.transform_line_width(d));
        }
        return xformed_dash;
    }

    //
    // debug
    std::ostream& GfxAttribs::StrokeFill::dump(std::ostream& o) const {
        o << " ci: " << color_idx
          << ", o: " << opacity
          << ", op: " << overprint;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, const GfxAttribs::StrokeFill& sf) {
        sf.dump(o);
        return o;
    }

    std::ostream& GfxAttribs::dump(std::ostream& o) const
    {
        o << "  S: " << stroke << std::endl
          << "  F: " << fill << std::endl
          << "  lw: " << l_width
          << ", ml: " << miter_limit
          << ", lc: " << (int) line_cap
          << ", lj: " << (int) line_join
          << ", bm: " << (int) blend_mode;
        return o;
    }
    std::ostream& operator<<(std::ostream& o, const GfxAttribs& a)
    {
        a.dump(o);
        return o;
    }


    // -------------------------------------------------------
    // path building
    //
    PdfPath::~PdfPath()
    {
        util::delete_ptr_container_elems(cmds);
    }

    //
    // compare if two paths are the same
    bool PdfPath::equals(const PdfPath& p2) const
    {
        if (cmds.size() != p2.cmds.size()) {
            return false;
        }

        for (auto i = cmds.begin(), j = p2.cmds.begin();
             (i != cmds.end() && j != p2.cmds.end());
             ++i, ++j)
        {
            if (*i != *j)
                return false;
        }
        return true;
    }


    //
    // extracts the last coordinate from the most recently added sub
    // path command
    bool PdfPath::get_cur_pt(Coord& c) const
    {
        // fetch the latest command and grab its coordinates
        return cmds.back()->get_cur_pt(c);
    }

    //
    // move_to is always the start of a path
    void PdfPath::move_to(const Coord& c)
    {
        cmds.push_back( new PdfMoveTo(c) );

        // set the bounds to this first coord
        bounds.expand(c);
    }

    // move_to is then followed by a curve_to with three coords
    void PdfPath::curve_to(const Coord& c1, const Coord& c2, const Coord& c3)
    {
        cmds.push_back( new PdfCurveTo(c1, c2, c3) );

        // resize bounding box if needed
        bounds.expand(c1);
        bounds.expand(c2);
        bounds.expand(c3);

        if (shape == UNKNOWN) {
            shape = IRREGULAR;
        }
    }

    //
    // or a line_to
    void PdfPath::line_to(const Coord& c)
    {
        cmds.push_back( new PdfLineTo(c) );
        bounds.expand(c);

        if (shape == UNKNOWN && cmds.size() > 5) {
            shape = IRREGULAR;
        }
    }

    //
    // mark a path closed
    void PdfPath::close()
    {
        cmds.push_back( new PdfClosePath );

        // check if rectangular
        if (shape == UNKNOWN && cmds.size() == 5) {
            Coord c[4];
            auto cmd = std::next(cmds.begin());

            for (uint8_t ii = 0; cmd != cmds.end(); ++cmd, ++ii) {
                if ((*cmd)->is_curved()) {
                    // there's a curve command.. break out
                    return;
                }

                (*(cmd))->get_cur_pt(c[ii]);
            }

            // build two bounding boxes with the four corners and compare them
            BoundingBox a(c[0], c[2]);
            BoundingBox b(c[1], c[3]);
            if  (a.x_min() == b.x_min() &&
                 a.y_min() == b.y_min() &&
                 a.x_max() == b.x_max() &&
                 a.y_max() == b.y_max()) {
                shape = RECTANGULAR;
            }
        }
    }


    //
    // path EDN output
    util::edn::Hash& PdfPath::to_edn_hash(util::edn::Hash& path_h) const
    {
        path_h.reserve(3);

        // contains the type, commands and attributes
        path_h.push( PdfGfxCmd::SYMBOL_TYPE, SYMBOL_TYPE_PATH );

        // traverse the cmds inserting them into an array
        util::edn::Vector path_a(cmds.size() + 1);
        for (const PdfSubPathCmd* c : cmds) {
            path_a.push( c );
        }
        path_h.push( SYMBOL_COMMAND_LIST, path_a );

        // add the path bounds
        path_h.push( BoundingBox::SYMBOL, bounds );
        return path_h;
    }

    std::ostream& PdfPath::to_edn(std::ostream& o) const
    {
        util::edn::Hash path_h;
        o << to_edn_hash(path_h);
        return o;
    }


    // -------------------------------------------------------
    // Document Paths
    //

    //
    // compares to paths to see if the coordinates and attributes are
    // the same
    bool PdfDocPath::equals(const PdfDocPath& p2) const
    {
        // same command type?
        if (path_type != p2.path_type) {
            //            std::cerr << "  * different path_type" << std::endl;
            return false;
        }

        // even-odd rule equal?
        if (even_odd != p2.even_odd) {
            return false;
        }

        // bounds equal?
        if (bounds != p2.bounds) {
            /*            std::cerr << "  * different bounds" << std::endl
                 << "     " << bounds << std::endl
                 << "     " << p2.bounds << std::endl;*/
            return false;
        }

        // if fill or stroke, are attribs equal?
        if (path_type != PdfDocPath::CLIP && attribs != p2.attribs) {
            /*            std::cerr << "  * different attribs" << std::endl
                 << "    " << attribs << std::endl
                 << "    " << p2.attribs << std::endl;*/
            return false;
        }

        // equal number of cmds?
        if (cmds.size() != p2.cmds.size()) {
            //            std::cerr << "  * different cmd count" << std::endl;
            return false;
        }

        // command list equal?
        for (auto ii = cmds.rbegin(), jj = p2.cmds.rbegin(); ii != cmds.rend(); ++ii, ++jj)
        {
            if (!((*ii)->equals(*(*jj)))) {
                //                std::cerr << "  * different subpaths" << std::endl;
                return false;
            }
        }

        return true;
    }


    //
    // doc path output
    std::ostream& PdfDocPath::to_edn(std::ostream& o) const
    {
        // ready to produce the output - a hash contains the path data
        // and attributes
        util::edn::Hash path_h(5);
        to_edn_hash(path_h);

        path_h.push( SYMBOL_PATH_TYPE, SYMBOL_PATH_TYPES[path_type] );

        // meaning of clip_id depends on the path type:
        if (path_type == PdfDocPath::CLIP) {
            // for CLIP paths, it is the id for SVG output
            path_h.push( SYMBOL_ID, clip_id );
        } else if (clip_id != -1) {
            // for FILL or STROKE, it is the clip path id to clip to
            // but only if != -1 ('-1' refers to the original clip -
            // aka the whole page)
            path_h.push( SYMBOL_CLIP_TO, clip_id );
        }

        // even-odd if set
        if (even_odd) {
            path_h.push( SYMBOL_EVEN_ODD, true );
        }

        // if within a link annot
        if (link_idx != -1) {
            path_h.push( PdfLink::SYMBOL_LINK_IDX, link_idx );
        }

        util::edn::Hash attribs_h;
        path_h.push( SYMBOL_ATTRIBS, attribs_to_edn_hash(attribs_h) );
        o << path_h;
        return o;
    }


    util::edn::Hash& PdfDocPath::attribs_to_edn_hash(util::edn::Hash& attribs_h) const
    {
        attribs_h.reserve(9);
        if (path_type != CLIP)
        {
            if (path_type == STROKE)
            {
                // stroke attributes
                if (attribs.stroke.color_idx != -1) {
                    attribs_h.push( GfxAttribs::SYMBOL_STROKE_COLOR_IDX, attribs.stroke.color_idx );

                    if (attribs.stroke.opacity < 1.0) {
                        attribs_h.push( GfxAttribs::SYMBOL_STROKE_OPACITY, attribs.stroke.opacity );
                    }

                    // line width & miter limit
                    attribs_h.push( GfxAttribs::SYMBOL_LINE_WIDTH, attribs.line_width() );
                    attribs_h.push( GfxAttribs::SYMBOL_MITER_LIMIT, attribs.miter_limit );

                    // translate the line cap:
                    // 0 -> butt, 1 -> round, 2 -> square
                    if (attribs.line_cap != -1 && attribs.line_cap < GfxAttribs::LINE_CAP_STYLE_COUNT) {
                        attribs_h.push( GfxAttribs::SYMBOL_LINE_CAP, GfxAttribs::SYMBOL_LINE_CAP_STYLE[attribs.line_cap] );
                    }

                    // translate the line join:
                    // 0 -> miter, 1 -> round, 2 -> bevel
                    if (attribs.line_join != -1 && attribs.line_join < GfxAttribs::LINE_JOIN_STYLE_COUNT) {
                        attribs_h.push( GfxAttribs::SYMBOL_LINE_JOIN, GfxAttribs::SYMBOL_LINE_JOIN_STYLE[attribs.line_join] );
                    }

                    // line dash
                    std::vector<double> xformed_dash = attribs.line_dash();
                    if (!xformed_dash.empty()) {
                        util::edn::Vector dash_a(xformed_dash.size());
                        for (double d : xformed_dash) {
                            dash_a.push(d);
                        }
                        attribs_h.push( GfxAttribs::SYMBOL_DASH_VECTOR, dash_a );
                    }

                    // overprint
                    if (attribs.stroke.overprint && attribs.overprint_mode < GfxAttribs::OVERPRINT_MODE_COUNT) {
                        attribs_h.push( GfxAttribs::SYMBOL_STROKE_OVERPRINT, GfxAttribs::SYMBOL_OVERPRINT_MODE_TYPES[ attribs.overprint_mode ] );
                    }
                }
            }
            else {
                // FILL attributes
                if (attribs.fill.color_idx != -1) {
                    attribs_h.push( GfxAttribs::SYMBOL_FILL_COLOR_IDX, attribs.fill.color_idx );

                    if (attribs.fill.opacity < 1.0) {
                        attribs_h.push( GfxAttribs::SYMBOL_FILL_OPACITY, attribs.fill.opacity );
                    }

                    // overprint
                    if (attribs.fill.overprint && attribs.overprint_mode < GfxAttribs::OVERPRINT_MODE_COUNT) {
                        attribs_h.push( GfxAttribs::SYMBOL_FILL_OVERPRINT, GfxAttribs::SYMBOL_OVERPRINT_MODE_TYPES[ attribs.overprint_mode ] );
                    }
                }
            }

            // blend mode
            if (attribs.blend_mode != GfxAttribs::NORMAL_BLEND &&
                attribs.blend_mode < GfxAttribs::BLEND_MODE_COUNT) {
                attribs_h.push( GfxAttribs::SYMBOL_BLEND_MODE, GfxAttribs::SYMBOL_BLEND_MODE_TYPES[attribs.blend_mode] );
            }
        }
        return attribs_h;
    }
} // namespace
