//
// Copyright 2016-2017 Ed Porras
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

#include <poppler/Page.h>
#include <poppler/OutputDev.h>
#include <poppler/Catalog.h>
#include <poppler/Link.h>
#include <poppler/Annot.h>

#include "eng_output_dev.h"
#include "doc_page.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // pdftoedn::EngOutputDev - base class for our output devices
    //------------------------------------------------------------------------

    EngOutputDev::~EngOutputDev()
    {
        delete pg_data;
    }

    //
    // iterate through the list of links in a page to add them
    void EngOutputDev::process_page_links(int page_num)
    {
        Page* page = catalog->getPage(page_num);

        if (!page) {
            return;
        }

        Links *linksList = page->getLinks();
        for (int i = 0; i < linksList->getNumLinks(); ++i)
        {
            if (linksList->getLink(i)) {
                create_annot_link(linksList->getLink(i));
            }
        }
        delete linksList;
    }


    //
    // helper to add annocation links to a page
    void EngOutputDev::create_annot_link(AnnotLink* annot_link)
    {
        assert(pg_data != NULL && "create_annot_link() - no page storage allocated");

        LinkAction* link_action = annot_link->getAction();

        // getting bit too many times by NULL pointers that shouldn't be.. ugh
        if (!link_action) {
            return;
        }

        LinkActionKind action_kind = link_action->getKind();

        // we are expecting one of the next four.. check this so we
        // don't bother getting rect & effect
        if (action_kind != actionGoTo &&
            action_kind != actionGoToR &&
            action_kind != actionURI &&
            action_kind != actionLaunch) {
            return;
        }

        // get the bounds and visual effect
        int x1, y1, x2, y2, effect;

        PDFRectangle *r = annot_link->getRect();

        // note: swapped y values otherwise bboxes will have inverted y values
        cvtUserToDev(r->x1, r->y1, &x1, &y2);
        cvtUserToDev(r->x2, r->y2, &x2, &y1);

        effect = annot_link->getLinkEffect();

        // here we allocate the appropriate type of link
        PdfAnnotLink* pdf_link = NULL;
        LinkDest *dest = NULL;

        switch (action_kind)
        {
          case actionGoTo:
              {
                  LinkGoTo *ha = dynamic_cast<LinkGoTo*>(link_action);

                  if (!ha) { // paranoid check
                      break;
                  }

                  std::string dest_file;

                  if (ha->getDest() != NULL) {
                      dest = ha->getDest()->copy();
                  }
                  else if (ha->getNamedDest() != NULL) {
                      dest = catalog->findDest(ha->getNamedDest());
                      dest_file = ha->getNamedDest()->getCString();
                  }

                  int page = (dest ? get_dest_goto_page(dest) : -1);

                  pdf_link = new PdfAnnotLinkGoto(x1, y1, x2, y2, effect, page, dest_file);
              }
              break;

          case actionGoToR:
              {
                  LinkGoToR *ha = dynamic_cast<LinkGoToR *>(link_action);

                  if (!ha) {
                      break;
                  }

                  std::string file_dest;

                  if (ha->getFileName()) {
                      file_dest = ha->getFileName()->getCString();
                  }

                  int page = -1;
                  if (ha->getDest() != NULL) {
                      dest = ha->getDest()->copy();

                      if (!dest->isPageRef()) {
                          page = dest->getPageNum();
                      }
                  }
                  pdf_link = new PdfAnnotLinkGotoR(x1, y1, x2, y2, effect, page, file_dest);
              }
              break;

          case actionURI:
              {
                  LinkURI *ha = dynamic_cast<LinkURI *>(link_action);

                  if (!ha) {
                      break;
                  }

                  // found some PDFs where URIs had NULL strings.. huh?
                  GooString* uri = const_cast<GooString *>(ha->getURI());
                  pdf_link = new PdfAnnotLinkURI(x1, y1, x2, y2, effect,
                                                 ((uri != NULL) ? uri->getCString() : ""));
              }
              break;

          case actionLaunch:
              {
                  LinkLaunch *ha = dynamic_cast<LinkLaunch *>(link_action);

                  if (!ha) {
                      break;
                  }

                  pdf_link = new PdfAnnotLinkLaunch(x1, y1, x2, y2, effect,
                                                    ha->getFileName()->getCString());
              }
              break;

          default:
              // unhandled link type
              break;
        }

        if (pdf_link) {
            if (dest) {
                util::copy_link_meta(*pdf_link, *dest, pg_data->height());
                delete dest;
            }

            pg_data->new_annot_link(pdf_link);
        }
    }


    //
    // retrieve the page number from a link actionGoto type
    uintmax_t EngOutputDev::get_dest_goto_page(LinkDest* dest) const
    {
        if (dest->isPageRef()){
            Ref pageref = dest->getPageRef();
            return catalog->findPage(pageref.num, pageref.gen);
        }

        return dest->getPageNum();
    }

} // namespace
