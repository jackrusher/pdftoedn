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
        // Does this device use drawChar() or drawString()?
        virtual bool useDrawChar() override { return true; }

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual bool interpretType3Chars() override { return true; }
        virtual bool needNonText() override { return true; }
        virtual bool useTilingPatternFill() override { return true; }

        //----- initialization and control
        virtual void startPage(int pageNum, GfxState *state, XRef *xref) override;
        virtual void endPage() override;

        //----- update graphics state
        virtual void saveState(GfxState* state) override;
        virtual void restoreState(GfxState* state) override;

        virtual void updateAll(GfxState *state) override;
        virtual void updateLineDash(GfxState* state) override;
        virtual void updateFlatness(GfxState * /*state*/) override {}
        virtual void updateLineJoin(GfxState *state) override;
        virtual void updateLineCap(GfxState *state) override;
        virtual void updateMiterLimit(GfxState *state) override;
        virtual void updateLineWidth(GfxState *state) override;
        virtual void updateAlphaIsShape(GfxState *state) override;
        virtual void updateTextKnockout(GfxState *state) override;
        virtual void updateFillColorSpace(GfxState * /*state*/) override {}
        virtual void updateStrokeColorSpace(GfxState * /*state*/) override {}
        virtual void updateFillColor(GfxState *state) override;
        virtual void updateStrokeColor(GfxState *state) override;
        virtual void updateBlendMode(GfxState *state) override;
        virtual void updateFillOpacity(GfxState *state) override;
        virtual void updateStrokeOpacity(GfxState *state) override;
        virtual void updateFillOverprint(GfxState *state) override;
        virtual void updateStrokeOverprint(GfxState *state) override;
        virtual void updateOverprintMode(GfxState* state) override;
        virtual void updateTransfer(GfxState * /*state*/) override {}
        virtual void updateFillColorStop(GfxState * /*state*/, double /*offset*/) override {}

        //----- text state
        virtual void updateFont(GfxState* state) override;
        virtual void updateTextShift(GfxState* state, double shift) override;

        //----- text drawing
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen) override;
        virtual void beginActualText(GfxState* state, const GooString *text ) override;
        virtual void endActualText(GfxState * /*state*/) override { }

        //----- paths
        virtual void stroke(GfxState *state) override;
        virtual void fill(GfxState *state) override;
        virtual void eoFill(GfxState *state) override;

        //----- patterns
        virtual bool tilingPatternFill(GfxState * /*state*/, Gfx * /*gfx*/, Catalog * /*cat*/, Object * /*str*/,
                                       const double * /*pmat*/, int /*paintType*/, int /*tilingType*/, Dict * /*resDict*/,
                                       const double * /*mat*/, const double * /*bbox*/,
                                       int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
                                       double /*xStep*/, double /*yStep*/) override;

        //----- clipping
        virtual void clip(GfxState *state) override;
        virtual void eoClip(GfxState *state) override;
        virtual void clipToStrokePath(GfxState *state) override;

        //----- images
        virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                                   int width, int height, bool invert, bool interpolate,
                                   bool inlineImg) override;
        virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GfxImageColorMap *colorMap,
                               bool interpolate, int *maskColors, bool inlineImg) override;

        virtual void setSoftMaskFromImageMask(GfxState *state,
                                              Object *ref, Stream *str,
                                              int width, int height, bool invert,
                                              bool inlineImg, double *baseMatrix) override;
        virtual void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix) override;
        virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
                                     int width, int height,
                                     GfxImageColorMap *colorMap, bool interpolate,
                                     Stream *maskStr, int maskWidth, int maskHeight,
                                     bool maskInvert, bool maskInterpolate) override;
        virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                                         int width, int height,
                                         GfxImageColorMap *colorMap,
                                         bool interpolate,
                                         Stream *maskStr,
                                         int maskWidth, int maskHeight,
                                         GfxImageColorMap *maskColorMap,
                                         bool maskInterpolate) override;

        //----- grouping operators
        virtual void endMarkedContent(GfxState *state) override;
        virtual void beginMarkedContent(const char *name, Dict *properties) override;
        virtual void markPoint(const char *name) override;
        virtual void markPoint(const char *name, Dict *properties) override;

        //----- transparency groups and soft masks
        virtual bool checkTransparencyGroup(GfxState * /*state*/, bool /*knockout*/) override { return true; }
        virtual void beginTransparencyGroup(GfxState * /*state*/, const double * /*bbox*/,
                                            GfxColorSpace * /*blendingColorSpace*/,
                                            bool /*isolated*/, bool /*knockout*/,
                                            bool /*forSoftMask*/) override;
        virtual void endTransparencyGroup(GfxState * /*state*/) override;
        virtual void paintTransparencyGroup(GfxState * /*state*/, const double * /*bbox*/) override;
        virtual void setSoftMask(GfxState * /*state*/, const double * /*bbox*/, bool /*alpha*/,
                                 Function * /*transferFunc*/, GfxColor * /*backdropColor*/) override;
        virtual void clearSoftMask(GfxState * /*state*/) override;

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
