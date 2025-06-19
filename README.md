# pdf2svgwrapper

`pdf2svgwrapper` is a small C/C++ library that converts PDF pages to SVG or PNG images using the Poppler and Cairo libraries. The project uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management and builds a shared library for Windows or Linux.

## Features

- Loads a PDF document from memory and reports the number of pages.
- Renders individual pages to SVG or PNG (image-only pages are rasterized and DPI can be specified).
- Simple C API for integration with other languages.

## Getting the Source

```bash
# Clone and fetch submodules
git clone <repo> pdf2svgwrapper
cd pdf2svgwrapper
git submodule update --init --recursive
```

The `vcpkg` directory is a submodule and must be initialized before building.

## Building

Use CMake to configure and build the library. The `vcpkg` toolchain is configured automatically when `CMAKE_TOOLCHAIN_FILE` is not set.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

This produces the `pdf2svgwrapper` shared library (e.g. `pdf2svgwrapper.dll` on Windows or `pdf2svgwrapper.so` on Linux) in the build directory.

## Usage

Include `pdf2svg.h` and link against `pdf2svgwrapper`. Example pseudo-code:

```c
int main() {
    /* load PDF data into memory */
    int page_count = 0;
    void* doc = pdf_open_doc(data, size, &page_count);
    if (!doc) return 1;

    for (int i = 0; i < page_count; ++i) {
        int len = 0;
        bool is_svg = false;
        unsigned char* data = pdf_get_page_data(doc, i, false, 0, &len, &is_svg);
        /* use the data (SVG when is_svg is true, otherwise PNG) */
        pdf_release_buffer(data);
    }

    pdf_close_doc(doc);
}
```

Refer to `src/pdf2svg.h` for the full function declarations.

`pdf_get_page_data` returns SVG data when the page contains text and PNG data for
image-only pages. Set `force_to_png` to `true` to always get PNG output. The `dpi`
argument controls the resolution of PNG output (72 DPI by default).

## Dependencies

Dependencies are declared in `vcpkg.json` and include `glib`, `cairo` and `poppler` with GLib and Cairo features:

```json
{
  "name": "pd2svgwrapper",
  "version": "0.1.0",
  "dependencies": [
    "pkgconf",
    "glib",
    "cairo",
    { "name": "poppler", "features": ["glib", "cairo"] }
  ]
}
```

## License

The repository does not currently include a license file. Consult the repository owner for licensing information.

