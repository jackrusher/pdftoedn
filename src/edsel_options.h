#pragma once

#include <string>

namespace pdftoedn {

    class Options
    {
    public:
        // runtime options
        struct Flags {
            bool omit_outline;
            bool use_page_crop_box;
            bool crop_page;
            bool include_invisible_text;
            bool link_output_only;
            bool include_debug_info;
            bool libpng_use_best_compression;
            bool force_font_preprocess;
            bool force_output_write;
        };

        Options() : page_num(-1) {}
        Options(const std::string& filename, const std::string& font_map, const std::string& out_filename,
                const Flags& f, intmax_t pg_num);

        const std::string& filename() const      { return file_name; }
        const std::string& outputdir() const     { return output_path; }
        const std::string& outputfile() const    { return output_file; }
        intmax_t page_number() const             { return page_num; }

        const std::string& map_config_path() const;
        const std::string& default_map_file() const;
        const std::string& font_map_file() const { return font_map; }

        bool get_image_path(intmax_t id, std::string& abs_file_path) const;
        std::string get_image_rel_path(const std::string& abs_path) const;

        bool omit_outline() const                { return flags.omit_outline; }
        bool use_page_crop_box() const           { return flags.use_page_crop_box; }
        bool crop_page() const                   { return flags.crop_page; }
        bool include_invisible_text() const      { return flags.include_invisible_text; }
        bool link_output_only() const            { return flags.link_output_only; }
        bool libpng_use_best_compression() const { return flags.libpng_use_best_compression; }
        bool include_debug_info() const          { return flags.include_debug_info; }
        bool force_pre_process_fonts() const     { return flags.force_font_preprocess; }
        bool force_output_write() const          { return flags.force_output_write; }

        static std::string get_absolute_map_path(const std::string& name);
        friend std::ostream& operator<<(std::ostream& o, const Options& opt);

    private:
        std::string file_name;
        std::string font_map;
        std::string output_file;
        Flags flags;
        intmax_t page_num;
        std::string output_path;
        std::string resource_dir;
        std::string doc_base_name;
    };

    extern pdftoedn::Options options;

} // namespace