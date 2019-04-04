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

#include <cfloat>
#include <iomanip>
#include <ostream>
#include <limits>

// next two are for M_PI
#define _USE_MATH_DEFINES
#include <cmath>

#include "base_types.h"
#include "util_edn.h"

namespace pdftoedn
{
    // static initializers
    const char* MODULE = "Edsel";

    const double PdfTM::RAD2DEG                     = 180 / M_PI;
    const double PdfTM::DEG2RAD                     = 1 / RAD2DEG;
    const double Bounds::COORD_MIN_VALUE            = std::numeric_limits<double>::lowest();
    const double Bounds::COORD_MAX_VALUE            = std::numeric_limits<double>::max();
    const double Bounds::COORD_HALF_MIN_VALUE       = Bounds::COORD_MIN_VALUE / 2;
    const double Bounds::COORD_HALF_MAX_VALUE       = Bounds::COORD_MAX_VALUE / 2;

    const Symbol BoundingBox::SYMBOL                = "bbox";

    const Symbol PdfBoxedItem::SYMBOL_ROTATION      = "rotation";
    const Symbol PdfBoxedItem::SYMBOL_XFORM         = "transform";
    const Symbol PdfBoxedItem::SYMBOL_SHEARED       = "sheared";

    // =============================================
    // Coordinates
    std::ostream& Coord::to_edn(std::ostream& o) const
    {
        util::edn::Vector v(2);
        v.push(x);
        v.push(y);
        o << v;
        return o;
    }

    // =============================================
    // transform matrix
    //

    //
    // build a matrix for rotation about an angle and origin
    PdfTM::PdfTM(double radians, double ox, double oy) :
        m11(cos(radians)), m21(sin(radians)), m12(-sin(radians)), m22(cos(radians)),
        dx(ox), dy(oy)
    { }

    PdfTM::PdfTM(double radians, const Coord& c) :
        m11(cos(radians)), m21(sin(radians)), m12(-sin(radians)), m22(cos(radians)),
        dx(c.x), dy(c.y)
    { }


    //
    // is it a sheared matrix? This is used by text in cases where
    // "italics" are done via a matrix shear transform
    bool PdfTM::is_sheared() const
    {
        return (m11 == 1 && m22 == -1 && ((m21 == 0 && m12 != 0) || (m21 != 0 && m12 == 0)));
    }

    //
    // return the matrix inverse
    PdfTM PdfTM::invert() const
    {
        double det = 1 / determinant();
        return PdfTM(
                     det *  m22,
                     det * -m21,
                     det * -m12,
                     det *  m11,
                     det * (m12 * dy - m22 * dx),
                     det * (m21 * dx - m11 * dy)
                     );
    }

    //
    // apply scaling
    void PdfTM::scale(double sx, double sy)
    {
        m11 *= sx; m12 *= sx;
        m21 *= sy; m22 *= sy;
    }

    // multiply two matrices
    PdfTM PdfTM::operator*(const PdfTM& m) const
    {
        return PdfTM(
                     m11 * m.m11 + m12 * m.m21,
                     m21 * m.m11 + m22 * m.m21,
                     m11 * m.m12 + m12 * m.m22,
                     m21 * m.m12 + m22 * m.m22,
                     dx * m.m11 + dy * m.m21 + m.dx,
                     dx * m.m12 + dy * m.m22 + m.dy
                     );
    }

    //
    // is it rotated along the x,y axes
    bool PdfTM::is_rotation_orthogonal() const
    {
        double rot = rotation_deg();
        return (rot == 0 || rot == 90 || rot == 180 || rot == 270);
    }

    //
    // rotation angle in radians
    double PdfTM::rotation() const
    {
        return std::atan2(m21, m11);
    }

    //
    // rotation in degrees - always > 0
    double PdfTM::rotation_deg() const
    {
        double angle = rotation() * RAD2DEG;
        if (angle < 0) {
            angle = 360 + angle;
        }
        // do we need to worry about angles > 359 || < -359? hmm...
        return angle;
    }

    double PdfTM::deg_to_rad(double degrees)
    {
        return (degrees * DEG2RAD);
    }

    double PdfTM::rad_to_deg(double radians)
    {
        return (radians * RAD2DEG);
    }

