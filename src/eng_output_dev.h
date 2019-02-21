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

#include <cmath>

#include <poppler/OutputDev.h>
#include <poppler/GfxState.h>
#include <poppler/Link.h>

namespace pdftoedn
{
    class PdfPage;

    //------------------------------------------------------------------------
    // pdftoedn::EngOutputDev - base class for all our output devices
    //------------------------------------------------------------------------
    class EngOutputDev : public ::OutputDev {
    public:
        EngOutputDev(Catalog* doc_cat) :
            catalog(doc_cat), pg_data(nullptr) { }
        virtual ~EngOutputDev();

        // skip anything larger than 10 inches
        static bool state_font_size_not_sane(GfxState* state) {
            return (state->getTransformedFontSize() > 10 * (state->getHDPI() + state->getVDPI()));
        }

        static double get_transformed_font_size(GfxState* state) {
            return (std::ceil(state->getTransformedFontSize() * 100) / 100);
        }

        static int adjust_page_dim(int dim);

        // returns the collected data after displayPage has been
        // called to process a page
        const PdfPage* page_data() const { return pg_data; }

        // some default values
        // Does this device use upside-down coordinates?
        // (Upside-down means (0,0) is the top left corner of the page.)
        virtual bool upsideDown() override { return true; }
        virtual bool useDrawChar() override { return false; }
        virtual bool interpretType3Chars() override { return false; }
        virtual bool needNonText() override { return false; }
        virtual bool needCharCount() override { return false; }
        virtual bool useTilingPatternFill() override { return false; }

        // force these methods to be NO-OPS unless defined by the subclasses
        virtual bool beginType3Char(GfxState * /*state*/, double /*x*/, double /*y*/,
                                    double /*dx*/, double /*dy*/,
                                    CharCode /*code*/, Unicode * /*u*/, int /*uLen*/) override
        { return false; }

        virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                                   int width, int height, bool invert, bool interpolate,
                                   bool inlineImg) override {}
        virtual void setSoftMaskFromImageMask(GfxState *state,
                                              Object *ref, Stream *str,
                                              int width, int height, bool invert,
                                              bool inlineImg, double *baseMatrix) override {}
        virtual void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix) override {}
        virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GfxImageColorMap *colorMap,
                               bool interpolate, int *maskColors, bool inlineImg) override {}
        virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
                                     int width, int height,
                                     GfxImageColorMap *colorMap, bool interpolate,
                                     Stream *maskStr, int maskWidth, int maskHeight,
                                     bool maskInvert, bool maskInterpolate) override {}
        virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                                         int width, int height,
                                         GfxImageColorMap *colorMap,
                                         bool interpolate,
                                         Stream *maskStr,
                                         int maskWidth, int maskHeight,
                                         GfxImageColorMap *maskColorMap,
                                         bool maskInterpolate) override {}
        virtual void beginMarkedContent(const char *name, Dict *properties) override {}
        virtual void markPoint(const char *name) override {}
        virtual void markPoint(const char *name, Dict *properties) override {}

    protected:
        Catalog* catalog;
        pdftoedn::PdfPage* pg_data;

        void process_page_links(int page_num);
        void create_annot_link(AnnotLink *link);
        uintmax_t get_dest_goto_page(LinkDest* dest) const;
    };

} // namespace
