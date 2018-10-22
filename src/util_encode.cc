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

#include <iostream>
#include <sstream>
#include <ostream>

#include <png.h>
#include <zlib.h>
#include <poppler/GfxState.h>

#include "image.h"
#include "pdf_error_tracker.h"
#include "util_encode.h"
#include "runtime_options.h"

namespace pdftoedn
{
    namespace util
    {
        namespace encode
        {
            static const char* MODULE = "ImageEncoding";

            struct libpng_error : public std::runtime_error {
                libpng_error(const std::string& what) : runtime_error(what) {}
            };

            // -------------------------------------------------------------------------------
            // image encoding using libpng
            //

            //
            // catch PNG errors instead of them getting set to stdout/stderr
            static void user_error_fn(png_structp png_ptr, png_const_charp error_msg)
            {
                throw libpng_error(error_msg);
            }

            static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
            {
                et.log_warn( ErrorTracker::ERROR_PNG_WARNING, MODULE, warning_msg);
            }

            //
            // write / flush methods to write to an ostream
            static void user_io_write(png_structp png_ptr, png_bytep data, png_size_t length)
            {
                std::ostream* str_p = reinterpret_cast<std::ostream*>( png_get_io_ptr(png_ptr) );

                if (!str_p) {
                    png_error(nullptr, "null stream pointer passed to PNG write()");
                    return;
                }

                if (!data) {
                    png_error(nullptr, "null data pointer passed to PNG write()");
                    return;
                }

                str_p->write((const char*) data, length);
            }

            static void user_io_flush(png_structp png_ptr)
            {
                std::ostream* str_p = reinterpret_cast<std::ostream*>( png_get_io_ptr(png_ptr) );

                if (!str_p) {
                    png_error(nullptr, "null stream pointer passed to PNG io_flush()");
                    return;
                }

                str_p->flush();
            }


            //
            // poppler to libpng translators
            static int poppler_cspace_mode_to_png_type(uint8_t num_pix_comps, GfxColorSpaceMode cspace_mode)
            {
                int png_color_type;

                if (num_pix_comps == 1) {
                    png_color_type = PNG_COLOR_TYPE_PALETTE;
                }
                else
                {
                    switch (cspace_mode)
                    {
                      case csIndexed:
                      case csSeparation:
                          png_color_type = PNG_COLOR_TYPE_PALETTE;
                          break;

                      case csDeviceGray:
                      case csCalGray:
                          png_color_type = PNG_COLOR_TYPE_GRAY;
                          break;

                      case csDeviceCMYK:
                      case csDeviceRGB:
                      case csCalRGB:
                      case csLab:
                      case csICCBased:
                      case csPattern:
                      case csDeviceN:
                      default:
                          png_color_type = PNG_COLOR_TYPE_RGB;
                          break;
                    }
                }

                return png_color_type;
            }


            //
            // copy pixmap data
            static void copy_image_data(png_structp png_ptr,
                                        ImageStream* img_str, uint32_t width, uint32_t height,
                                        uint8_t num_pix_comps, GfxColorSpaceMode cspace_mode, GfxColorSpace* cspace)
            {
                // read the image bytes
                img_str->reset();

                switch (cspace_mode)
                {
                  case csIndexed:
                  case csSeparation:
                      for (size_t y = 0; y < height; y++) {
                          Guchar *pix = img_str->getLine();
                          png_write_rows(png_ptr, &pix, 1);
                      }
                      break;

                  case csDeviceCMYK:
                      {
                          GfxDeviceCMYKColorSpace* cmyk_cs = dynamic_cast<GfxDeviceCMYKColorSpace*>(cspace);

                          if (!cmyk_cs) {
                              std::stringstream err;
                              err << __FUNCTION__ << "() - poppler "
                                  << GfxColorSpace::getColorSpaceModeName(cspace_mode)
                                  << " stream not carrying CMYK color space";
                              throw std::runtime_error(err.str());
                          }

                          // we'll convert cmyk to RGB so only need 3 color chans
                          png_bytep data_row = reinterpret_cast<png_bytep>( png_malloc(png_ptr, 3 * sizeof(png_byte) * width) );
                          if (!data_row) {
                              std::stringstream err;
                              err << __FUNCTION__ << "() - png_malloc failed when handling color space "
                                  << GfxColorSpace::getColorSpaceModeName(cspace_mode);
                              throw std::runtime_error(err.str());
                          }

                          for (size_t y = 0; y < height; ++y)
                          {
                              cmyk_cs->getRGBLine(img_str->getLine(), data_row, width);
                              png_write_rows(png_ptr, &data_row, 1);
                          }

                          png_free(png_ptr, data_row);
                      }
                      break;

                  case csDeviceGray:
                  case csCalGray:
                  case csDeviceRGB:
                  case csCalRGB:
                  case csLab:
                  case csICCBased:
                  case csPattern:
                  case csDeviceN:
                      {
                          Guchar pix[num_pix_comps];

                          png_bytep data_row = reinterpret_cast<png_bytep>( png_malloc(png_ptr, num_pix_comps * sizeof(png_byte) * width) );
                          if (!data_row) {
                              std::stringstream err;
                              err << __FUNCTION__ << "() - png_malloc failed for color space "
                                  << GfxColorSpace::getColorSpaceModeName(cspace_mode);
                              throw std::runtime_error(err.str());
                          }

                          for (size_t y = 0; y < height; ++y)
                          {
                              for (size_t x = 0; x < width * num_pix_comps;)
                              {
                                  if (img_str->getPixel(pix)) {
                                      for (int z = 0; z < num_pix_comps; z++) {
                                          data_row[x++] = pix[z];
                                      }
                                  }
                              }
                              png_write_rows(png_ptr, &data_row, 1);
                          }

                          png_free(png_ptr, data_row);
                      }
                      break;

                  default:
                      std::stringstream err;
                      err << __FUNCTION__ << "() - unhandled color space mode:  "
                          << GfxColorSpace::getColorSpaceModeName(cspace_mode);
                      throw std::runtime_error(err.str());
                }
            }


