# pdf2svgwrapper

`pdf2svgwrapper` is a small C/C++ library that converts PDF pages to SVG images using the Poppler and Cairo libraries. The project uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management and builds a shared library for Windows or Linux.

## Features

- Loads a PDF document from memory and reports the number of pages.
- Renders individual pages to SVG format.
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
        unsigned char* svg = pdf_get_page_svg(doc, i, &len);
        /* use the SVG data */
        pdf_release_buffer(svg);
    }

    pdf_close_doc(doc);
}
```

Refer to `src/pdf2svg.h` for the full function declarations.

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

