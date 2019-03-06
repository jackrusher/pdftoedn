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

#include <ostream>
#include <string>

#include "base_types.h"
#include "color.h"
#include "graphics.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // properties of an image stream
    //  - primarily for debug / extra info
    //
    class StreamProps : public gemable {
    public:
        enum cmd_type_e { MASK, IMAGE, SOFT_MASKED_IMAGE, MASKED_IMAGE };

        enum stream_type_e {
            STREAM_UNDEF,
            STREAM_FILE,
            STREAM_CACHED_FILE,
            STREAM_ASCII_HEX,
            STREAM_ASCII85,
            STREAM_LZW,
            STREAM_RUN_LENGTH,
            STREAM_CCITT_FAX,
            STREAM_DCT,
            STREAM_FLATE,
            STREAM_JBIG2,
            STREAM_JPX,

            STREAM_TYPE_COUNT
        };

        // masks
        StreamProps(StreamKind strKind, uint32_t width, uint32_t height,
                    const RGBColor& fill, GfxColorSpaceMode fill_cs_mode,
                    bool interpolate, bool mask_upside_down, bool str_inlined,
                    bool invert) :
            type(MASK),
            mask(strKind, width, height, 1, 1, interpolate, invert, fill, fill_cs_mode),
            inlined(str_inlined),
            upside_down(mask_upside_down)
        { }
        // images
        StreamProps(StreamKind strKind, uint32_t width, uint32_t height,
                    uint8_t num_pixel_comps, uint8_t bits_per_pixel,
                    bool interpolate, bool img_upside_down, bool str_inlined) :
            type(IMAGE),
            bitmap(strKind, width, height, num_pixel_comps, bits_per_pixel, interpolate),
            inlined(str_inlined),
            upside_down(img_upside_down)
        { }
        // soft masked images
        StreamProps(StreamKind strKind, uint32_t width, uint32_t height,
                    GfxImageColorMap* colorMap, bool interpolate,
                    StreamKind mskStrKind, uint32_t mask_width, uint32_t mask_height,
                    GfxImageColorMap* mskColorMap, bool mask_interpolate,
                    bool img_upside_down) :
            type(SOFT_MASKED_IMAGE),
            bitmap(strKind, width, height,
                   colorMap->getNumPixelComps(), colorMap->getBits(), interpolate),
            mask(mskStrKind, mask_width, mask_height,
                 mskColorMap->getNumPixelComps(), colorMap->getBits(), mask_interpolate,
                 false),
            inlined(false),
            upside_down(img_upside_down)
        { }
        // masked images
        StreamProps(StreamKind strKind, uint32_t width, uint32_t height,
                    GfxImageColorMap* colorMap, bool interpolate,
                    StreamKind mskStrKind, uint32_t mask_width, uint32_t mask_height,
                    bool mask_interpolate, bool mask_invert,
                    bool img_upside_down) :
            type(MASKED_IMAGE),
            bitmap(strKind, width, height,
                   colorMap->getNumPixelComps(), colorMap->getBits(), interpolate),
            mask(mskStrKind, mask_width, mask_height, 1, 1, mask_interpolate, mask_invert),
            inlined(false),
            upside_down(img_upside_down)
        { }

        // query methods
        bool is_inlined() const { return inlined; }
        bool mask_is_inverted() const { return mask.invert; }
        const RGBColor& mask_fill_color() const { return mask.fill; }

        uintmax_t mask_width() const { return mask.width; }
        uintmax_t mask_height() const { return mask.height; }
        uintmax_t bitmap_width() const { return bitmap.width; }
        uintmax_t bitmap_height() const { return bitmap.height; }

        uint8_t mask_num_pixel_comps() const { return mask.num_pixel_comps; }
        uint8_t bitmap_num_pixel_comps() const { return bitmap.num_pixel_comps; }

        uint8_t mask_bpp() const { return mask.bits_per_pixel; }
        uint8_t bitmap_bpp() const { return bitmap.bits_per_pixel; }

        virtual std::ostream& to_edn(std::ostream& o) const;

    private:
        // streams in a PDF can be straight bitmaps, mask, or masked
        // bitmaps. This carries properties for both in case it's
        // needed
        struct BitmapAttribs {
            BitmapAttribs() : stream_type(STREAM_UNDEF) { }
            BitmapAttribs(StreamKind strKind, uint32_t w, uint32_t h,
                          uint8_t num_comps, uint8_t bpp, bool interp);

            stream_type_e stream_type;
            uintmax_t width, height;
            uint8_t num_pixel_comps;
            uint8_t bits_per_pixel;
            bool interpolate;
        };

        struct MaskAttribs {
            MaskAttribs() : stream_type(STREAM_UNDEF), interpolate(false), invert(false) { }
            MaskAttribs(StreamKind strKind, uintmax_t w, uintmax_t h,
                        uint8_t pix_comps, uint8_t bpp, bool interp, bool m_inverted);
            MaskAttribs(StreamKind strKind, uintmax_t w, uintmax_t h,
                        uint8_t pix_comps, uint8_t bpp, bool interp, bool m_inverted,
                        const RGBColor& m_fill, GfxColorSpaceMode fill_cs_mode);

            stream_type_e stream_type;
            uintmax_t width, height;
            uint8_t num_pixel_comps;
            uint8_t bits_per_pixel;
            bool interpolate;
            bool invert;
            RGBColor fill;
            GfxColorSpaceMode fill_cspace_mode;
        };

        cmd_type_e type;
        BitmapAttribs bitmap;
        MaskAttribs mask;
        bool inlined;
        bool upside_down;

        friend class ImageData;
    };


    // -------------------------------------------------------
    // pdf image data blob management - for caching image data that
    // may be used several times in a doc
    //
    //
    class ImageData : public gemable
    {
    public:
        // constructor
        ImageData(intmax_t resource_id,
                  const BoundingBox& b,
                  int img_width, int img_height,
                  const StreamProps& props,
                  const std::string& img_data_md5,
                  const std::string& filename) :
            res_id(resource_id),
            bbox(b),
            width(img_width), height(img_height),
            stream_props(props),
            file_name(filename),
            blob_md5(img_data_md5),
            ref_count(1)
        { }

        // accessors
        intmax_t id() const { return res_id; }
        const std::string& md5() const { return blob_md5; }
        void ref() const { ref_count++; }
        bool equals(int id) const { return (res_id == id); }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_ID;
        static const pdftoedn::Symbol SYMBOL_INSTANCE_COUNT;
        static const pdftoedn::Symbol SYMBOL_WIDTH;
        static const pdftoedn::Symbol SYMBOL_HEIGHT;
        static const pdftoedn::Symbol SYMBOL_MD5;
        static const pdftoedn::Symbol SYMBOL_IMAGE_PATH;
        static const pdftoedn::Symbol SYMBOL_STREAM_PROPS;

        // predicate for sorting a set of images
        struct lt {
            bool operator()(const pdftoedn::ImageData* const i1, const pdftoedn::ImageData* const i2) const {
                return (i1->id() < i2->id());
            }
        };

    private:
        intmax_t res_id;
        BoundingBox bbox;
        uintmax_t width;
        uintmax_t height;
        StreamProps stream_props;
        std::string file_name;
        std::string blob_md5;
        mutable uintmax_t ref_count;
    };


    // -------------------------------------------------------
    // pdf image metadata
    //
    //
    class PdfImage : public PdfGfxCmd {
    public:
        // constructor
        PdfImage(intmax_t resource_id, // index in image table
                 const BoundingBox& b,
                 intmax_t link_index) :
            PdfGfxCmd(SYMBOL_TYPE_IMAGE),
            res_id(resource_id),
            clip_path_id(-1),
            link_idx(link_index),
            bbox(b)
        {  }

        void set_clip_id(intmax_t clip_id) { clip_path_id = clip_id; }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_TYPE_IMAGE;

    private:
        intmax_t res_id;
        intmax_t clip_path_id;
        intmax_t link_idx;
        BoundingBox bbox;
    };

}
