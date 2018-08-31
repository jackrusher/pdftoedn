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

#include <string>
#include <boost/filesystem.hpp>

namespace pdftoedn
{
    namespace util
    {
        namespace fs {
            bool create_fs_dir(const boost::filesystem::path& path);
            std::string expand_path(const std::string& path);
            bool check_valid_input_file(const boost::filesystem::path& infile);
            bool write_image_to_disk(const std::string& filename, const std::string& blob,
                                     bool overwrite = false);
            bool read_text_file(const std::string& filename, char** data);
        }
    }
}
