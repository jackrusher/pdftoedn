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

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <sstream>
#include <vector>
#include <assert.h>

#include <poppler/Error.h>
#include <poppler/Object.h>
#include <poppler/GfxFont.h>
#include <poppler/Page.h>
#include <poppler/UTF.h>

#include "pdf_output_dev.h"
#include "font_engine.h"
#include "doc_page.h"
#include "color.h"
#include "graphics.h"
#include "util_encode.h"
#include "util_xform.h"
#include "runtime_options.h"

// debug
//#define ENABLE_OP_TRACE            // dump traces to show op order
//#define ENABLE_PAGE_MARKER
//#define ENABLE_OP_TRACE_TEXT
//#define ENABLE_OP_TRACE_IMG

// trace sub options
#ifdef ENABLE_OP_TRACE_TEXT
//#define ENABLE_OP_TRACE_TEXT_VERBOSE
#endif

//#define ENABLE_IMG_DUMP_TO_DISK    // dump images to desktop

#define DBG_TRACE(t)
#define DBG_TRACE_TXT(t)
#define DBG_TRACE_IMG(t)
#if defined(ENABLE_PAGE_MARKER) || defined(ENABLE_OP_TRACE) || defined(ENABLE_OP_TRACE_TEXT) || defined(ENABLE_OP_TRACE_IMG)
#include <iostream>
#include <iomanip>
#include "util.h"
#include "util_debug.h"
 #ifdef ENABLE_OP_TRACE
  #undef DBG_TRACE
  #define DBG_TRACE(t) t
 #endif
 #ifdef ENABLE_OP_TRACE_TEXT
  #undef DBG_TRACE_TXT
  #define DBG_TRACE_TXT(t) t
 #endif
 #ifdef ENABLE_OP_TRACE_IMG
  #undef DBG_TRACE_IMG
  #define DBG_TRACE_IMG(t) t
 #endif
#endif

#ifdef ENABLE_IMG_DUMP_TO_DISK
  #define DUMP_IMG(basename) util::debug::save_blob_to_disk(blob, basename "-a", ref_num); \
                             if (xformed_blob.str().length() > 0) {                        \
                                 util::debug::save_blob_to_disk( xformed_blob, basename "-x", ref_num); \
                             }
#else
  #define DUMP_IMG(basename)
#endif


namespace pdftoedn
{
    //------------------------------------------------------------------------
    // pdftoedn::OutputDev
    //------------------------------------------------------------------------

    //
    // begin page processing
    // poppler >= 0.24.0 added the xref parameter to startPage
    void OutputDev::startPage(int pageNum, GfxState *state, XRef *xref)
    {
#ifdef ENABLE_PAGE_MARKER
        std::cerr << " + ===== " << __FUNCTION__ << " - page " << (pageNum - 1)<< " ===== + " << std::endl;
#endif

        int w, h, rot;

        // set up graphics output for page
        if (state) {
            // setup HTML output for page
            setDefaultCTM( state->getCTM() );

            w = EngOutputDev::adjust_page_dim(state->getPageWidth());
            h = EngOutputDev::adjust_page_dim(state->getPageHeight());
            rot = state->getRotate();

            updateCTM(state, 0, 0, 0, 0, 0, 0);

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

        // finally, update the xref pointer with the font engine
        if (xref) {
            font_engine.update_document_ref(xref);
        }

        // and extract the page links - we used to do this at the end
        // of the page but now need the links first to check if a
        // text span must be split because of a link
        process_page_links( pageNum );
    }

    //
    //
    // actions to perform once page processing is complete
    void OutputDev::endPage()
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        // close page collection
        pg_data->finalize();
    }

