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

#include <list>
#include "base_types.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // abstract base xform type
    //
    struct Transform : public gemable
    {
        virtual std::ostream& to_edn(std::ostream& o) const = 0;
        static util::edn::Vector& list_to_edn(const std::list<Transform*>& l, util::edn::Vector& transform_a);

        static const pdftoedn::Symbol SYMBOL;
    };


    // -------------------------------------------------------
    // rotation transforms
    //
    class Rotate : public Transform
    {
    public:
        Rotate(double a, double x, double y) :
            angle(a), origin(x, y)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL;

    private:
        double angle;
        Coord origin;
    };


    // -------------------------------------------------------
    // translate transforms
    //
    class Translate : public Transform
    {
    public:
        Translate(double x, double y) :
            delta(x, y)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL;

    private:
        Coord delta;
    };

} // namespace

