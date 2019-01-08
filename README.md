# pdftoedn 0.36.4
A [poppler](https://poppler.freedesktop.org)-based PDF processing tool
to extract document data and save it in
[EDN](https://github.com/edn-format/edn) format. It supports:

* Font and glyph remapping via user-defined font map configurations
  (in JSON format) to allow glyph substitutions for Type 1 or TT fonts
  with invalid/incorrect unicode tables and even embedded CID fonts
  with missing tables.
* Path data extraction.
* Transformed image output, written directly to disk in PNG format.
* Annotations.
* PDF outlines.

## Usage

Process a pdf document and write its output to `output_file.edn`:

```sh
pdftoedn -o output_file.edn input_file.pdf
```

## Further reading

Refer to the [wiki](https://github.com/edporras/pdftoedn/wiki) for

* more usage examples.
* [exit error code reference](https://github.com/edporras/pdftoedn/wiki/Returned-Exit-Error-Codes).
* [installation instructions](https://github.com/edporras/pdftoedn/wiki/Installation).
* [output file format](https://github.com/edporras/pdftoedn/wiki/Output-Format-Description).
* [overview of font map substitution](https://github.com/edporras/pdftoedn/wiki/Overview-of-Font-Substitution-Maps) and [sample font configuration file](https://github.com/edporras/pdftoedn/wiki/Sample-Font-Map-Configuration).
* List of [internal font maps](https://github.com/edporras/pdftoedn/wiki/Internal-Font-Maps) and [internal glyph maps](https://github.com/edporras/pdftoedn/wiki/Internal-Glyph-Maps).