    //
    // debug
    std::ostream& PdfTM::dump(std::ostream& o) const
    {
        std::ios_base::fmtflags f = o.flags();
        o.precision(5);
        o.fill(' ');

        o << "\t | "
          << std::setw(6) << m11 << " "
          << std::setw(6) << m12 << " "
          << std::setw(6) << dx << " |" << std::endl
          << "\t | "
          << std::setw(6) << m21 << " "
          << std::setw(6) << m22 << " "
          << std::setw(6) << dy << " |";

        if (is_rotated()) {
            o << std::endl << "\trotation: " << (rotation() * RAD2DEG) << "° (as " << rotation_deg() << "°)";
        }

        o.flags(f);
        return o;
    }

    std::ostream& operator<<(std::ostream& o, const PdfTM& tm)
    {
        tm.dump(o);
        return o;
    }

    // =============================================
    // BoundingBox
    //

    void BoundingBox::init(const Coord& a, const Coord& b)
    {
        c1.x = std::min(a.x, b.x);
        c2.x = std::max(a.x, b.x);
        c1.y = std::min(a.y, b.y);
        c2.y = std::max(a.y, b.y);
    }

    //
    // bbox constructor for images - use a CTM to compute it
    BoundingBox::BoundingBox(const PdfTM& tm)
    {
        double x0, y0, x1, y1;

        bool minor_axis_0 = (tm.b() == 0 && tm.c() == 0);
        if (tm.a() > 0 && minor_axis_0 && tm.d() > 0) {
            // scaling only
            x0 = tm.e();
            y0 = tm.f();
            x1 = tm.a() + tm.e();
            y1 = tm.d() + tm.f();
            // ensure boxes are at least one pixel wide
            if (x0 == x1) {
                x1 += 1;
            }
            if (y0 == y1) {
                y1 += 1;
            }
        } else if (tm.a() > 0 && minor_axis_0 && tm.d() < 0) {
            // scaling w/ vertical flip
            x0 = tm.e();
            y0 = tm.d() + tm.f();
            x1 = tm.a() + tm.e();
            y1 = tm.f();
            if (x0 == x1) {
                if (tm.e() + tm.a() * 0.5 < x0) {
                    --x0;
                } else {
                    ++x1;
                }
            }
            if (y0 == y1) {
                if (tm.f() + tm.b() * 0.5 < y0) {
                    --y0;
                } else {
                    ++y1;
                }
            }
        } else {
            // all other cases
            double vx[4], vy[4];
            double t0, t1;

            vx[0] = tm.e();                    vy[0] = tm.f();
            vx[1] = tm.c() + tm.e();           vy[1] = tm.d() + tm.f();
            vx[2] = tm.a() + tm.c() + tm.e();  vy[2] = tm.b() + tm.d() + tm.f();
            vx[3] = tm.a() + tm.e();           vy[3] = tm.b() + tm.f();

            // clipping
            x0 = x1 = vx[0];
            y0 = y1 = vy[0];

            for (uint8_t i = 1; i < 4; ++i) {
                t0 = vx[i];
                if (t0 < x0) {
                    x0 = t0;
                }
                t0 = vx[i];
                if (t0 > x1) {
                    x1 = t0;
                }
                t1 = vy[i];
                if (t1 < y0) {
                    y0 = t1;
                }
                t1 = vy[i];
                if (t1 > y1) {
                    y1 = t1;
                }
            }
        }

        init(Coord(x0, y0), Coord(x1, y1));
    }

    //
    // compare two bounding boxes by position
    bool BoundingBox::operator<(const BoundingBox& b) const
    {
        // TODO: check this...
        if (y_min() == b.y_min()) {
            return (x_min() < b.x_min());
        }

        return (y_min() < b.y_min());
    }


    //
    // are this path's bounds clipped by the given bbox?
    BoundingBox::eClipState BoundingBox::is_clipped_by(const BoundingBox& bbox) const
    {
        // check if it's contained
        if ( (x_min() >= bbox.x_min()) &&
             (x_max() <= bbox.x_max()) &&
             (y_min() >= bbox.y_min()) &&
             (y_max() <= bbox.y_max()) ) {
            return NOT_CLIPPED;
        }

        // check if fully excluded
        if ( (x_min() > bbox.x_max()) ||
             (x_max() < bbox.x_min()) ||
             (y_min() > bbox.y_max()) ||
             (y_max() < bbox.y_min()) ) {
            return FULLY_CLIPPED;
        }

#if 0
        cerr << "comparing: " << std::endl
             << *this << std::endl
             << bbox << std::endl;
#endif
        // even fewer cases
        return PARTIALLY_CLIPPED;
    }


