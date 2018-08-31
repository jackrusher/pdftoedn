AUTOMAKE_OPTIONS = subdir-objects

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = pdftoedn
pdftoedn_SOURCES = \
	base_types.cc \
	color.cc \
	doc_page.cc \
	eng_output_dev.cc \
	font.cc \
	font_engine.cc \
	font_engine_dev.cc \
	font_maps.cc \
	graphics.cc \
	image.cc \
	link_output_dev.cc \
	main.cc \
	pdf_doc_outline.cc \
	pdf_error_tracker.cc \
	pdf_font_source.cc \
	pdf_links.cc \
	pdf_output_dev.cc \
	pdf_reader.cc \
	runtime_options.cc \
	text.cc \
	transforms.cc \
	util.cc \
	util_config.cc \
	util_config_default_map.cc \
	util_data_format_version.cc \
	util_debug.cc \
	util_edn.cc \
	util_encode.cc \
	util_fs.cc \
	util_versions.cc \
	util_xform.cc

if LOCAL_MD5
# include md5 code if openssl was not found
pdftoedn_SOURCES += external/bzflag_md5.cc
endif

# what flags you want to pass to the C compiler & linker
AM_CXXFLAGS = \
    $(PDFTOEDN_BUILD_CPPFLAGS) \
    $(BOOST_CXXFLAGS) \
    $(POPPLER_PARENT_INCLUDE) \
    $(poppler_CFLAGS) \
    $(freetype2_CFLAGS) \
    $(png_CFLAGS) \
    $(lept_CFLAGS) \
    $(OPENSSL_INCLUDES)

AM_LDFLAGS = \
    $(BOOST_LDFLAGS) \
    $(OPENSSL_LDFLAGS)

pdftoedn_LDADD =  \
    $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_LOCALE_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_REGEX_LIB) \
    $(poppler_LIBS) $(poppler_cpp_LIBS) \
    $(freetype2_LIBS) \
    $(png_LIBS) \
    $(lept_LIBS) \
    $(OPENSSL_LIBS)

# got bit by a leftover config.h in the src directory so rm -f
clean-local:
	-rm -f config.h config.log config.status
	-rm -rf *.dSYM