            //
            // export the data as a PNG onto an ostream - based on:
            //
            //  http://www.linbox.com/ucome.rvt?file=/any/doc_distrib/libgr-2.0.13/png/example.c
            bool encode_image(std::ostream& output, ImageStream* img_str,
                              const StreamProps& properties,
                              GfxImageColorMap *color_map)
            {
                // Create and initialize the png_struct with the desired error handler
                // functions.  If you want to use the default stderr and longjump method,
                // you can supply null for the last three parameters.  We also check that
                // the library version is compatible with the one used at compile time,
                // in case we are using dynamically linked libraries.
                png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                              nullptr,
                                                              user_error_fn,
                                                              user_warning_fn);
                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr, nullptr);
                    return false;
                }

                // use our write & flush functions
                png_set_write_fn(png_ptr,
                                 reinterpret_cast<png_voidp *>(&output),
                                 user_io_write,
                                 user_io_flush);

                // prep some things before we make the libpng calls
                uintmax_t width = properties.bitmap_width();
                uintmax_t height = properties.bitmap_height();
                uint8_t bpp = properties.bitmap_bpp();
                uint8_t num_pix_comps = properties.bitmap_num_pixel_comps();

                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type = poppler_cspace_mode_to_png_type(num_pix_comps, cspace_mode);
                png_colorp palette = nullptr;

#if 0
                std::cerr << "color space mode is: '"
                          << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode)
                          << "', num pixel comps: " << num_pix_comps
                          << ", bpp: " << (int) bpp
                          << std::endl;
