# Change Log

## 0.36.1 - 2018-10-31
### Fixed
* Replaced deprecated poppler boooleans to allow compilation against poppler-0.71.0

## 0.36.0 - 2018-10-22
### Fixed
* Updated method / function signatures to compile with poppler-0.70.0

### Changed
* minor internal tweaks and cleanup

## 0.35.3 - 2018-08-31
### Changed
* Relaxed boost min version to 1.57 to make it easier to install on
  certain distros
* Removed (almost?) all remaining references to Edsel
* Prevent overwriting of source PDF if passed to -o by mistake

## 0.35.2 - 2018-06-20
### Changed
* Minor change to prevent joining spans that are too far apart.

### Added
* New -T and -G options for extracting only text or graphics data.

## 0.35.1 - 2018-04-18
### Fixed
* Added missing m4 macros

## 0.35.0 - 2018-04-18
### Changed
* API compatibility for poppler-0.64.0 (new min. required)

### Fixed
* lib version output.
* program arguments don't require options not needed.

## 0.34.3 - 2017-08-14

### Fixed
* segfault on homebrew builds due to variable optimization.

### Changed
* Updated AX_CXX_ M4 macros.

## 0.34.2 - 2017-06-21

### Changed
* try not to break up text spans when inside a link
* refactoring of various internal methods
* minor corrections and updates to usage/lib output
* dependency version bumps
* corrected license information

## 0.34.1 - 2016-08-22

Last minute commits that should have been included in 0.34.0:

### Changed
* corrected use of `pixWriteMemPng()` to avoid memory leak.
* changed logic to prevent additional copy of encoded and transformed
  blobs
* fix for incorrect dash lines (transformation is required)
* renamed string stream identifiers for consistency and clarity

### Removed
* `memstream` handling since it's not used afterall - `leptonica` now
  handles tmp file creation on systems that don't provided
  `open_memstream` support. Will approach Dan to see if he's open to
  integrating the solution into his source to avoid file creation.

## 0.34.0 - 2016-08-20

### Added
* loads of error checking & runtime safety checks.
* now using `AX_CHECK_ENABLE_DEBUG` to set CPP flags up for
  debug/release builds.
* handling `~` home dir path expansion.
* new option to pass owner or user passwords for encrypted PDFs.
* fontmap file tests scripts for `make check`.

### Changed
* app now returns hex-code to indicate what components reported
  errors.
* configure now uses `pkg-config` for `rapidjson` and `leptonica`.
* exception handling to indicate init errors opening files, etc.
* libpng errors now reported in `:error` entries. Better error handling
  and cleanup of various encoding methods.
* replaced many lambda `std::for_each` loops with range-based for
  loops to make things a bit more readable.
* replaced old tmp-file OS X kludge for leptonica with bundled
  open_memstream from http://piumarta.com/software/memstream/
* improved leptonica error reporting.
* refactored image blob encoding, transformation, and
  caching. Simplified parameter passing to `StreamProps` constructors
  and other calls.

### Removed
* removed `#ifdef` wrapped `EDSEL_RUBY_GEM` methods and files.
* ruby formatting output option.

## 0.32.2 - 2016-08-12

### Added
* wrapped non-ruby support code in #else clause to allow gem build

### Changed
* fix incorrect output with closed path commands introduced with
  format changed on `v0.30.2`
* app now returns error codes greater than 0 to allow shell status
  check. Errors prior to document processing now go to stdout. Invalid
  PDF now returns error code 5 (will start introducing others and
  documenting them somewhere).
* `make check` now works from alternate build dirs. Added additional
  checks.

### Removed
* disabled logic to omit spans overlapped by other spans as it is
  incorrectly removing some valid cases. This should really be
  resolved by correctly tracking PDF operation order between text and
  graphics

## 0.32.1 - 2016-08-09

### Added
* autotools-based test suite (run with `make check`)
* function to output loaded font maps

### Changed
* fix for -1 page number argument. Ignored but should not be accepted

## 0.32.0 - 2016-08-08

Initial public release. Completed conversion from ruby gem - some
clode still in place to allow parallel builds for the time being.

### Added
* using boost program options for arg parsing
* autotools for setting up build
* man page and WIKI documentation

### Changed
* scoping of poppler and freetype headers
* internalized default font map to avoid requiring installed file
* changed config dir to `~/.pdftoedn/`
* output file format version for minor tweaks due to app name change