    //
    // state save & restore
    void OutputDev::saveState(GfxState* state)
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        pg_data->push_gfx_state();
    }

    void OutputDev::restoreState(GfxState* state)
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        pg_data->pop_gfx_state();
    }


    //
    //  Text and Font processing
    //
    void OutputDev::updateFont(GfxState *state)
    {
        GfxFont *gfx_font = state->getFont();
        if (!gfx_font) {
            return;
        }

        // sanity-check the font size
        if (EngOutputDev::state_font_size_not_sane(state)) {
            return;
        }

        // load / update the current font
        PdfFont* font = font_engine.load_font(gfx_font);

        if (!font) {
            // error - font engine will report cause
            return;
        }

        DBG_TRACE_TXT(std::cerr << __FUNCTION__ << " - name: " << font->name() << ", family: " << font->family()
                                << " (" << util::debug::get_font_type_str(gfx_font->getType()) << ")"
                                << std::endl);

        // matrix manipulation to compensate for rotated pages -
        // compute the product of the state and text matrices.
        const double* ctma = state->getCTM();
        const double* txta = state->getTextMat();
        text_tm = PdfTM(txta[0] * ctma[0] + txta[1] * ctma[2],
                        -(txta[0] * ctma[1] + txta[1] * ctma[3]),
                        txta[2] * ctma[0] + txta[3] * ctma[2],
                        -(txta[2] * ctma[1] + txta[3] * ctma[3]),
                        0, 0);

        // add the font instance w/ associated (rounded) size to the current page
        pg_data->update_font( font, EngOutputDev::get_transformed_font_size(state) );
    }


    //
    // text callbacks
    //

    //
    // new character to be 'drawn'
    void OutputDev::drawChar(GfxState *state, double x, double y,
                             double dx, double dy,
                             double originX, double originY,
                             CharCode code, int nBytes,
                             Unicode *u, int uLen)
    {
        DBG_TRACE_TXT(std::cerr << __FUNCTION__ << " ");

        // we usually want to ignore non-marking text but some OCR'd
        // PDFs carry text data this way so we've added an option to
        // allow processing
        bool invisible = (state->getRender() == util::TEXT_RENDER_INVISIBLE);
        if (!pdftoedn::options.include_invisible_text() && invisible) {
            return;
        }

        if (pdftoedn::options.gfx_output_only() ||
            state->getStrokeColorSpace()->isNonMarking()) {
            return;
        }

        x -= originX;
        y -= originY;

        // coordinate transform
        double x1, y1, w1, h1, dx2, dy2;
        state->transform(x, y, &x1, &y1);
        state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(), 0, &dx2, &dy2);
        dx -= dx2;
        dy -= dy2;
        state->transformDelta(dx, dy, &w1, &h1);

        // try to do any re-mapping, if applicable
        size_t glyph_idx = -1;
        uintmax_t unicode;

        FontEngine::eCodeRemapStatus status;

        // if there's pending actual text, use the cached unicode
        // value instead of what we receive here
        if (!actual_text.empty()) {
            Unicode at_u = actual_text.front();
            actual_text.pop();
            status = font_engine.get_code_unicode(code, &at_u, unicode);
        }
        else {
            status = font_engine.get_code_unicode(code, u, unicode);
        }

        switch (status) {
          case FontEngine::CODE_REMAP_IGNORE:
          case FontEngine::CODE_REMAP_ERROR:
              return;
              break;
          default:
              break;
        }

#ifdef ENABLE_OP_TRACE_TEXT
#ifdef ENABLE_OP_TRACE_TEXT_VERBOSE
        if (
            !text_tm.is_rotated()
            //            && string("HGMaruGothicMPRO") == state->getFont()->getName()->getCString()
            ) {
            std::cerr << "\t'"
                 << (char) code << "' (" << (int) code << "), u? " << (u != nullptr ? *u : 0)
                 << ", returned unicode: '" << (char) unicode << "' (" << unicode << ") *u? " << (u?*u:0);
                //                 << " - bbox: " << BoundingBox(x1, y1, w1, h1)
                //                 << "\tx, y: " << x << " " << y << ", w: " << w1 << ", h: " << h1 << std::endl
                //                 << "  state rot: " << PdfTM(state->getCTM()).rotation_deg() << "°" << std::endl
                //                 << "  text rot:  " << PdfTM(state->getTextMat()).rotation_deg() << "°" << std::endl
                //                 << "  rslt rot:  " << text_tm.rotation_deg() << "°"
                //                 << ", font: " << state->getFont()->getName()->getCString()
                //                 << ", ch sp * scaling: " << state->getCharSpace() * state->getHorizScaling()
                //                 << ", xformed font size: " <<  state->getTransformedFontSize()
                //                 << "\ttext CTM: " << std::endl << text_tm
        }
