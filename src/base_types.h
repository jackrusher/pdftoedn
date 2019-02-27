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

#include <sstream>
#include <ostream>
#include <cmath>

namespace pdftoedn
{
    // forward declarations of common types needed for output
    namespace util {
        namespace edn {
            struct Vector;
            struct Hash;
        }
    }

    // module string for error reporting
    extern const char* MODULE;

    // -------------------------------------------------------
    // abstract base class for all types that need to be returned as
    // ruby objects
    //
    struct gemable {
        virtual ~gemable() { }

        virtual std::ostream& to_edn(std::ostream& o) const = 0;
        friend std::ostream& operator<<(std::ostream& o, const gemable& g) {
            g.to_edn(o);
            return o;
        }
    };


    // -------------------------------------------------------
    // edn symbols with a leading ':'
    //
    struct Symbol : public gemable
    {
        Symbol() =default;
        Symbol(const char* s) : str(s) {}
        explicit Symbol(const std::string& s) : str(s) {}

        virtual std::ostream& to_edn(std::ostream& o) const {
            o << ":" << str;
            return o;
        }

    private:
        std::string str;
    };


    // -------------------------------------------------------
    // a simple coordinate container
    //
    struct Coord : public gemable {
        double x, y;

        Coord() : x(0), y(0) { }
        // change -0 weirdness to just 0
        Coord(double _x, double _y) :
            x( (_x == -0) ? 0 : _x), y( (_y == -0) ? 0 : _y)
        { }

        bool operator==(const Coord& c) const { return (x == c.x && y == c.y); }

        virtual std::ostream& to_edn(std::ostream& o) const;
    };


    // -------------------------------------------------------
    // transform matrix
    //
    class PdfTM {
    public:
        enum TM_indices {
            A, B, C, D, E, F,

            MATRIX_VECTOR_SIZE
        };

        static const double RAD2DEG;
        static const double DEG2RAD;

        // constructors
        PdfTM() :
            m11(1), m21(0), m12(0), m22(1), dx(0), dy(0)
        { }
        PdfTM(const double* const ctm) :
            m11(ctm[A]), m21(ctm[B]),
            m12(ctm[C]), m22(ctm[D]),
            dx(ctm[E]),  dy(ctm[F])
        { }
        PdfTM(double radians, double ox, double oy);
        PdfTM(double radians, const Coord& c);
        // direct constructor - no matrix manip performed
        PdfTM(double a, double b, double c, double d, double e, double f) :
            m11(a), m21(b), m12(c), m22(d), dx(e), dy(f)
        { }

        // accessors
        double a() const { return m11; }
        double b() const { return m21; }
        double c() const { return m12; }
        double d() const { return m22; }
        double e() const { return dx; }
        double f() const { return dy; }

        bool operator==(const PdfTM& m) const {
            return (m11 == m.m11 && m21 == m.m21 && m12 == m.m12 &&
                    m22 == m.m22 && dx == m.dx && dy == m.dy);
        }

        // queries
        bool is_rotated() const { return (!is_zero(m21) && !is_zero(m12)); }
        bool is_rotation_orthogonal() const;         // is the rotation angle 0, 90, 180 or 270?
        bool is_flipped() const { return ((is_zero(m12) && is_zero(m21)) && (m11 < 0)); }
        bool is_upside_down() const { return ((is_zero(m12) && is_zero(m21)) && (m22 > 0)); }
        bool is_sheared() const;

        bool is_only_scaled() const { return ((m11 > 0) && (is_zero(m12) && is_zero(m21)) && (m22 > 0)); }
        bool is_only_scaled_and_vflipped() const { return ((m11 > 0) && (is_zero(m12) && is_zero(m21)) && (m22 < 0)); }

        bool is_transformed() const { return (is_rotated() || is_flipped() || is_upside_down()); }
        bool is_finite() const {
            return (std::isfinite(m11) && std::isfinite(m12) && std::isfinite(m21) &&
                    std::isfinite(m22) && std::isfinite(dx) && std::isfinite(dy));
        }

        // matrix ops
        double determinant() const { return m11 * m22 - m21 * m12;  }
        PdfTM invert() const;
        void scale(double x, double y);
        PdfTM operator*(const PdfTM& m) const;

        Coord transform(double x, double y) const {
            return Coord( m11 * x + m12 * y + dx, m21 * x + m22 * y + dy );
        }
        Coord transform(const Coord& c) const {
            return transform(c.x, c.y);
        }
        Coord transform_delta(double x, double y) const {
            return Coord( m11 * x + m12 * y, m21 * x + m22 * y );
        }
        Coord transform_delta(const Coord& c) const {
            return transform_delta(c.x, c.y);
        }
        double transform_line_width(double w) const {
            Coord p = transform_delta(w, w);
            return std::min(std::abs(p.x), std::abs(p.y));
        }
        double rotation() const;
        double rotation_deg() const;

        static double deg_to_rad(double degress);
        static double rad_to_deg(double radians);

        // debug
        std::ostream& dump(std::ostream& o) const;
        friend std::ostream& operator<<(std::ostream& o, const PdfTM& tm);

    private:
        double m11, m21, m12, m22, dx, dy;

