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
    if (!pdf_data || pdf_len <= 0 || !out_page_count)
        return nullptr;

    GError* err = nullptr;
    auto* bytes = g_bytes_new(pdf_data, (gsize)pdf_len);
    auto* doc = poppler_document_new_from_bytes(bytes, nullptr, &err);
    if (!doc) {
        g_bytes_unref(bytes);
        if (err)
            g_error_free(err);
        *out_page_count = 0;
        return nullptr;
    }
    auto* ctx = new PdfDoc{ bytes, doc, poppler_document_get_n_pages(doc) };
    *out_page_count = ctx->page_count;
    return ctx;
}

unsigned char*
pdf_get_page_data(void*   doc_handle,
                                 int     page_num,
                                 bool    force_to_png,
                                 int     dpi,
                                 int*    out_len,
                                 bool*   out_is_svg)
{
    if (!doc_handle || !out_len || !out_is_svg) {
        if (out_len)    *out_len    = 0;
        if (out_is_svg) *out_is_svg = false;
        return nullptr;
    }

    auto* ctx = static_cast<PdfDoc*>(doc_handle);
    PopplerPage* page = poppler_document_get_page(ctx->doc, page_num);
    if (!page) {
        *out_len    = 0;
        *out_is_svg = false;
        return nullptr;
    }

    double w, h;
    poppler_page_get_size(page, &w, &h);

    bool has_text = false;
    bool image_only = false;

    if (!force_to_png) {
        char* txt = poppler_page_get_text(page);
        has_text = (txt && *txt != '\0');
        g_free(txt);

        GList* images = poppler_page_get_image_mapping(page);
        const double tol = 0.5;

        for (GList* iter = images; iter; iter = iter->next) {
            auto* m = static_cast<PopplerImageMapping*>(iter->data);
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
    } else {
        image_only = true;
    }

    std::vector<unsigned char> buf;
    if (image_only && !has_text) {
        if (dpi <= 0)
            dpi = 72;

        double scale      = dpi / 72.0;
        int    width_px   = static_cast<int>(std::ceil(w * scale));
        int    height_px  = static_cast<int>(std::ceil(h * scale));

        cairo_surface_t* img_surf = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            width_px,
            height_px);

        if (cairo_surface_status(img_surf) != CAIRO_STATUS_SUCCESS) {
            cairo_surface_destroy(img_surf);
            g_object_unref(page);
            *out_len    = 0;
            *out_is_svg = false;
            return nullptr;
        }

        cairo_t* img_cr = cairo_create(img_surf);

        if (cairo_status(img_cr) != CAIRO_STATUS_SUCCESS) {
            cairo_destroy(img_cr);
            cairo_surface_destroy(img_surf);
            g_object_unref(page);
            *out_len    = 0;
            *out_is_svg = false;
            return nullptr;
        }

        cairo_scale(img_cr, scale, scale);

        poppler_page_render(page, img_cr);

        cairo_destroy(img_cr);

        cairo_surface_write_to_png_stream(img_surf, write_cb, &buf);

        cairo_surface_destroy(img_surf);

        *out_is_svg = false;
    } else {
        cairo_surface_t* svg_surf = cairo_svg_surface_create_for_stream(
            write_cb, &buf, w, h);

        if (cairo_surface_status(svg_surf) != CAIRO_STATUS_SUCCESS) {
            cairo_surface_destroy(svg_surf);
            g_object_unref(page);
            *out_len    = 0;
            *out_is_svg = false;
            return nullptr;
        }

        cairo_t* svg_cr = cairo_create(svg_surf);

        if (cairo_status(svg_cr) != CAIRO_STATUS_SUCCESS) {
            cairo_destroy(svg_cr);
            cairo_surface_destroy(svg_surf);
            g_object_unref(page);
            *out_len    = 0;
            *out_is_svg = false;
            return nullptr;
        }

        poppler_page_render_for_printing(page, svg_cr);

        cairo_destroy(svg_cr);
        cairo_surface_destroy(svg_surf);

        *out_is_svg = true;
    }

    g_object_unref(page);

    *out_len = static_cast<int>(buf.size());
    if (*out_len == 0) {
        *out_len    = 0;
        *out_is_svg = false;
        return nullptr;
    }

    unsigned char* out = static_cast<unsigned char*>(std::malloc(buf.size()));
    if (!out) {
        *out_len    = 0;
        *out_is_svg = false;
        return nullptr;
    }

    std::memcpy(out, buf.data(), buf.size());

    return out;
}

void pdf_close_doc(void* doc_handle) {
    if (!doc_handle) return;
    auto* ctx = static_cast<PdfDoc*>(doc_handle);
    g_object_unref(ctx->doc);
    g_bytes_unref(ctx->bytes);
    delete ctx;
}

void pdf_release_buffer(unsigned char* buffer) {
    if (!buffer) return;
    std::free(buffer);
}