#endif

                bool status = true;
                try
                {
                    // Set the image information here.  Width and height
                    // are up to 2^31, bit_depth is one of 1, 2, 4, 8, or
                    // 16, but valid values also depend on the color_type
                    // selected. color_type is one of PNG_COLOR_TYPE_GRAY,
                    // PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE,
                    // PNG_COLOR_TYPE_RGB, or PNG_COLOR_TYPE_RGB_ALPHA.
                    // interlace is either PNG_INTERLACE_NONE or
                    // PNG_INTERLACE_ADAM7, and the compression_type and
                    // filter_type MUST currently be
                    // PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE.
                    //
                    png_set_IHDR(png_ptr, info_ptr, width, height, bpp,
                                 png_type,
                                 PNG_INTERLACE_NONE,
                                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                    if (pdftoedn::options.libpng_use_best_compression()) {
                        png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                    }

                    // allocate palette for indexed-color images
                    if (png_type == PNG_COLOR_TYPE_PALETTE)
                    {
                        int n = 1 << bpp;

                        palette = reinterpret_cast<png_colorp>( png_malloc(png_ptr, n * sizeof (png_color)) );
                        if (!palette) {
                            std::stringstream err;
                            err << __FUNCTION__ << "() - couldn't alloc storage for image palette";
                            throw std::runtime_error(err.str());
                        }

                        Guchar pix;
                        GfxRGB rgb;

                        for (int i = 0; i < n; i++) {
                            pix = (Guchar) i;

                            color_map->getRGB(&pix, &rgb);
                            palette[i].red = colToByte(rgb.r);
                            palette[i].green = colToByte(rgb.g);
                            palette[i].blue = colToByte(rgb.b);
                        }

                        png_set_PLTE(png_ptr, info_ptr, palette, n);
                    }

                    // Write the file header information.
                    png_write_info(png_ptr, info_ptr);

                    // pack pixels into bytes
                    png_set_packing(png_ptr);

                    // swap bits of 1, 2, 4 bit packed pixel formats
                    png_set_packswap(png_ptr);

                    // ready to copy the data - iterate through the lines
                    img_str->reset();
                    copy_image_data(png_ptr, img_str, width, height, num_pix_comps,
                                    cspace_mode, color_map->getColorSpace());

                    // finish writing the rest of the file
                    png_write_end(png_ptr, info_ptr);

                }
                catch (libpng_error& e) {
                    et.log_critical( ErrorTracker::ERROR_PNG_ERROR, MODULE, e.what() );
                    status = false;
                }
                catch (std::exception& e) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, e.what() );
                    status = false;
                }

                // cleanup
                img_str->close();

                // free the palette if allocated
                if (palette) {
                    png_free(png_ptr, palette);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);
                return status;
            }


            //
            // export the data as a PNG RGBA onto an ostream - based on:
            //
            //  http://www.linbox.com/ucome.rvt?file=/any/doc_distrib/libgr-2.0.13/png/example.c
            bool encode_rgba_image(std::ostream& output, ImageStream* img_str, ImageStream* mask_str,
                                   const StreamProps& properties,
                                   GfxImageColorMap *color_map, GfxImageColorMap *mask_color_map,
                                   bool mask_invert)
            {
                // Set the image information here
                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type;

                switch (cspace_mode) {
                  case csDeviceRGB:
                  case csICCBased:
                  case csIndexed:
                      png_type = PNG_COLOR_TYPE_RGB_ALPHA;
                      break;
                  case csDeviceGray:
                      png_type = PNG_COLOR_TYPE_GRAY_ALPHA;
                      break;
                  default:
                      std::stringstream err;
                      err << __FUNCTION__ << " - image unhandled cspace mode ("
                          << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode)
                          << ")";
                      et.log_critical( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, err.str() );
                      return false;
                }

                // Create and initialize the png_struct with the desired error handler
                // functions.
                png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                              nullptr,
                                                              user_error_fn,
                                                              user_warning_fn);
                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr, nullptr);
                    return false;
                }

                // use our write & flush functions
                png_set_write_fn(png_ptr,
                                 reinterpret_cast<png_voidp *>(&output),
                                 user_io_write,
                                 user_io_flush);

                uintmax_t width = properties.bitmap_width();
                uintmax_t height = properties.bitmap_height();
                uint8_t num_pix_comps = properties.bitmap_num_pixel_comps();
                uint8_t bpp = properties.bitmap_bpp();
                uintmax_t mask_width = properties.mask_width();
                uintmax_t mask_height = properties.mask_height();
                uint8_t mask_num_pix_comps = properties.mask_num_pixel_comps();
                png_colorp palette = nullptr;

                png_bytep data_row = nullptr;
                uint8_t* mask_buf = nullptr;
                bool status = true;

                try
                {
                    png_set_IHDR(png_ptr, info_ptr, width, height, bpp,
                                 png_type,
                                 PNG_INTERLACE_NONE,
                                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                    if (pdftoedn::options.libpng_use_best_compression()) {
                        png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                    }

                    // Write the file header information.
                    png_write_info(png_ptr, info_ptr);

                    // pack pixels into bytes
                    png_set_packing(png_ptr);

                    // swap bits of 1, 2, 4 bit packed pixel formats
                    png_set_packswap(png_ptr);

                    // ready to copy image data - combine the image data
                    // (RGB if 3 bpp, Grey if 1) with mask data
                    // (1-channel) to form an RGBA png
                    Guchar pix[num_pix_comps];
                    Guchar* mask_bits;
                    uint32_t o_num_pix_comps = 4; // add 1 for alpha channel

                    data_row = reinterpret_cast<png_bytep>( png_malloc(png_ptr, o_num_pix_comps * sizeof(png_byte) * width) );
                    if (!data_row) {
                        std::stringstream err;
                        err << __FUNCTION__ << "() - couldn't alloc storage for image row buffer";
                        throw std::runtime_error(err.str());
                    }

                    // allocate a buffer for the mask line
                    mask_buf = new uint8_t[mask_width * mask_num_pix_comps];
                    if (!mask_buf) {
                        std::stringstream err;
                        err << __FUNCTION__ << "() - couldn't alloc storage for mask buffer";
                        throw std::runtime_error(err.str());
                    }

                    img_str->reset();
                    mask_str->reset();

                    // process image line by line
                    for (size_t y = 0; y < height; ++y)
                    {
                        mask_bits = mask_str->getLine();

                        // poppler returns some masked images w/ a color map
                        if (mask_color_map) {
                            mask_color_map->getGrayLine(mask_bits, mask_buf, mask_width);
                        }
                        else {
                            // no cmap - just read the value and invert if needed
                            for (size_t mx = 0; mx < mask_width; mx++) {
                                bool bit = (mask_invert ? !mask_bits[mx] : mask_bits[mx]);
                                mask_buf[mx] = (bit ? 0 : 0xff);
                            }
                        }

                        // now process image data
                        for (size_t x = 0, mx = 0; x < width * o_num_pix_comps;)
                        {
                            if (img_str->getPixel(pix)) {
                                if (num_pix_comps > 1) {
                                    GfxRGB rgb;
                                    color_map->getRGB(pix, &rgb);
                                    data_row[x++] = colToByte(rgb.r);
                                    data_row[x++] = colToByte(rgb.g);
                                    data_row[x++] = colToByte(rgb.b);
                                } else {
                                    GfxGray g;
                                    color_map->getGray(pix, &g);
                                    uint8_t c = colToByte(g);

                                    data_row[x++] = c;
                                    data_row[x++] = c;
                                    data_row[x++] = c;
                                }
                            }

                            // and set alpha to the mask value
                            data_row[x++] = mask_buf[mx++];
                        }
                        png_write_rows(png_ptr, &data_row, 1);
                    }

                    png_write_end(png_ptr, info_ptr);
                }
                catch (libpng_error& e) {
                    et.log_critical( ErrorTracker::ERROR_PNG_ERROR, MODULE, e.what() );
                    status = false;
                }
                catch (std::exception& e) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, e.what() );
                    status = false;
                }

                // cleanup
                mask_str->close();
                img_str->close();

                delete [] mask_buf;
                if (data_row) {
                    png_free(png_ptr, data_row);
                }

                // free the palette if allocated
                if (palette) {
                    png_free(png_ptr, palette);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);
                return status;
            }


            //
            // export a mask onto a stream as a PNG w/ transparency.
            // Yeah, lots of replicated steps from encode_image.. :(
            bool encode_mask(std::ostream& output, ImageStream* img_str, const StreamProps& properties)
            {
                png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                              nullptr,
                                                              user_error_fn,
                                                              user_warning_fn);
                if (!png_ptr) {
                    return false;
                }

                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr, nullptr);
                    return false;
                }

                png_set_write_fn(png_ptr,
                                 reinterpret_cast<png_voidp *>(&output),
                                 user_io_write,
                                 user_io_flush);

                uintmax_t width = properties.mask_width();
                uintmax_t height = properties.mask_height();
                png_colorp palette = nullptr;
                uint8_t* data_row = nullptr;
                bool status = true;

                try
                {
                    // 2-palette entries.. one is set to transparent (0x00)
                    png_byte transp[2] = { 0xff, 0x00 };
                    png_set_tRNS(png_ptr, info_ptr, transp, 2, nullptr);

                    // save the header chunk
                    png_set_IHDR(png_ptr, info_ptr, width, height, 1,
                                 PNG_COLOR_TYPE_PALETTE,
                                 PNG_INTERLACE_NONE,
                                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                    if (pdftoedn::options.libpng_use_best_compression()) {
                        png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                    }

                    // set the palette
                    palette = reinterpret_cast<png_colorp>( png_malloc(png_ptr, 2 * sizeof(png_colorp)) );
                    if (!palette) {
                        std::stringstream err;
                        err << __FUNCTION__ << "() - couldn't alloc storage for image palette";
                        throw std::runtime_error(err.str());
                    }

                    palette[0].red   = properties.mask_fill_color().red();
                    palette[0].green = properties.mask_fill_color().green();
                    palette[0].blue  = properties.mask_fill_color().blue();

                    // by default set the 2nd color to white.. leptonica
                    // does not preserve transparency so this should make
                    // it match white background at least
                    palette[1].red   = palette[1].green = palette[1].blue = 0xff;
                    png_set_PLTE(png_ptr, info_ptr, palette, 2);

                    // Write the file header information.
                    png_write_info(png_ptr, info_ptr);

                    // pack pixels into bytes
                    png_set_packing(png_ptr);
                    // swap bits of 1, 2, 4 bit packed pixel formats - we
                    // don't need this but I've found some images in PDFs
                    // that cause a segfault when this is disabled. Ugh.
                    png_set_packswap(png_ptr);

                    data_row = new uint8_t[ width ];
                    if (!data_row) {
                        std::stringstream err;
                        err << __FUNCTION__ << "() - couldn't alloc storage for image row buffer";
                        throw std::runtime_error(err.str());
                    }

                    // reset the poppler image stream
                    img_str->reset();

                    for (size_t y = 0; y < height; y++) {
                        Guchar* pix = img_str->getLine();
                        for (size_t x = 0; x < width; x++) {
                            data_row[x] = (properties.mask_is_inverted() ? !pix[x] : pix[x]);
                        }
                        png_write_rows(png_ptr, &data_row, 1);
                    }

                    png_write_end(png_ptr, info_ptr);
                }
                catch (libpng_error& e) {
                    et.log_critical( ErrorTracker::ERROR_PNG_ERROR, MODULE, e.what() );
                    status = false;
                }
                catch (std::exception& e) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, e.what() );
                    status = false;
                }

                // cleanup
                img_str->close();

                delete [] data_row;

                if (palette) {
                    png_free(png_ptr, palette);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);

                return status;
            }


