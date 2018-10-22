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

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <wordexp.h>
#include "util_fs.h"
#include "pdf_error_tracker.h"

namespace pdftoedn
{
    namespace util
    {
        namespace fs
        {
            bool create_fs_dir(const boost::filesystem::path& dir)
            {
                namespace fs = boost::filesystem;

                // create it if it doesn't exist
                if (fs::exists(dir)) {
                    // if it exists, error out if it's NOT a directory
                    if (!fs::is_directory(dir)) {
                        std::cerr << "directory " << dir << " can't be created because it exists and is not a directory" << std::endl;
                        return false;
                    }
                }
                else {
                    // otherwise try to create it
                    return (fs::create_directories(dir));
                }
                return true;
            }


            //
            // if path starts with ~, expand it
            std::string expand_path(const std::string& path)
            {
                if (path.find("~") == 0) {
                    wordexp_t exp_result;
                    std::string exp_rslt;

                    if (wordexp(path.c_str(), &exp_result, WRDE_SHOWERR) != 0) {
                        throw invalid_file("error expanding path");
                    }

                    for (intmax_t ii = 0; ii < exp_result.we_wordc; ++ii) {
                        if (ii != 0) {
                            exp_rslt += ' ';
                        }
                        exp_rslt += exp_result.we_wordv[ii];
                    }
                    wordfree(&exp_result);
                    return exp_rslt;
                }
                return path;
            }

            //
            // check for valid input files
            bool check_valid_input_file(const boost::filesystem::path& infile)
            {
                namespace fs = boost::filesystem;

                // check input file
                if (!fs::exists(infile)) {
                    std::stringstream err;
                    err << "File " << infile << " does not exist" << std::endl;
                    throw invalid_file(err.str());
                }

                if (!fs::is_regular_file(infile)) {
                    std::stringstream err;
                    err << infile << " is not a valid file" << std::endl;
                    throw invalid_file(err.str());
                }
                return true;
            }


            //
            // write image blob to disk
            bool write_image_to_disk(const std::string& filename, const std::string& blob,
                                     bool overwrite)
            {
                // TODO: overwrite is now false by default but maybe
                // check if destination is the same and overwrite?
                if (overwrite || !boost::filesystem::exists(filename))
                {
                    std::ofstream file;
                    file.open(filename.c_str());

                    if (!file.is_open()) {
                        return false;
                    }

                    file << blob;
                    file.close();
                }
                return true;
            }


            //
            // opens file, reads size, allocates buffer for storage
            // (pointed to by *data), and copies content to it. NOTE:
            // caller must free buffer
            bool read_text_file(const std::string& filename, char** data)
            {
                *data = nullptr;

                // open it for reading
                std::ifstream tmp(filename, std::ifstream::in);
                if (!tmp.is_open()) {
                    return false;
                }

                // determine the size
                tmp.seekg(0, std::ios::end);
                std::size_t size = tmp.tellg();

                // allocate and copy contents of file to the buffer
                // with room to null-terminate
                *data = new char[size + 1];
                tmp.seekg(0, std::ios::beg);
                tmp.read(*data, size);
                tmp.close();
                (*data)[size] = 0;
                return true;
            }

        } // fs
    } // util
} // namespace
