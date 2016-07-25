#include <Page.h>
#include <OutputDev.h>
#include <Catalog.h>
#include <Link.h>
#include <Annot.h>

#include "eng_output_dev.h"
#include "doc_page.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // pdftoedn::EngOutputDev - base class for our output devices
    //------------------------------------------------------------------------

    EngOutputDev::~EngOutputDev()
    {
        delete page_out;
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

                  int page = -1;
                  if (dest) {

                      if (dest->isPageRef()){
                          Ref pageref = dest->getPageRef();
                          page = catalog->findPage(pageref.num, pageref.gen);
                      }
                      else {
                          page = dest->getPageNum();
                      }
                  }

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
                  GooString* uri = ha->getURI();
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
                util::copy_link_meta(*pdf_link, *dest, page_out->height());
                delete dest;
            }

            page_out->new_annot_link(pdf_link);
        }
    }

} // namespace