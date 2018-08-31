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
#include <string>
#include <list>

#include <poppler/Error.h>

#include "pdf_error_tracker.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    const Symbol ErrorTracker::SYMBOL_ERROR_TYPES[]      = {
        "syntax_warning",
        "syntax_error",
        "config",
        "cli",
        "io",
        "not_allowed",
        "unimplemented",
        "internal",

        // graphics libraries
        "png_warning",
        "png_error",

        // our errors from here on
        "", // entry for the delimiter

        "invalid_args",
        "unhandled_link_action",
        "fe_font_freetype",
        "fe_font_read",
        "fe_font_read_unsupported_type",
        "fe_font_map",
        "fe_font_map_dup",
        "od_unimplemented_cb",
        "page_data",
        "ut_image_encode",
        "ut_image_xform",
    };

    const Symbol ErrorTracker::SYMBOL_ERROR_LEVELS[]     = {
        "info",
        "warning",
        "error",
        "critical",
    };
    const Symbol ErrorTracker::SYMBOL_ERRORS             = "errors";
    const Symbol ErrorTracker::error::SYMBOL_TYPE        = "type";
    const Symbol ErrorTracker::error::SYMBOL_LEVEL       = "level";
    const Symbol ErrorTracker::error::SYMBOL_MODULE      = "module";
    const Symbol ErrorTracker::error::SYMBOL_DESC        = "desc";
    const Symbol ErrorTracker::error::SYMBOL_COUNT       = "count";


    //
    // output error as EDN
    std::ostream& ErrorTracker::error::to_edn(std::ostream& o) const
    {
        util::edn::Hash h(5);
        h.push( SYMBOL_TYPE, SYMBOL_ERROR_TYPES[ type ] );
        h.push( SYMBOL_LEVEL, SYMBOL_ERROR_LEVELS[ lvl ] );
        h.push( SYMBOL_MODULE, mod );
        h.push( SYMBOL_DESC, msg );
        if (count > 1) {
            h.push( SYMBOL_COUNT, count );
        }
        o << h;
        return o;
    }


    //
    // has this error been marked to be ignored already?
    bool ErrorTracker::error_muted(error_type e) const
    {
        if ( std::any_of( ignore_errors.begin(), ignore_errors.end(),
                          [&](const error_type& err) { return (err == e); }
                          ) ) {
            return true;
        }
        return false;
    }

    //
    // marks this error type so we don't report it in the output
    void ErrorTracker::ignore_error(error_type e)
    {
        if (!error_muted(e)) {
            ignore_errors.push_back(e);
        }
    }


    //
    // set exit code bit flags depending on the error code
    void ErrorTracker::set_error_code(error_type e)
    {
        switch (e)
        {
          // treat these as warnings so do nothing with them
          case ERROR_SYNTAX_WARNING:
          case ERROR_PNG_WARNING:
          case ERROR_OD_UNIMPLEMENTED_CB:
              break;

          // poppler errors
          case ERROR_SYNTAX_ERROR:
          case ERROR_CONFIG:
          case ERROR_COMMAND_LINE:
          case ERROR_IO:
          case ERROR_NOT_ALLOWED:
          case ERROR_UNIMPLEMENTED:
          case ERROR_INTERNAL:
              exit_code_flags |= CODE_RUNTIME_POPPLER;
              break;

          // image conversion errors
          case ERROR_PNG_ERROR:
          case ERROR_UT_IMAGE_ENCODE:
              exit_code_flags |= CODE_RUNTIME_IMGWRITE;
              break;

          // image transform errors
          case ERROR_UT_IMAGE_XFORM:
              exit_code_flags |= CODE_RUNTIME_IMGXFORM;
              break;

          // font map errors
          case ERROR_FE_FONT_FT:
          case ERROR_FE_FONT_READ:
          case ERROR_FE_FONT_READ_UNSUPPORTED:
          case ERROR_FE_FONT_MAPPING:
          case ERROR_FE_FONT_MAPPING_DUPLICATE:
              exit_code_flags |= CODE_RUNTIME_FONTMAP;
              break;

          case ERROR_INVALID_ARGS:
          case ERROR_UNHANDLED_LINK_ACTION:
          case ERROR_PAGE_DATA:
              exit_code_flags |= CODE_RUNTIME_APP;
              break;

          default:
              std::cerr << "Unhandled error code: " << e << std::endl;
              break;
        }
    }


    //
    // add an entry to the list
    void ErrorTracker::log(error_type e, error::level l, const std::string& module, const std::string& msg)
    {
        if (error_muted(e)) {
            // don't log if it has been muted
            return;
        }

        // check if it's already been logged
        if (std::any_of( errors.begin(), errors.end(),
                         [&](const error* err) { return (err->matches(e, module, msg)); }
                         )) {
            return;
        }

        // set the status code flag depending on the error type if the
        // log level is ERROR or CRITICAL
        if (l > error::L_WARNING) {
            set_error_code(e);
        }

        // add it to the list
        error* err = new error(e, l, module, msg);
        errors.push_back( err );

        // stderr if it's critical
        if (l == error::L_CRITICAL) {
            std::cerr << "Critical error: " << *err << std::endl;
        }
    }

    //
    // purge the list of errors
    void ErrorTracker::flush_errors()
    {
        if (!errors.empty()) {
            util::delete_ptr_container_elems(errors);
            errors.clear();
        }
    }

    //
    // checks if there's anything more serious than warnings
    bool ErrorTracker::errors_reported() const
    {
        return std::any_of( errors.begin(), errors.end(),
                            [](const error* e) { return (e->higher_than(error::L_WARNING)); }
                            );
    }


    // outputs the list of errors
    std::ostream& ErrorTracker::to_edn(std::ostream& o) const
    {
        util::edn::Vector v(errors.size());
        for (const error* e : errors) {
            v.push( e );
        }
        o << v;
        return o;
    }


    //
    // static function for registering with poppler's error handler
    void ErrorTracker::error_handler(void *data, ErrorCategory category, Goffset pos, char *msg)
    {
        if (!msg) {
            std::cerr << __FUNCTION__ << " - NULL error message" << std::endl;
            return;
        }

        ErrorTracker* et = reinterpret_cast<ErrorTracker*>(data);

        if (et)
        {
            ErrorTracker::error_type e;
            ErrorTracker::error::level l;

            if (util::poppler_error_to_edn(category, msg, pos, e, l)) {
                et->log(e, l, "poppler", msg);
            }
        }
    }

} // namespace
