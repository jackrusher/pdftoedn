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

#pragma once

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <queue>

#include <poppler/GfxState.h>

#include "eng_output_dev.h"
#include "graphics.h"

namespace pdftoedn
{
    class FontEngine;
    class StreamProps;

    //------------------------------------------------------------------------
    // pdftoedn::OutputDev
    //------------------------------------------------------------------------
    class OutputDev : public EngOutputDev {
    public:
        // for inlined images (do not have a ref id)
        enum { IMG_RES_ID_UNDEF = -1 };

        // constructor takes reference to object that will store
        // extracted data
        OutputDev(Catalog* doc_cat, pdftoedn::FontEngine& fnt_engine) :
            EngOutputDev(doc_cat),
            font_engine(fnt_engine),
            inline_img_id(IMG_RES_ID_UNDEF - 1)
        { }
        virtual ~OutputDev() { }

        // set up font manager, etc.
        bool init();

        // POPPLER virtual interface
        // =========================
        // Does this device use upside-down coordinates?
        // (Upside-down means (0,0) is the top left corner of the page.)
        virtual bool upsideDown() { return true; }

        virtual bool needNonText() { return true; }
        virtual bool needCharCount() { return true; }

        // Does this device use drawChar() or drawString()?
        virtual bool useDrawChar() { return true; }

        virtual bool useTilingPatternFill() { return true; }

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual bool interpretType3Chars() { return true; }

        // This device now supports text in pattern colorspace!
        virtual bool supportTextCSPattern(GfxState *state) {
            return state->getFillColorSpace()->getMode() == csPattern;
        }

        //----- initialization and control
        virtual void startPage(int pageNum, GfxState *state, XRef *xref);
        virtual void endPage();

        //----- update graphics state
        virtual void saveState(GfxState* state);
        virtual void restoreState(GfxState* state);

        virtual void updateAll(GfxState *state);
        virtual void updateLineDash(GfxState* state);
        virtual void updateFlatness(GfxState * /*state*/) {}
        virtual void updateLineJoin(GfxState *state);
        virtual void updateLineCap(GfxState *state);
        virtual void updateMiterLimit(GfxState *state);
        virtual void updateLineWidth(GfxState *state);
        virtual void updateAlphaIsShape(GfxState *state);
        virtual void updateTextKnockout(GfxState *state);
        virtual void updateFillColorSpace(GfxState * /*state*/) {}
        virtual void updateStrokeColorSpace(GfxState * /*state*/) {}
        virtual void updateFillColor(GfxState *state);
        virtual void updateStrokeColor(GfxState *state);
        virtual void updateBlendMode(GfxState *state);
        virtual void updateFillOpacity(GfxState *state);
        virtual void updateStrokeOpacity(GfxState *state);
        virtual void updateFillOverprint(GfxState *state);
        virtual void updateStrokeOverprint(GfxState *state);
        virtual void updateOverprintMode(GfxState* state);
        virtual void updateTransfer(GfxState * /*state*/) {}
        virtual void updateFillColorStop(GfxState * /*state*/, double /*offset*/) {}

        //----- text state
        virtual void updateFont(GfxState* state);
        virtual void updateTextShift(GfxState* state, double shift);

        //----- text drawing
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen);
        virtual void beginActualText(GfxState* state, const GooString *text );
        virtual void endActualText(GfxState * /*state*/) { }

        //----- paths
        virtual void stroke(GfxState *state);
        virtual void fill(GfxState *state);
        virtual void eoFill(GfxState *state);

        //----- patterns
        virtual bool tilingPatternFill(GfxState * /*state*/, Gfx * /*gfx*/, Catalog * /*cat*/, Object * /*str*/,
                                       const double * /*pmat*/, int /*paintType*/, int /*tilingType*/, Dict * /*resDict*/,
                                       const double * /*mat*/, const double * /*bbox*/,
                                       int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
                                       double /*xStep*/, double /*yStep*/);

        //----- clipping
        virtual void clip(GfxState *state);
        virtual void eoClip(GfxState *state);
        virtual void clipToStrokePath(GfxState *state);

        //----- images
        virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                                   int width, int height, bool invert, bool interpolate,
                                   bool inlineImg);
        virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GfxImageColorMap *colorMap,
                               bool interpolate, int *maskColors, bool inlineImg);

        virtual void setSoftMaskFromImageMask(GfxState *state,
                                              Object *ref, Stream *str,
                                              int width, int height, bool invert,
                                              bool inlineImg, double *baseMatrix);
        virtual void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix);
        virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
                                     int width, int height,
                                     GfxImageColorMap *colorMap, bool interpolate,
                                     Stream *maskStr, int maskWidth, int maskHeight,
                                     bool maskInvert, bool maskInterpolate);
        virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                                         int width, int height,
                                         GfxImageColorMap *colorMap,
                                         bool interpolate,
                                         Stream *maskStr,
                                         int maskWidth, int maskHeight,
                                         GfxImageColorMap *maskColorMap,
                                         bool maskInterpolate);

        //----- grouping operators
        virtual void endMarkedContent(GfxState *state);
        virtual void beginMarkedContent(const char *name, Dict *properties);
        virtual void markPoint(const char *name);
        virtual void markPoint(const char *name, Dict *properties);

        //----- transparency groups and soft masks
        virtual bool checkTransparencyGroup(GfxState * /*state*/, bool /*knockout*/) { return true; }
        virtual void beginTransparencyGroup(GfxState * /*state*/, double * /*bbox*/,
                                            GfxColorSpace * /*blendingColorSpace*/,
                                            bool /*isolated*/, bool /*knockout*/,
                                            bool /*forSoftMask*/);
        virtual void endTransparencyGroup(GfxState * /*state*/);
        virtual void paintTransparencyGroup(GfxState * /*state*/, double * /*bbox*/);
        virtual void setSoftMask(GfxState * /*state*/, double * /*bbox*/, bool /*alpha*/,
                                 Function * /*transferFunc*/, GfxColor * /*backdropColor*/);
        virtual void clearSoftMask(GfxState * /*state*/);

    private:
        pdftoedn::FontEngine& font_engine;
        PdfTM text_tm;
        std::queue<Unicode> actual_text;
        int inline_img_id;

        // non-virtual methods; helpers
        bool process_image_blob(const std::ostringstream& blob, const PdfTM& ctm,
                                const BoundingBox& bbox, const StreamProps& properties,
                                int width, int height,
                                intmax_t& ref_num);
        void build_path_command(GfxState* state, PdfDocPath::Type type,
                                PdfDocPath::EvenOddRule eo_rule = PdfDocPath::EVEN_ODD_RULE_DISABLED);
    };

} // namespace
