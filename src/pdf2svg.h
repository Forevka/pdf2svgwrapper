#pragma once

#ifdef _WIN32
    #ifdef BUILDING_DLL
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define DLL_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

    DLL_EXPORT void* pdf_open_doc(
        const unsigned char* pdf_data,
        int                 pdf_len,
        int*                out_page_count
    );

    DLL_EXPORT unsigned char* pdf_get_page_data(
        void* doc_handle,
        int   page_num,
        bool  force_to_png,
        int   dpi,
        int*  out_svg_len,
        bool* out_is_svg
    );

    DLL_EXPORT void pdf_close_doc(void* doc_handle);

    DLL_EXPORT void pdf_release_buffer(unsigned char* buffer);

#ifdef __cplusplus
}
#endif
