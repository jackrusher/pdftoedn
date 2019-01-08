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

#include <string>
#include <iostream>
#include <fstream>
#include <clocale>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <poppler/GlobalParams.h>
#include <poppler/Error.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "base_types.h"
#include "pdf_error_tracker.h"
#include "pdf_reader.h"
#include "runtime_options.h"
#include "font_maps.h"
#include "util_edn.h"
#include "util_fs.h"
#include "util_xform.h"
#include "util_versions.h"


namespace pdftoedn {

    // task-level error handler for poppler errors
    pdftoedn::ErrorTracker et;

    // run-time options passed as args
    pdftoedn::Options options;

    // process-wide font maps
    pdftoedn::DocFontMaps doc_font_maps;

} // namespace

std::ostream& progversion(std::ostream& o, const char* prog_name)
{
    o << boost::filesystem::basename(prog_name) << " "
      << PDFTOEDN_VERSION << std::endl;
    return o;
}


int main(int argc, char** argv)
{
    // pass things back as utf-8
    if (!std::setlocale( LC_ALL, "" )) {
        std::cout << "Error setting locale" << std::endl;
        return pdftoedn::ErrorTracker::CODE_INIT_ERROR;
    }

    // parse the options
    pdftoedn::Options::Flags flags = { false };
    std::string pdf_filename, pdf_owner_password, pdf_user_password, edn_output_filename, font_map_file;
    intmax_t page_number = -1;

    try
    {
        namespace po = boost::program_options;
        po::options_description desc("Options");
        desc.add_options()
            ("help,h",
             "Display this message.")
            ("version,v",
             "Display version information and exit.")
            ("show_font_map_list,F",
             "Display the configured font substitution list and exit. Use with -m flag to see resulting merged list.")
            ("use_page_crop_box,a", po::bool_switch(&flags.use_page_crop_box),
             "Use page crop box instead of media box when reading page content.")
            ("debug_meta,D",        po::bool_switch(&flags.include_debug_info),
             "Include additional debug metadata in output.")
            ("force,f",             po::bool_switch(&flags.force_output_write),
             "Overwrite output file if it exists.")
            ("invisible_text,i",    po::bool_switch(&flags.include_invisible_text),
             "Include invisible text in output (for use with OCR'd documents).")
            ("write_doc_edn_only,d", po::bool_switch(&flags.edn_output_only),
             "Write only EDN output (omit writing image blobs to disk).")
            ("links_only,L",        po::bool_switch(&flags.link_output_only),
             "Extract only link data.")
            ("text_only,T",         po::bool_switch(&flags.text_output_only),
             "Extract only text data.")
            ("graphics_only,G",     po::bool_switch(&flags.gfx_output_only),
             "Extract only graphics data.")
            ("omit_outline,O",      po::bool_switch(&flags.omit_outline),
             "Don't extract outline data.")
            ("font_map_file,m",     po::value<std::string>(&font_map_file),
             "JSON font mapping configuration file to use for this run.")
            ("page_number,p",       po::value<intmax_t>(&page_number),
             "Extract data for only this page.")
            ("owner_password,t",    po::value<std::string>(&pdf_owner_password),
             "PDF owner password if document is encrypted.")
            ("user_password,u",     po::value<std::string>(&pdf_user_password),
             "PDF user password if document is encrypted.")
            ("output_file,o",       po::value<std::string>(&edn_output_filename)->required(),
             "REQUIRED: Destination file (.edn) to write output to.")
            ("filename",            po::value<std::string>(&pdf_filename)->required(),
             "REQUIRED: PDF document to process (--filename flag is optional when the arg is passed last).")
            ;

        po::positional_options_description po_desc;
        po_desc.add("filename", 1);

        po::variables_map vm;

        try
        {
            po::store(po::command_line_parser(argc, argv).options(desc).positional(po_desc).run(), vm);

            if (vm.count("help")) {
                progversion(std::cerr, argv[0]) << desc << std::endl;
                return pdftoedn::ErrorTracker::CODE_RUNTIME_OK;
            }
            if (vm.count("version")) {
                progversion(std::cerr, argv[0]) << "Linked libraries:" << std::endl
                                                << pdftoedn::util::version::info();
                return pdftoedn::ErrorTracker::CODE_RUNTIME_OK;
            }
            if (vm.count("show_font_map_list")) {
                // if one is given, load the font map config & dump
                // the font map list - NOTE: if the -m flag is not
                // passed, font_map_file is "" so only the default
                // map is loaded
                if (vm.count("font_map_file")) {
                    font_map_file = vm["font_map_file"].as<std::string>();
                }
                pdftoedn::options = pdftoedn::Options(font_map_file);
                std::cout << pdftoedn::doc_font_maps << std::endl;
                return pdftoedn::ErrorTracker::CODE_RUNTIME_OK;
            }
            if (vm.count("page_number")) {
                intmax_t pg = vm["page_number"].as<intmax_t>();
                if (pg < 0) {
                    std::cerr << "Invalid page number " << pg << std::endl;
                    return pdftoedn::ErrorTracker::CODE_INIT_ERROR;
                }
            }
            if (vm.count("text_only") && vm["text_only"].as<bool>() &&
                vm.count("graphics_only") && vm["graphics_only"].as<bool>()) {
                throw std::logic_error("Can't select both 'text only' and 'graphics only' options.");
            }
            po::notify(vm);
        }
        catch (po::error& e) {
            std::cerr << "Error parsing program arguments: " << e.what() << std::endl
                      << std::endl
                      << desc << std::endl;
            return pdftoedn::ErrorTracker::CODE_INIT_ERROR;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Argument error: " << std::endl
                  << e.what() << std::endl;
        return pdftoedn::ErrorTracker::CODE_INIT_ERROR;
    }

    //
    // try to set the options - this checks that files exist, etc.
    try
    {
        // expand the paths if they start with ~
        pdftoedn::options = pdftoedn::Options(pdftoedn::util::fs::expand_path(pdf_filename),
                                              pdf_owner_password,
                                              pdf_user_password,
                                              pdftoedn::util::fs::expand_path(edn_output_filename),
                                              font_map_file,
                                              flags,
                                              (page_number >= 0 ? page_number : -1));
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return pdftoedn::ErrorTracker::CODE_INIT_ERROR;
    }

    // init support libs if needed
    pdftoedn::util::xform::init_transform_lib();

    globalParams = new GlobalParams();

    // register the error handler for this document
    setErrorCallback(&pdftoedn::ErrorTracker::error_handler, &pdftoedn::et);

    uintmax_t status = 0;
    try
    {
        // open the doc using arguments in Options - this step reads
        // general properties from the doc (num pages, PDF version) and
        // the outline
        pdftoedn::PDFReader doc_reader;

        std::ofstream output;
        output.open(pdftoedn::options.edn_filename().c_str());

        if (!output.is_open()) {
            std::stringstream err;
            err << pdftoedn::options.edn_filename() << "Cannot open file for write";
            throw pdftoedn::invalid_file(err.str());
        }

        // write the document data
        output << doc_reader;

        // done
        output.close();

        // set the exit code based on the logged errors
        status = pdftoedn::et.exit_code();

    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        status = pdftoedn::ErrorTracker::CODE_INIT_ERROR;
    }

    delete globalParams;

    return status;
}