#if 0
            //
            // export the data to PNG, forcing grayscale output instead of palletized
            //
            bool encode_grey_image(std::ostream& output, ImageStream* img_str,
                                   const StreamProps& properties,
                                   GfxImageColorMap *color_map)
            {
                png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                              nullptr,
                                                              user_error_fn,
                                                              user_warning_fn);
                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr, nullptr);
                    return false;
                }

                png_set_write_fn(png_ptr,
                                 (png_voidp *)&output,
                                 user_io_write,
                                 user_io_flush);

                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type = poppler_cspace_mode_to_png_type(num_pix_comps, cspace_mode);

#if 0
                std::cerr << "color space mode is: '" << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode)
                          << "', num pixel comps: " << num_pix_comps
                          << ", bpp: " << (int) bpp
                          << ", w: " << width << ", h: " << height
                          << std::endl;
#endif

                png_bytep data_row = nullptr;
                bool status = true;
                try
                {
                    png_set_IHDR(png_ptr, info_ptr, width, height, bpp, // change bpp to 1 / 8-bit if needed
                                 PNG_COLOR_TYPE_GRAY, // forcing GRAY
                                 PNG_INTERLACE_NONE,
                                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

#ifdef MOTO_USE_BEST_COMPRESSION
                    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
#endif

                    // Write the file header information.
                    png_write_info(png_ptr, info_ptr);

                    // pack pixels into bytes
                    png_set_packing(png_ptr);

                    // swap bits of 1, 2, 4 bit packed pixel formats
                    png_set_packswap(png_ptr);

                    // ready to copy the data - iterate through the lines
                    data_row = reinterpret_cast<png_bytep>( png_malloc(png_ptr, num_pix_comps * sizeof(png_byte) * width) );
                    if (!data_row) {
                        std::stringstream err;
                        err << __FUNCTION__ << " - png_malloc failed for data row";
                        throw std::runtime_error(err.str());
                    }

                    // write the data, row by row
                    img_str->reset();

                    GfxGray gray;
                    for (size_t y = 0; y < height; y++) {
                        Guchar* pix = img_str->getLine();

                        for (size_t x = 0; x < width; x++) {
                            color_map->getGray(&pix[x], &gray);
                            data_row[x] = reinterpret_cast<png_byte>( colToByte(gray) );
                        }
                        png_write_rows(png_ptr, &data_row, 1);
                    }
                    img_str->close();

                    png_write_end(png_ptr, info_ptr);
                }
                catch (libpng_error& e) {
                    et.log_critical( ErrorTracker::ERROR_PNG_ERROR, MODULE, e.what() );
                    status = false;
                }
                catch (std::exception& e) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, e.what() );
                    status = false;
                }

                if (data_row) {
                    png_free(png_ptr, data_row);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);
                return status;
            }
#endif

        } // namespace encode

    } // namespace util

} // namespace
