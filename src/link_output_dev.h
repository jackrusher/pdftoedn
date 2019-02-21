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

#include "eng_output_dev.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // LinkOutputDev
    //------------------------------------------------------------------------
    class LinkOutputDev : public EngOutputDev
    {
    public:
        // constructor takes reference to object that will store
        // extracted data
        LinkOutputDev(Catalog* doc_cat) :
            EngOutputDev(doc_cat) { }
        virtual ~LinkOutputDev() { }

        // POPPLER virtual interface
        // =========================

        // to prevent updates to fonts, etc.
        virtual void updateAll(GfxState *state) override {}

        //----- initialization and control
        virtual void startPage(int pageNum, GfxState *state, XRef *xref) override;
    };

} // namespace
