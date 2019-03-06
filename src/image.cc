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
#include <poppler/Stream.h>

#include "image.h"
#include "pdf_links.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    // static initializers
    static const pdftoedn::Symbol SYMBOL_SP_CMD_TYPE         = "cmd_type";
    static const pdftoedn::Symbol SYMBOL_SP_CMD_TYPES[]      = {
        "mask",
        "image",
        "soft_masked_image",
        "masked_image"
    };
    static const pdftoedn::Symbol SYMBOL_SP_STR              = "stream";
    static const pdftoedn::Symbol SYMBOL_SP_STR_MASK         = "mask_stream";

    static const pdftoedn::Symbol SYMBOL_SP_PIX_COMP         = "pixel_comps";
    static const pdftoedn::Symbol SYMBOL_SP_BPP              = "bpp";
    static const pdftoedn::Symbol SYMBOL_SP_STR_KIND         = "kind";
    static const pdftoedn::Symbol SYMBOL_SP_STREAM_KINDS[]   = {
        "UNDEFINED",
        "file",
        "cached_file",
        "asciihex",
        "ascii85",
        "lzw",
        "run_length",
        "ccittfax",
        "dct",
        "flate",
        "jbig2",
        "jpx"
    };
    static const pdftoedn::Symbol SYMBOL_SP_INTERPOLATE      = "interpolate";
    static const pdftoedn::Symbol SYMBOL_SP_INLINED          = "inlined";
    static const pdftoedn::Symbol SYMBOL_SP_UPSIDE_DOWN      = "upside_down";
    static const pdftoedn::Symbol SYMBOL_SP_INVERT           = "invert";
    static const pdftoedn::Symbol SYMBOL_SP_FILL_COLOR       = "fill";
    static const pdftoedn::Symbol SYMBOL_SP_FILL_CSPACE      = "fill_cs";
    static const pdftoedn::Symbol SYMBOL_SP_COLOR_SPACE_MODES[] = {
        "device_gray",
        "cal_gray",
        "device_rgb",
        "cal_rgb",
        "device_cmyk",
        "lab",
        "icc_based",
        "indexed",
        "separation",
        "device_n",
        "pattern"
    };

    const pdftoedn::Symbol ImageData::SYMBOL_ID               = "id";
    const pdftoedn::Symbol ImageData::SYMBOL_INSTANCE_COUNT   = "instances";
    const pdftoedn::Symbol ImageData::SYMBOL_WIDTH            = "width";
    const pdftoedn::Symbol ImageData::SYMBOL_HEIGHT           = "height";
    const pdftoedn::Symbol ImageData::SYMBOL_MD5              = "md5";
    const pdftoedn::Symbol ImageData::SYMBOL_IMAGE_PATH       = "image_path";
    const pdftoedn::Symbol ImageData::SYMBOL_STREAM_PROPS     = "props";

    const pdftoedn::Symbol PdfImage::SYMBOL_TYPE_IMAGE        = "image";

    // =============================================
    // PDF image stream properties
    //
    StreamProps::BitmapAttribs::BitmapAttribs(StreamKind strKind, uint32_t w, uint32_t h,
                                              uint8_t num_comps, uint8_t bpp, bool interp) :
        stream_type(util::poppler_stream_type_to_edn(strKind)), width(w), height(h),
        num_pixel_comps(num_comps), bits_per_pixel(bpp),
        interpolate(interp)
    {}


    StreamProps::MaskAttribs::MaskAttribs(StreamKind strKind, uintmax_t w, uintmax_t h,
                                          uint8_t pix_comps, uint8_t bpp,
                                          bool interp, bool m_inverted) :
        stream_type(util::poppler_stream_type_to_edn(strKind)),
        width(w), height(h),
        num_pixel_comps(pix_comps), bits_per_pixel(bpp),
        interpolate(interp), invert(m_inverted)
    { }

    StreamProps::MaskAttribs::MaskAttribs(StreamKind strKind, uintmax_t w, uintmax_t h,
                                          uint8_t pix_comps, uint8_t bpp, bool interp, bool m_inverted,
                                          const RGBColor& m_fill, GfxColorSpaceMode fill_cs_mode) :
        stream_type(util::poppler_stream_type_to_edn(strKind)),
        width(w), height(h),
        num_pixel_comps(pix_comps), bits_per_pixel(bpp),
        interpolate(interp), invert(m_inverted),
        fill(m_fill), fill_cspace_mode(fill_cs_mode)
    { }

    //
    // EDN output
    std::ostream& StreamProps::to_edn(std::ostream& o) const
    {
        util::edn::Hash props_h(5);

        // mostly for debug / info
        props_h.push( SYMBOL_SP_CMD_TYPE, SYMBOL_SP_CMD_TYPES[ type ] );

        // if it's IMAGE, SOFT_MASKED, or MASKED_IMAGE, it carries image stream data
        if (type != MASK) {
            util::edn::Hash image_str_h(6);

            image_str_h.push( SYMBOL_SP_STR_KIND, SYMBOL_SP_STREAM_KINDS[ bitmap.stream_type ] );
            image_str_h.push( ImageData::SYMBOL_WIDTH, bitmap.width );
            image_str_h.push( ImageData::SYMBOL_HEIGHT, bitmap.height );
            image_str_h.push( SYMBOL_SP_PIX_COMP, bitmap.num_pixel_comps );
            image_str_h.push( SYMBOL_SP_BPP, bitmap.bits_per_pixel );
            if (bitmap.interpolate) {
                image_str_h.push( SYMBOL_SP_INTERPOLATE, true );
            }

            props_h.push( SYMBOL_SP_STR, image_str_h );
        }

        // MASK, SOFT_MASK, or MASKED_IMAGE, it carries some mask data
        if (type != IMAGE) {
            util::edn::Hash mask_str_h(9);

            mask_str_h.push( SYMBOL_SP_STR_KIND, SYMBOL_SP_STREAM_KINDS[ mask.stream_type ] );
            mask_str_h.push( ImageData::SYMBOL_WIDTH, mask.width );
            mask_str_h.push( ImageData::SYMBOL_HEIGHT, mask.height );
            mask_str_h.push( SYMBOL_SP_PIX_COMP, mask.num_pixel_comps );
            mask_str_h.push( SYMBOL_SP_BPP, mask.bits_per_pixel );

            if (type == MASK) {
                mask_str_h.push( SYMBOL_SP_FILL_COLOR, &mask.fill );
                mask_str_h.push( SYMBOL_SP_FILL_CSPACE, SYMBOL_SP_COLOR_SPACE_MODES[ mask.fill_cspace_mode ] );
            }

            if (mask.interpolate) {
                mask_str_h.push( SYMBOL_SP_INTERPOLATE, true );
            }
            if (mask.invert) {
                mask_str_h.push( SYMBOL_SP_INVERT, true );
            }

            // ":stream" if it's a mask; ":mask_stream" otherwise
            props_h.push( (type != MASK ? SYMBOL_SP_STR_MASK : SYMBOL_SP_STR), mask_str_h );
        }

        if (inlined) {
            props_h.push( SYMBOL_SP_INLINED, true );
        }
        if (upside_down) {
            props_h.push( SYMBOL_SP_UPSIDE_DOWN, true );
        }
        o << props_h;
        return o;
    }


    // =============================================
    // PDF image blob
    //
    std::ostream& ImageData::to_edn(std::ostream& o) const
    {
        util::edn::Hash image_h(7);

        image_h.push( BoundingBox::SYMBOL, bbox );
        image_h.push( SYMBOL_INSTANCE_COUNT, ref_count );
        image_h.push( SYMBOL_WIDTH, width );
        image_h.push( SYMBOL_HEIGHT, height );
        image_h.push( SYMBOL_MD5, blob_md5 );
        image_h.push( SYMBOL_STREAM_PROPS, &stream_props );
        image_h.push( SYMBOL_IMAGE_PATH, file_name );

        o << image_h;
        return o;

    }


    // =============================================
    // PDF image class
    //
    std::ostream& PdfImage::to_edn(std::ostream& o) const
    {
        // image fields
        util::edn::Hash image_h(5);
        image_h.push( SYMBOL_TYPE,               cmd );
        image_h.push( BoundingBox::SYMBOL,       bbox );
        image_h.push( ImageData::SYMBOL_ID,      res_id );

        if (clip_path_id != -1) {
            image_h.push( PdfDocPath::SYMBOL_CLIP_TO, clip_path_id );
        }
        if (link_idx != -1) {
            image_h.push( PdfLink::SYMBOL_LINK_IDX, link_idx );
        }
        o << image_h;
        return o;
    }

} // namespace