    //
    // clip the bounding box to the given clip's bbox
    BoundingBox BoundingBox::clip(const BoundingBox& clip_bbox) const
    {
        double x1, y1, x2, y2;
        x1 = (clip_bbox.x_min() > x_min()) ? clip_bbox.x_min() : x_min();
        y1 = (clip_bbox.y_min() > y_min()) ? clip_bbox.y_min() : y_min();
        x2 = (clip_bbox.x_max() < x_max()) ? clip_bbox.x_max() : x_max();
        y2 = (clip_bbox.y_max() < y_max()) ? clip_bbox.y_max() : y_max();

        // if fully clipped, adjust so w or h is 0
        if (x2 < x1) {
            x2 = x1;
        }
        if (y2 < y1) {
            y2 = y1;
        }
        return BoundingBox(x1, y1, x2-x1, y2-y1);
    }

    //
    // computes the intersection area between two rectangular bboxes
    // (does not check if they are rectangular)
    double BoundingBox::intersection_area(const BoundingBox& bbox) const
    {
        return ( std::max( 0.0,
                           std::min( bbox.x2(), x2() ) -
                           std::max( bbox.x1(), x1() )
                           ) *
                 std::max( 0.0,
                           std::min( bbox.y2(), y2() ) -
                           std::max( bbox.y1(), y1() )
                           )
                 );
    }


    // output a bounding box
    std::ostream& BoundingBox::to_edn(std::ostream& o) const
    {
        util::edn::Vector v(2);
        v.push(c1);
        v.push(c2);
        o << v;
        return o;
    }

    // =============================================
    // Bounds-builder
    //
    Bounds::Bounds() :
        x_min(COORD_MAX_VALUE), y_min(COORD_MAX_VALUE), x_max(COORD_MIN_VALUE), y_max(COORD_MIN_VALUE)
    {
    }

    //
    // assignment from a BoundingBox
    Bounds& Bounds::operator=(const BoundingBox& bbox)
    {
        x_min = bbox.x_min();
        x_max = bbox.x_max();
        y_min = bbox.y_min();
        y_max = bbox.y_max();
        return *this;
    }

    //
    // checks if two bounds types are equal
    bool Bounds::equals(const Bounds& b2) const
    {
        return (x_min == b2.x_min &&
                y_min == b2.y_min &&
                x_max == b2.x_max &&
                y_max == b2.y_max);
    }

    //
    // check if resize against coordinate values
    void Bounds::expand(const Coord& c)
    {
        // This function was failing, sometimes giving
        // crazy results. I've simplified it at a cost
        // one branch operation in the generated code. --jar
        if (c.x < x_min)
            x_min = c.x;
        if (c.x > x_max)
            x_max = c.x;
        if (c.y < y_min)
            y_min = c.y;
        if (c.y > y_max)
            y_max = c.y;
    }

    //
    // resize against a bounding box
    void Bounds::expand(const BoundingBox& bbox)
    {
        expand(bbox.p1());
        expand(bbox.p2());
    }


    void Bounds::clip(const BoundingBox& bbox)
    {
        if (bbox.x_min() > x_min) {
            x_min = bbox.x_min();
        }
        if (bbox.x_max() < x_max) {
            x_max = bbox.x_max();
        }
        if (bbox.y_min() > y_min) {
            y_min = bbox.y_min();
        }
        if (bbox.y_max() < y_max) {
            y_max = bbox.y_max();
        }

        // ensure it's not an inverted bbox due to full clipping
        if (x_max < x_min) {
            x_max = x_min;
        }
        if (y_max < y_min) {
            y_max = y_min;
        }
    }

    //
    // output Bounds
    std::ostream& Bounds::to_edn(std::ostream& o) const
    {
        // check if we've expanded any of the values. If not, x_min is
        // still set to std::numeric_limits<double>::max() so compare
        // if it's an unreasonable value like it. Rather than checking
        // if equal, see if it's more than half of max() - this should
        // be beyond resonable for PDF content
        if (x_min > Bounds::COORD_HALF_MAX_VALUE) {
            // not set. This is likely due to the Bounds instance not
            // being expanded. Return a [[0, 0], [0, 0]] bbox
            o << BoundingBox(0, 0, 0, 0);
        } else {
            o << bounding_box();
        }

        return o;
    }

} // namespace
