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

#include <poppler/GfxState.h>

#include "link_output_dev.h"
#include "doc_page.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // LinkOutputDev
    //------------------------------------------------------------------------
    void LinkOutputDev::startPage(int pageNum, GfxState *state, XRef *xref)
    {
        int w, h, rot;

        // set up graphics output for page
        if (state) {
            w = EngOutputDev::adjust_page_dim(state->getPageWidth());
            h = EngOutputDev::adjust_page_dim(state->getPageHeight());
            rot = state->getRotate();
        } else {
            w = h = 1;
            rot = 0;
        }

        // allocate a new page in the collector doc. and register the
        // error callback
        if (pg_data) {
            delete pg_data;
        }

        pg_data = new pdftoedn::PdfPage(pageNum, w, h, rot);

        // links
        process_page_links( pageNum );
    }

} // namespace