        static bool is_zero(double v) { return (std::abs(v) < 0.001); }
    };


    // -------------------------------------------------------
    // common bounding box container.. not resizable
    //
    class BoundingBox : public gemable {
    public:

        // constructors
        BoundingBox(double x, double y, double width, double height) {
            init(Coord(x, y), Coord(x + width, y + height));
        }
        BoundingBox(const Coord& p1, double width, double height) {
            init(p1, Coord(c1.x + width, c1.y + height));
        }
        BoundingBox(const Coord& p1, const Coord& p2) {
            init(p1, p2);
        }
        BoundingBox(const PdfTM& tm); // only for computing an image bbox from its CTM

        // getters
        double x1() const { return c1.x; }
        double y1() const { return c1.y; }
        double x2() const { return c2.x; }
        double y2() const { return c2.y; }
        Coord center() const { return Coord( (x1() + x2()) / 2, (y1() + y2()) / 2); }

        const Coord& p1() const { return c1; }
        const Coord& p2() const { return c2; }
        Coord dimensions() const { return Coord(x2() - x1(), y2() - y1()); }
        double width() const { return x2() - x1(); }
        double height() const { return y2() - y1(); }
        double area() const { return width() * height(); }

        double x_min() const { return c1.x; }
        double x_max() const { return c2.x; }
        double y_min() const { return c1.y; }
        double y_max() const { return c2.y; }

        bool operator<(const BoundingBox& b) const;

        // clipping ops
        enum eClipState { NOT_CLIPPED, PARTIALLY_CLIPPED, FULLY_CLIPPED };
        eClipState is_clipped_by(const BoundingBox& bbox) const;
        bool is_inside(const BoundingBox& bbox) const { // alias
            return (is_clipped_by(bbox) != BoundingBox::FULLY_CLIPPED);
        }
        BoundingBox clip(const BoundingBox& clip_bbox) const;
        double intersection_area(const BoundingBox& bbox) const;
        double intersection_area_ratio(const BoundingBox& bbox) const {
            return (intersection_area(bbox) / area());
        }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const Symbol SYMBOL;

    private:
        Coord c1, c2;

        void init(const Coord& a, const Coord& b);
    };


    // -------------------------------------------------------
    // helps determine a bounding box from a sequence of coords.. a
    // hybrid of this and BoundingBox might eventually replace the two
    //
    class Bounds : public gemable {
    public:

        static const double COORD_MIN_VALUE;
        static const double COORD_MAX_VALUE;
        static const double COORD_HALF_MIN_VALUE;
        static const double COORD_HALF_MAX_VALUE;

        Bounds();

        Bounds& operator=(const BoundingBox& bbox);

        // eq?
        bool operator==(const Bounds& b2) const { return equals(b2); }
        bool operator!=(const Bounds& b2) const { return !equals(b2); }

        // adjusts the tracked limits if the passed coordinate is
        // outside of the range
        void expand(const Coord& c);
        void expand(const BoundingBox& bbox);
        void clip(const BoundingBox& bbox);

        // the computed bounding box
        BoundingBox bounding_box() const {
            return BoundingBox(x_min, y_min, x_max - x_min, y_max - y_min);
        }

        virtual std::ostream& to_edn(std::ostream& o) const;

    private:
        double x_min, y_min, x_max, y_max;

        bool equals(const Bounds& b2) const;
    };


    // -------------------------------------------------------
    // abstract base class for items that are bound by a box
    //
    class PdfBoxedItem : public gemable {
    protected:
        BoundingBox bbox;
        PdfTM ctm;

        PdfBoxedItem() : bbox(0, 0, 0, 0) { }
        PdfBoxedItem(const PdfTM& item_ctm) : bbox(0, 0, 0, 0), ctm(item_ctm) { }
        PdfBoxedItem(const BoundingBox& item_bbox, const PdfTM& item_ctm) :
            bbox(item_bbox), ctm(item_ctm) { }
        PdfBoxedItem(double x, double y, double dx, double dy, const PdfTM& item_ctm) :
            bbox(x, y, dx, dy), ctm(item_ctm) {}

    public:
        // getters
        const PdfTM& CTM() const { return ctm; }

        double x1() const { return bbox.x1(); }
        double y1() const { return bbox.y1(); }
        double x2() const { return bbox.x2(); }
        double y2() const { return bbox.y2(); }

        double x_min() const { return bbox.x_min(); }
        double y_min() const { return bbox.y_min(); }
        double x_max() const { return bbox.x_max(); }
        double y_max() const { return bbox.y_max(); }

        const BoundingBox& bounding_box() const { return bbox; }

        virtual bool is_positioned_before(const PdfBoxedItem* i) const { return (bbox < i->bbox); }

        static const Symbol SYMBOL_ROTATION;
        static const Symbol SYMBOL_XFORM;
        static const Symbol SYMBOL_SHEARED;

        // predicate for sorting a set of boxed items
        struct lt {
            bool operator()(const pdftoedn::PdfBoxedItem* const i1, const pdftoedn::PdfBoxedItem* const i2) const {
                return i1->is_positioned_before(i2);
            }
        };
    };

} // namespace
