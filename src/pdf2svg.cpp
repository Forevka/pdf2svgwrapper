
#include <vector>
#include <cstdlib>
#include <cstring>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include "pdf2svg.h"

#include <glib.h>
#include <glib-object.h>
#include <poppler/glib/poppler.h>
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>

struct PdfDoc {
    GBytes* bytes;
    PopplerDocument* doc;
    int page_count;
};

cairo_status_t write_cb(void* closure,
    const unsigned char* data,
    unsigned int len) {
    auto* buf = static_cast<std::vector<unsigned char>*>(closure);
    buf->insert(buf->end(), data, data + len);
    return CAIRO_STATUS_SUCCESS;
}

void* pdf_open_doc(const unsigned char* pdf_data, int pdf_len, int* out_page_count) {
    GError* err = nullptr;
    auto* bytes = g_bytes_new(pdf_data, (gsize)pdf_len);
    auto* doc = poppler_document_new_from_bytes(bytes, nullptr, &err);
    if (!doc) {
        g_bytes_unref(bytes);
        return nullptr;
    }
    auto* ctx = new PdfDoc{ bytes, doc, poppler_document_get_n_pages(doc) };
    *out_page_count = ctx->page_count;
    return ctx;
}

unsigned char* pdf_get_page_svg(void* doc_handle, int page_num, int* out_svg_len) {
    auto* ctx = static_cast<PdfDoc*>(doc_handle);
    PopplerPage* page = poppler_document_get_page(ctx->doc, page_num);
    double w, h; poppler_page_get_size(page, &w, &h);

    std::vector<unsigned char> buf;
    auto* surface = cairo_svg_surface_create_for_stream(
        write_cb, &buf, w, h
    );
    auto* cr = cairo_create(surface);
    poppler_page_render_for_printing(page, cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    *out_svg_len = (int)buf.size();
    auto* out = (unsigned char*)std::malloc(buf.size());
    std::memcpy(out, buf.data(), buf.size());
    return out;
}

void pdf_close_doc(void* doc_handle) {
    auto* ctx = static_cast<PdfDoc*>(doc_handle);
    g_object_unref(ctx->doc);
    g_bytes_unref(ctx->bytes);
    delete ctx;
}

void pdf_release_buffer(unsigned char* buffer) {
    std::free(buffer);
}
