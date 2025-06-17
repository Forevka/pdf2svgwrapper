
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>

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

unsigned char*
pdf_get_page_data(void* doc_handle,
                  int   page_num,
                  int*  out_len,
                  bool* out_is_svg)
{
    auto* ctx = static_cast<PdfDoc*>(doc_handle);
    PopplerPage* page = poppler_document_get_page(ctx->doc, page_num);

    double w, h;
    poppler_page_get_size(page, &w, &h);

    // if there is any selectable text
    char* txt = poppler_page_get_text(page);
    bool has_text = txt && *txt;
    g_free(txt);

    GList* images = poppler_page_get_image_mapping(page);
    bool image_only = false;

    // iterate WITHOUT freeing first
    for (GList* iter = images; iter; iter = iter->next) {
        auto* m = static_cast<PopplerImageMapping*>(iter->data);
        // check if the mapped image covers ~the entire page
        const double tol = 0.5;
        if (fabs(m->area.x1) < tol &&
            fabs(m->area.y1) < tol &&
            fabs(m->area.x2 - w) < tol &&
            fabs(m->area.y2 - h) < tol)
        {
            image_only = true;
            break;
        }
    }

    poppler_page_free_image_mapping(images);

    std::vector<unsigned char> buf;

    if (image_only && !has_text) {
        // --- Rasterize to PNG ---
        auto* img_surf = cairo_image_surface_create(
                             CAIRO_FORMAT_ARGB32,
                             static_cast<int>(w),
                             static_cast<int>(h));
        auto* img_cr   = cairo_create(img_surf);
        poppler_page_render(page, img_cr);
        cairo_destroy(img_cr);

        cairo_surface_write_to_png_stream(img_surf, write_cb, &buf);
        cairo_surface_destroy(img_surf);
        *out_is_svg = false;
    }
    else {
        // --- Emit SVG ---
        auto* svg_surf = cairo_svg_surface_create_for_stream(
                             write_cb, &buf, w, h);
        auto* svg_cr   = cairo_create(svg_surf);
        poppler_page_render_for_printing(page, svg_cr);
        cairo_destroy(svg_cr);
        cairo_surface_destroy(svg_surf);
        *out_is_svg = true;
    }

    *out_len = static_cast<int>(buf.size());
    auto* out = static_cast<unsigned char*>(std::malloc(buf.size()));
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