#endif
        std::cerr << std::endl;
#endif

        // add the character
        pg_data->new_character( x1, y1, w1, h1,
                                text_tm,
                                TextMetrics( state->getLeading(),
                                             state->getRise(),
                                             state->getCharSpace(),
                                             state->getHorizScaling() ),
                                unicode,
                                glyph_idx,
                                invisible );
    }


    //
    // capture instances of the Actual Text command - these are used
    // so a PDF viewer shows different text from what's encoded. We
    // cache the unicode values and then substitute them when
    // characters come in
    void OutputDev::beginActualText(GfxState* state, const GooString *text )
    {
        if (pdftoedn::options.gfx_output_only() ||
            state->getStrokeColorSpace()->isNonMarking()) {
            return;
        }

        Unicode *uni = nullptr;
        int length = TextStringToUCS4(text, &uni);

#ifdef ENABLE_OP_TRACE
        wchar_t c[2] = { 0, 0 };
        std::wstring s;

        std::cerr << __FUNCTION__;
#endif

        // store the unicode values
        for (int i = 0; i < length; i++) {
            actual_text.push(uni[i]);
#ifdef ENABLE_OP_TRACE
            c[0] = uni[i];
            s += c;
#endif
        }

        DBG_TRACE(std::cerr << " - " << util::string_to_utf(s) << std::endl);

        if (uni) {
            gfree(uni);
        }
    }

    //
    // path drawing
    //

    //
    // stroke
    void OutputDev::stroke(GfxState *state)
    {
        if (state->getStrokeColorSpace()->isNonMarking()) {
            return;
        }

        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        build_path_command(state, PdfDocPath::STROKE);
    }

    //
    // fills
    void OutputDev::fill(GfxState *state)
    {
        if (state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        build_path_command(state, PdfDocPath::FILL);
    }

    //
    // even-odd fills
    void OutputDev::eoFill(GfxState *state)
    {
        if (state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        build_path_command(state, PdfDocPath::FILL, PdfDocPath::EVEN_ODD_RULE_ENABLED);
    }

    //
    // tiled patterns
    bool OutputDev::tilingPatternFill(GfxState* state, Gfx* gfx, Catalog* cat, Object* str,
                                      const double* pmat, int paintType, int tilingType, Dict* resDict,
                                      const double* mat, const double* bbox,
                                      int x0, int y0, int x1, int y1,
                                      double xStep, double yStep)
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        // TESLA-5735 - don't do anything for now as it's not
        // crucial. However, adding this prevents poppler from
        // entering an infinite loop with certain docs
        return true;
    }

    //
    // clip paths
    void OutputDev::clip(GfxState *state)
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        build_path_command(state, PdfDocPath::CLIP);
    }

    //
    // even-odd clip paths
    void OutputDev::eoClip(GfxState *state)
    {
        DBG_TRACE(std::cerr << __FUNCTION__ << std::endl);

        build_path_command(state, PdfDocPath::CLIP, PdfDocPath::EVEN_ODD_RULE_ENABLED);
    }

    //
    // ??
    void OutputDev::clipToStrokePath(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        //        buildClippedPathCommand(state, PdfPath::CLIP_TO_STROKE);
        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }



    //
    // Images
    //

    //
    // 1-bit rasters
    void OutputDev::drawImageMask(GfxState *state, Object *obj, Stream *str,
                                  int width, int height, bool invert, bool interpolate,
                                  bool inlined)
    {
        DBG_TRACE_IMG(std::cerr << "===========================" << std::endl << __FUNCTION__);

        if (pdftoedn::options.text_output_only() ||
            state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        // check matrix is valid
        PdfTM ctm(state->getCTM());
        if (!ctm.is_finite()) {
            et.log_warn( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                         "drawImageMask - mask has non-finite CTM" );
            return;
        }

        intmax_t ref_num;
        if (inlined) {
            // inline images don't carry a resource id so the cache
            // lookup will fail every time. Unfortunately, we have to
            // processed the stream data every time and check if the
            // generated image is then cache by its MD5
            ref_num = inline_img_id;
        }
        else if (obj && obj->isRef()) {
            ref_num = obj->getRef().num;
        }
        else {
            et.log_error( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                         "drawImageMask() - non-inlined image has no valid ref_num. Poppler error?" );
            return;
        }

        BoundingBox bbox(ctm);

        // lookup the object id to see if we've cached it already -
        // inlined images don't have a ref_num so we stream must be
        // extracted anyway and the md5 can be used to determine if
        // it's cached
        if (inlined || !pg_data->image_is_cached(ref_num))
        {
            // poppler's interface to rip through a stream for an image
            ImageStream *imgStr = new ImageStream(str, width, 1, 1);

            // get the fill color of the mask
            GfxRGB fill;
            state->getFillRGB(&fill);

            // set up stream properties
            StreamProps properties(str->getKind(), width, height,
                                   fill, state->getFillColorSpace()->getMode(),
                                   interpolate, ctm.is_upside_down(), inlined, invert);

            DBG_TRACE_IMG( std::cerr << std::endl
                                     << properties << std::endl
                                     << "\tctm: " << std::endl << ctm
                                     << std::endl; );

            // extract the data and copy it to a string stream
            std::ostringstream blob;
            bool encode_status = util::encode::encode_mask(blob, imgStr, properties);

            // poppler cleanup
            delete imgStr;

            // don't continue if encode failed
            if (!encode_status ||
                !process_image_blob(blob, ctm, bbox, properties, width, height, ref_num)) {
                return;
            }

            DUMP_IMG("img_mask");
        }
        DBG_TRACE_IMG(
        else {
            std::cerr << "\tCached image id: " << ref_num << std::endl;
        }
                      );

        // add a meta container for the image
        pg_data->new_image(ref_num, bbox);
    }


    //
    // bitmap + mask - implemented using PNG w/ alpha channel
    void OutputDev::drawSoftMaskedImage(GfxState *state, Object *obj, Stream *str,
                                        int width, int height,
                                        GfxImageColorMap *colorMap,
                                        bool interpolate,
                                        Stream *maskStr,
                                        int maskWidth, int maskHeight,
                                        GfxImageColorMap *maskColorMap,
                                        bool maskInterpolate)
    {
        if (pdftoedn::options.text_output_only() ||
            state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        // check matrix is valid
        PdfTM ctm(state->getCTM());
        if (!ctm.is_finite()) {
            et.log_warn( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                         "drawSoftMaskedImage - mask has non-finite CTM" );
            return;
        }

        intmax_t ref_num;
        if (obj && obj->isRef()) {
            ref_num = obj->getRef().num;
        }
        else {
            et.log_error( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                          "drawSoftMaskedImage() - image has no valid ref_num. Poppler error?" );
            return;
        }

        BoundingBox bbox(ctm);

        // lookup the object id to see if we've cached it already
        if (!pg_data->image_is_cached(ref_num))
        {
            StreamProps properties(str->getKind(), width, height,
                                   colorMap, interpolate,
                                   maskStr->getKind(), maskWidth, maskHeight,
                                   maskColorMap, maskInterpolate,
                                   ctm.is_upside_down());

            DBG_TRACE_IMG(std::cerr << __FUNCTION__ << std::endl
                                    << properties << std::endl
                                    << "           c-space: "
                                    << GfxColorSpace::getColorSpaceModeName(colorMap->getColorSpace()->getMode())
                                    << "      mask c-space: "
                                    << GfxColorSpace::getColorSpaceModeName(maskColorMap->getColorSpace()->getMode())
                                    << std::endl);

            // poppler's interface to rip through a stream for an image
            ImageStream *imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
                                                  colorMap->getBits());
            ImageStream *maskImgStr = new ImageStream(maskStr, maskWidth,
                                                      maskColorMap->getNumPixelComps(),
                                                      maskColorMap->getBits());

            // image data will be written here
            std::ostringstream blob;
            bool encode_status = util::encode::encode_rgba_image(blob, imgStr, maskImgStr,
                                                                 properties,
                                                                 colorMap, maskColorMap,
                                                                 false);
            // poppler cleanup
            delete maskImgStr;
            delete imgStr;

            // don't continue if encode failed
            if (!encode_status ||
                !process_image_blob(blob, ctm, bbox, properties, width, height,
                                    ref_num)) {
                return;
            }

            DUMP_IMG("softmasked_img");
        }

        // add a meta container for the image
        pg_data->new_image(ref_num, bbox);
    }


    //
    // not yet implemented
    void OutputDev::setSoftMaskFromImageMask(GfxState *state,
                                             Object *ref, Stream *str,
                                             int width, int height, bool invert,
                                             bool inlineImg, double *baseMatrix)
    {
        DBG_TRACE_IMG(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix)
    {
        DBG_TRACE_IMG(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        //        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::drawMaskedImage(GfxState *state, Object *obj, Stream *str,
                                    int width, int height,
                                    GfxImageColorMap *colorMap, bool interpolate,
                                    Stream *maskStr, int maskWidth, int maskHeight,
                                    bool maskInvert, bool maskInterpolate)
    {
        DBG_TRACE_IMG(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        if (pdftoedn::options.text_output_only() ||
            state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        // check matrix is valid
        PdfTM ctm(state->getCTM());
        if (!ctm.is_finite()) {
            et.log_warn( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                         "drawSoftMaskedImage - mask has non-finite CTM" );
            return;
        }

        intmax_t ref_num;
        if (obj && obj->isRef()) {
            ref_num = obj->getRef().num;
        }
        else {
            et.log_error( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                          "drawMaskedImage() - image has no valid ref_num. Poppler error?" );
            return;
        }

        BoundingBox bbox(ctm);

        // lookup the object id to see if we've cached it already
        if (!pg_data->image_is_cached(ref_num))
        {
            StreamProps properties(str->getKind(), width, height,
                                   colorMap, interpolate,
                                   maskStr->getKind(), maskWidth, maskHeight,
                                   ctm.is_upside_down(), maskInterpolate, maskInvert);

            DBG_TRACE_IMG(std::cerr << __FUNCTION__ << std::endl
                                    << properties << std::endl
                                    << ", cspace: "
                                    << GfxColorSpace::getColorSpaceModeName(colorMap->getColorSpace()->getMode())
                                    << std::endl);

            // poppler's interface to rip through a stream for an image
            ImageStream *imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
                                                  colorMap->getBits());
            ImageStream *maskImgStr = new ImageStream(maskStr, maskWidth, 1, 1);

            // image data will be written here
            std::ostringstream blob;
            bool encode_status = util::encode::encode_rgba_image(blob, imgStr, maskImgStr,
                                                                 properties,
                                                                 colorMap, nullptr,
                                                                 maskInvert);

            // poppler cleanup
            delete maskImgStr;
            delete imgStr;

            // don't continue if encode failed
            if (!encode_status ||
                !process_image_blob(blob, ctm, bbox, properties, width, height,
                                    ref_num)) {
                return;
            }

            DUMP_IMG("masked_img");
        }

        // add a meta container for the image
        pg_data->new_image(ref_num, bbox);
    }


    //
    // image handling
    void OutputDev::drawImage(GfxState *state, Object *obj, Stream *str,
                              int width, int height, GfxImageColorMap *colorMap,
                              bool interpolate, int *maskColors, bool inlined)
    {
        if (pdftoedn::options.text_output_only() ||
            state->getFillColorSpace()->isNonMarking()) {
            return;
        }

        // check matrix is valid
        PdfTM ctm(state->getCTM());
        if (!ctm.is_finite()) {
            et.log_warn( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                         "drawImage - masked image has non-finite CTM" );
            return;
        }

        // get object reference id
        intmax_t ref_num;
        if (inlined) {
            // inline images don't carry a resource id so the cache
            // lookup will fail every time. Unfortunately, we have to
            // processed the stream data every time and check if the
            // generated image is then cache by its MD5
            ref_num = inline_img_id;
        }
        else if (obj && obj->isRef()) {
            ref_num = obj->getRef().num;
        }
        else {
            et.log_error( ErrorTracker::ERROR_INVALID_ARGS, MODULE,
                          "drawImage() - non-inlined image has no valid ref_num. Poppler error?" );
            return;
        }

        BoundingBox bbox(ctm);

        // lookup the object id to see if we've cached it already -
        // inlined images don't have a ref_num so we stream must be
        // extracted anyway and the md5 can be used to determine if
        // it's cached
        if (inlined || !pg_data->image_is_cached(ref_num))
        {
            int num_pix_comps = colorMap->getNumPixelComps();
            int bpp = colorMap->getBits();
            StreamProps properties(str->getKind(), width, height,
                                   num_pix_comps, bpp,
                                   interpolate, ctm.is_upside_down(), inlined);

            DBG_TRACE_IMG(std::cerr << __FUNCTION__ << std::endl
                                    << properties << std::endl
                                    << "mask colors? " << (maskColors != nullptr) << std::endl
                                    << " ctm: " << std::endl << ctm
                                    << std::endl);

            // poppler's interface to rip through a stream for an image
            ImageStream *imgStr = new ImageStream(str, width, num_pix_comps, bpp);

            // image data will be written here
            std::ostringstream blob;
            bool encode_status = util::encode::encode_image(blob, imgStr, properties, colorMap);

            // poppler cleanup
            delete imgStr;

            if (!encode_status ||
                !process_image_blob(blob, ctm, bbox, properties, width, height,
                                    ref_num)) {
                return;
            }

            DUMP_IMG("img");
        }

        // add a meta container for the image
        pg_data->new_image(ref_num, bbox);
    }


    //
    // transform the encoded image if needed, then cache it
    bool OutputDev::process_image_blob(const std::ostringstream& blob, const PdfTM& ctm,
                                       const BoundingBox& bbox, const StreamProps& properties,
                                       int width, int height,
                                       intmax_t& ref_num)
    {
        std::string data = blob.str();

        // handle transformations if needed
        if (ctm.is_transformed()) {
            if (util::xform::transform_image(ctm, data, width, height,
                                             properties.mask_is_inverted()) == util::xform::XFORM_ERR) {
                // don't continue if transform failed
                return false;
            }
        }

        std::string data_md5 = util::md5(data);

        // we've seen instances of repeated usage of inlined streams
        // so we cache them and search the cache by md5
        if (properties.is_inlined()) {
            // If inlined, PdfPage will search to make sure an image
            // w/ same md5 is not already in the DB and return its
            // ref_num instead of caching it again.  Otherwise, the
            // same ref_num is returned
            intmax_t cached_res_id;
            if (pg_data->inlined_image_is_cached(data_md5, cached_res_id)) {
                // return the resource id found
                ref_num = cached_res_id;
                return true;
            }

            // decrement the custom assigned inline_img_id for
            // the next instance
            inline_img_id -= 1;
        }

        // cache it - cache_image()
        if (!pg_data->cache_image(ref_num, bbox, width, height, properties,
                                  data, data_md5)) {
            // error caching image
            return false;
        }

        return true;
    }


    //
    // gfx state update methods
    void OutputDev::updateAll(GfxState *state)
    {
        ::OutputDev::updateAll(state);

        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        assert((pg_data != nullptr) && "updateAll: PdfPage instance has not been allocated" );
    }


    //
    // line-dashes
    void OutputDev::updateLineDash(GfxState* state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        // line dash pattern
        double *pattern;
        int length;
        double d;
        state->getLineDash(&pattern, &length, &d);

        std::vector<double> line_dash;
        if (length > 0)
        {
            line_dash.reserve(length + 1);

            // dash phase is stored at the head
            line_dash.push_back( d );

            // followed by the dash pattern - lengths have to be
            // tranformed
            PdfTM ctm(state->getCTM());
            for (intmax_t i = 0; i < length; ++i) {
                d = ctm.transform_line_width(pattern[i]);
                line_dash.push_back( d );
            }
            pg_data->update_line_dash(line_dash);
        }
        else {
            // dash pattern reset
            pg_data->clear_line_dash();
        }
    }


    //
    // line join style
    void OutputDev::updateLineJoin(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getLineJoin() << " ---- + " << std::endl);

        pg_data->update_line_join(state->getLineJoin());
    }

    //
    // line cap style
    void OutputDev::updateLineCap(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getLineCap() << " ---- + " << std::endl);

        pg_data->update_line_cap(state->getLineCap());
    }

    //
    // miter limit
    void OutputDev::updateMiterLimit(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getMiterLimit() << " ---- + " << std::endl);

        pg_data->update_miter_limit(state->getMiterLimit());
    }

    //
    // line width
    void OutputDev::updateLineWidth(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": SA? " << std::boolalpha << state->getStrokeAdjust()
                            << ", state LW: " << state->getLineWidth()
                            << " ---- + " << std::endl);
        PdfTM ctm(state->getCTM());
        pg_data->update_line_width( ctm.transform_line_width(state->getLineWidth()) );
    }

    //
    // is soft mask shape / alpha interpreted as shape or opacity?
    void OutputDev::updateAlphaIsShape(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << std::boolalpha << state->getAlphaIsShape() << " ---- + " << std::endl);

        if (state->getAlphaIsShape()) {
            std::stringstream err;
            err << __FUNCTION__ << " - value: " << std::boolalpha << state->getAlphaIsShape();
            et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, err.str() );
        }
    }

    //
    // does text knock out overlapping glyphs
    void OutputDev::updateTextKnockout(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << std::boolalpha << state->getTextKnockout() << " ---- + " << std::endl);

        std::stringstream err;
        err << __FUNCTION__ << " - value: " << std::boolalpha << state->getTextKnockout();
        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, err.str() );
    }

    //
    // fill color
    void OutputDev::updateFillColor(GfxState *state)
    {
        GfxRGB rgb;
        state->getFillRGB(&rgb);

        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << RGBColor(rgb.r, rgb.g, rgb.b) << " ---- + " << std::endl);

        pg_data->update_fill_color(rgb.r, rgb.g, rgb.b);
    }

    //
    // stroke
    void OutputDev::updateStrokeColor(GfxState *state)
    {
        GfxRGB rgb;
        state->getStrokeRGB(&rgb);

        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ":" << RGBColor(rgb.r, rgb.g, rgb.b) << " ---- + " << std::endl);

        pg_data->update_stroke_color(rgb.r, rgb.g, rgb.b);
    }


    void OutputDev::updateBlendMode(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << util::debug::get_blend_mode_str(state->getBlendMode()) << " ---- + " << std::endl);

        pg_data->update_blend_mode(util::pdf_to_svg_blend_mode(state->getBlendMode()));
    }

    void OutputDev::updateFillOpacity(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getFillOpacity() << " ---- + " << std::endl);

        pg_data->update_fill_opacity(state->getFillOpacity());
    }

    void OutputDev::updateStrokeOpacity(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__  << ": " << state->getStrokeOpacity() << " ---- + " << std::endl);

        pg_data->update_stroke_opacity(state->getStrokeOpacity());
    }

    void OutputDev::updateFillOverprint(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getFillOverprint() << " ---- + " << std::endl);

        pg_data->update_fill_overprint(state->getFillOverprint());
    }

    void OutputDev::updateStrokeOverprint(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getStrokeOverprint() << " ---- + " << std::endl);

        pg_data->update_stroke_overprint(state->getStrokeOverprint());
    }

    void OutputDev::updateOverprintMode(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << state->getOverprintMode() << " ---- + " << std::endl);

        pg_data->update_overprint_mode(state->getOverprintMode());
    }

    //
    // updateTextShift tells us when text position shifts happen. I
    // was using this to resolve TESLA-6205 but it was too agressive
    // at splitting up spans. Now resolved by modifying text's spans()
    // method
    void OutputDev::updateTextShift(GfxState *state, double shift)
    {
#if 0 //def ENABLE_OP_TRACE
        std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl;
#endif

#if 0
        double ss = -shift * 0.001 * state->getFontSize();
        int wMode = state->getFont()->getWMode();
        if (!wMode) {
            ss *= state->getHorizScaling();
        }

        // 0.1 is arbitrary but so far looks good enough to tell us
        // when to treat the jump like a space
        if (ss > 0.1) {
#if 0
            std::cerr << " + ---- " << __FUNCTION__ << " shift: " << shift << ", ss: " << ss
                 << " (wMode: " << wMode
                 << "), X, Y: (" << state->getCurX() << ", " << state->getCurY() << "), sp: "
                 << state->getWordSpace() << " ---- + "
                 << std::endl;
#endif
        }
#endif
    }


    //----- grouping operators
    void OutputDev::beginMarkedContent(const char *name, Dict *properties)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << name << " ---- + " << std::endl);

        //        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::endMarkedContent(GfxState *state)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        //        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::markPoint(const char *name)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << name << " ---- + " << std::endl);
    }

    void OutputDev::markPoint(const char *name, Dict *properties)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << ": " << name << " ---- + " << std::endl);
    }


    //----- transparency groups and soft masks
    void OutputDev::beginTransparencyGroup(GfxState * /*state*/, double * /*bbox*/,
                                           GfxColorSpace * /*blendingColorSpace*/,
                                           bool /*isolated*/, bool /*knockout*/,
                                           bool /*forSoftMask*/)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::endTransparencyGroup(GfxState * /*state*/)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::paintTransparencyGroup(GfxState * /*state*/, double * /*bbox*/)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }


    void OutputDev::setSoftMask(GfxState * /*state*/, double * /*bbox*/, bool /*alpha*/,
                                Function * /*transferFunc*/, GfxColor * /*backdropColor*/)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }

    void OutputDev::clearSoftMask(GfxState * /*state*/)
    {
        DBG_TRACE(std::cerr << " + ---- " << __FUNCTION__ << " ---- + " << std::endl);

        et.log_info( ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB, MODULE, __FUNCTION__ );
    }


    // =========================================================================
    // non-virtual methods; helpers.
    //

    void OutputDev::build_path_command(GfxState* state, PdfDocPath::Type type, PdfDocPath::EvenOddRule eo_flag)
    {
        if (pdftoedn::options.text_output_only() ||
            !state->getPath()) {
            return;
        }

        pg_data->add_path(state, type, eo_flag);
    }

} // namespace
