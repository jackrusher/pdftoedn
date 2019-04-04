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

#include <string>
#include <list>
#include <stdexcept>
#include <poppler/Error.h>
#include "base_types.h"

namespace pdftoedn
{
    // ----------------------------------
    //
    //
    struct ErrorTracker : public gemable {
        // runtime error codes - these map to enums in poppler/Error.h
        // plus we could add our own
        enum error_type {
            ERROR_SYNTAX_WARNING,
            ERROR_SYNTAX_ERROR,
            ERROR_CONFIG,
            ERROR_COMMAND_LINE,
            ERROR_IO,
            ERROR_NOT_ALLOWED,
            ERROR_UNIMPLEMENTED,
            ERROR_INTERNAL,

            // graphic libraries
            ERROR_PNG_WARNING,
            ERROR_PNG_ERROR,

            // our own errors from here on
            ERROR_NONE, // not an error - used to separate poppler errors from our own

            ERROR_INVALID_ARGS,
            ERROR_UNHANDLED_LINK_ACTION,
            ERROR_FE_FONT_FT,
            ERROR_FE_FONT_READ,
            ERROR_FE_FONT_READ_UNSUPPORTED,
            ERROR_FE_FONT_MAPPING,
            ERROR_FE_FONT_MAPPING_DUPLICATE,
            ERROR_OD_UNIMPLEMENTED_CB,
            ERROR_PAGE_DATA,
            ERROR_UT_IMAGE_ENCODE,
            ERROR_UT_IMAGE_XFORM,

            ERROR_TYPE_COUNT // delim
        };

        // application exit code flags
        enum ExitCode {
            CODE_INIT_ERROR       = 0x80,
            CODE_RUNTIME_POPPLER  = 0x40,
            CODE_RUNTIME_IMGWRITE = 0x20,
            CODE_RUNTIME_IMGXFORM = 0x10,
            CODE_RUNTIME_APP      = 0x08,
            CODE_RUNTIME_FONTMAP  = 0x04,

            //...
            CODE_RUNTIME_OK       = 0
        };


        // ========================================================
        //
        struct error : public gemable {
            enum level {
                L_INFO,
                L_WARNING,
                L_ERROR,
                L_CRITICAL
            };

            static const Symbol SYMBOL_TYPE;
            static const Symbol SYMBOL_LEVEL;
            static const Symbol SYMBOL_MODULE;
            static const Symbol SYMBOL_DESC;
            static const Symbol SYMBOL_COUNT;

            error(error_type e, level l, const std::string& m, const std::string& e_msg) :
                type(e), lvl(l), mod(m), msg(e_msg), count(1)
            { }

            bool matches(error_type e_type, const std::string& module, const std::string& message) const {
                // if it matches, increase the instance count
                if (e_type == type && module == mod && message == msg) {
                    count++;
                    return true;
                }
                return false;
            }
            bool higher_than(level floor) const { return (lvl > floor); }

            virtual std::ostream& to_edn(std::ostream& o) const;

        private:
            error_type type;
            level lvl;
            std::string mod;
            std::string msg;
            mutable uintmax_t count;
        };

        static const Symbol SYMBOL_ERROR_TYPES[];
        static const Symbol SYMBOL_ERROR_LEVELS[];
        static const Symbol SYMBOL_ERRORS;

        ErrorTracker() : exit_code_flags(0) {}
        virtual ~ErrorTracker() { flush_errors(); }

        void ignore_error(error_type e); // don't report this error type

        void log(error_type e, error::level l, const std::string& mod, const std::string& msg);
        void log_info(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_INFO, mod, msg);
        }
        void log_warn(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_WARNING, mod, msg);
        }
        void log_error(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_ERROR, mod, msg);
        }
        void log_critical(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_CRITICAL, mod, msg);
        }

        uint8_t exit_code() const { return exit_code_flags; }
        bool errors_reported() const;
        bool errors_or_warnings_reported() const { return !errors.empty(); }
        void flush_errors();

        virtual std::ostream& to_edn(std::ostream& o) const;

        // method to register error handler w/ poppler
        static void error_handler(void *data, ErrorCategory category, Goffset pos, const char *msg);

    private:

        uint8_t exit_code_flags;
        std::list<error *> errors;
        std::list<error_type> ignore_errors;

        void set_error_code(error_type e);
        bool error_muted(error_type e) const;
    };

    // exceptions
    struct init_error : public std::logic_error {
        init_error(const std::string& what) : logic_error(what) {}
    };
    struct invalid_file : public std::invalid_argument {
        invalid_file(const std::string& what) : invalid_argument(what) {}
    };

    extern pdftoedn::ErrorTracker et;
} // namespace
