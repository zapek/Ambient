#ifndef CLIB_PNGLIB_PROTOS_H
#define CLIB_PNGLIB_PROTOS_H
/*
 * $Id: png_protos.h,v 1.2 2005/07/04 23:06:11 laire Exp $
 */

ULONG png_access_version_number(void);
void png_set_sig_bytes(APTR png_ptr, LONG num_bytes);
LONG png_sig_cmp(APTR sig, ULONG start, ULONG num_to_check);
LONG png_check_sig(APTR sig, LONG num);
APTR png_create_read_struct(STRPTR user_png_ver, APTR error_ptr, APTR error_fn, APTR warn_fn);
APTR png_create_write_struct(STRPTR user_png_ver, APTR error_ptr, APTR error_fn, APTR warn_fn);
ULONG png_get_compression_buffer_size(APTR png_ptr);
void png_set_compression_buffer_size(APTR png_ptr, ULONG size);
LONG png_reset_zstream(APTR png_ptr);
APTR png_create_read_struct_2(STRPTR user_png_ver, APTR error_ptr, APTR error_fn, APTR warn_fn, APTR mem_ptr, APTR malloc_fn, APTR free_fn);
APTR png_create_write_struct_2(STRPTR user_png_ver, APTR error_ptr, APTR error_fn, APTR warn_fn, APTR mem_ptr, APTR malloc_fn, APTR free_fn);

void png_write_chunk(APTR png_ptr, APTR chunk_name, APTR data, ULONG length);
void png_write_chunk_start(APTR png_ptr, APTR chunk_name, ULONG length);
void png_write_chunk_data(APTR png_ptr, APTR data, ULONG length);
void png_write_chunk_end(APTR png_ptr);
APTR png_create_info_struct(APTR png_ptr);
void png_info_init_3(APTR info_ptr, ULONG png_info_struct_size);

void png_write_info_before_PLTE(APTR png_ptr, APTR info_ptr);
void png_write_info(APTR png_ptr, APTR info_ptr);
void png_read_info(APTR png_ptr, APTR info_ptr);

void png_convert_from_struct_tm(APTR ptime, APTR ttime);
void png_convert_from_time_t(APTR ptime, APTR ttime);

void png_set_expand(APTR png_ptr);
void png_set_gray_1_2_4_to_8(APTR png_ptr);
void png_set_palette_to_rgb(APTR png_ptr);
void png_set_tRNS_to_alpha(APTR png_ptr);

void png_set_bgr(APTR png_ptr);
void png_set_gray_to_rgb(APTR png_ptr);

void png_set_rgb_to_gray(APTR png_ptr, LONG error_action, double red, double green);
void png_set_rgb_to_gray_fixed(APTR png_ptr, LONG error_action, LONG red, LONG green);
BYTE png_get_rgb_to_gray_status(APTR png_ptr);
void png_build_grayscale_palette(LONG bit_depth, APTR palette);

void png_set_strip_alpha(APTR png_ptr);
void png_set_swap_alpha(APTR png_ptr);
void png_set_invert_alpha(APTR png_ptr);

void png_set_filler(APTR pnt_ptr, ULONG filler, LONG flags);

void png_set_swap(APTR pnt_ptr);
void png_set_packing(APTR pnt_ptr);
void png_set_packswap(APTR png_ptr);
void png_set_shift(APTR png_ptr, APTR true_bits);
LONG png_set_interlace_handling(APTR png_ptr);
void png_set_invert_mono(APTR png_ptr);
void png_set_background(APTR png_ptr, APTR background_color, LONG background_gamma_code, LONG need_expand, double background_gamma);
void png_set_strip_16(APTR png_ptr);
void png_set_dither(APTR png_ptr, APTR palette, LONG num_palette, LONG maximum_colors, APTR histogram, LONG full_dither);
void png_set_gamma(APTR png_ptr, double screen_gamma, double default_file_gamma);

void png_permit_empty_plte(APTR png_ptr, LONG empty_plte_permitted);

void png_set_flush(APTR png_ptr, LONG nrows);
void png_write_flush(APTR png_ptr);

void png_start_read_image(APTR png_ptr);
void png_read_update_info(APTR png_ptr, APTR info_ptr);

void png_read_rows(APTR png_ptr, APTR row, APTR display_row, ULONG num_rows);
void png_read_row(APTR png_ptr, APTR row, APTR display_row);
void png_read_image(APTR png_ptr, APTR image);

void png_write_row(APTR png_ptr, APTR row);
void png_write_rows(APTR png_ptr, APTR row, ULONG num_rows);
void png_write_image(APTR png_ptr, APTR image);
void png_write_end(APTR png_ptr, APTR info_ptr);

void png_read_end(APTR png_ptr, APTR info_ptr);
void png_destroy_info_struct(APTR png_ptr, APTR info_ptr_ptr);

void png_destroy_read_struct(APTR * png_ptr_ptr, APTR info_ptr_ptr, APTR end_info_ptr_ptr);
void png_read_destroy(APTR png_ptr, APTR info_ptr, APTR end_info_ptr);
void png_destroy_write_struct(APTR * png_ptr_ptr, APTR info_ptr_ptr);

void png_write_destroy(APTR png_ptr);
void png_set_crc_action(APTR png_ptr, LONG crit_action, LONG ancil_action);
void png_set_filter(APTR png_ptr, LONG method, LONG filters);
void png_set_filter_heuristics(APTR png_ptr, LONG heuristic_method, LONG num_weights, APTR filter_weights, APTR filter_costs);
void png_set_compression_level(APTR png_ptr, LONG level);
void png_set_compression_mem_level(APTR png_ptr, LONG mem_level);
void png_set_compression_strategy(APTR png_ptr, LONG strategy);
void png_set_compression_window_bits(APTR png_ptr, LONG window_bits);
void png_set_compression_method(APTR png_ptr, LONG method);
void png_set_error_fn(APTR png_ptr, APTR error_ptr, APTR error_fn, APTR warning_fn);
APTR png_get_error_ptr(APTR png_ptr);
void png_set_write_fn(APTR png_ptr, APTR io_ptr, APTR write_data_fn, APTR output_flush_fn);
void png_set_read_fn(APTR png_ptr, APTR io_ptr, APTR read_data_fn);
APTR png_get_io_ptr(APTR png_ptr);
void png_set_read_status_fn(APTR png_ptr, APTR read_row_fn);
void png_set_write_status_fn(APTR png_ptr, APTR write_row_fn);
void png_set_mem_fn(APTR png_ptr, APTR mem_ptr, APTR malloc_fn, APTR free_fn);
APTR png_get_mem_ptr(APTR png_ptr);
void png_set_read_user_transform_fn(APTR png_ptr, APTR read_user_transform_fn);
void png_set_write_user_transform_fn(APTR png_ptr, APTR write_user_transform_fn);
void png_set_user_transform_info(APTR png_ptr, APTR user_transform_ptr, LONG user_transform_depth, LONG user_transform_channels);
APTR png_get_user_transform_ptr(APTR png_ptr);
void png_set_read_user_chunk_fn(APTR png_ptr, APTR user_chunk_ptr, APTR read_user_chunk_fn);
APTR png_get_user_chunk_ptr(APTR png_ptr);

APTR png_malloc(APTR png_ptr, ULONG size);
void png_free(APTR png_ptr, APTR ptr);
APTR png_zalloc(APTR png_ptr, ULONG items, ULONG size);
void png_zfree(APTR png_ptr, APTR ptr);

void png_free_data(APTR png_ptr, APTR info_ptr, ULONG free_me, LONG num);
void png_data_freer(APTR png_ptr, APTR info_ptr, LONG freer, ULONG mask);

APTR png_malloc_default(APTR png_ptr, ULONG size);
void png_free_default(APTR png_ptr, APTR ptr);

APTR png_memcpy_check(APTR png_ptr, APTR s1, APTR s2, ULONG size);
APTR png_memset_check(APTR png_ptr, APTR s1, LONG value, ULONG size);

void png_error(APTR png_ptr, APTR error);
void png_chunk_error(APTR png_ptr, APTR error);
void png_warning(APTR png_ptr, APTR message);
void png_chunk_warning(APTR png_ptr, APTR message);
ULONG png_get_valid(APTR png_ptr, APTR info_ptr, ULONG flag);

ULONG png_get_rowbytes(APTR png_ptr, APTR info_ptr);
APTR * png_get_rows(APTR png_ptr, APTR info_ptr);
void png_set_rows(APTR png_ptr, APTR info_ptr, APTR * row_pointers);

UBYTE png_get_channels(APTR png_ptr, APTR info_ptr);
ULONG png_get_image_width(APTR png_ptr, APTR info_ptr);
ULONG png_get_image_height(APTR png_ptr, APTR info_ptr);
BYTE png_get_bit_depth(APTR png_ptr, APTR info_ptr);
BYTE png_get_color_type(APTR png_ptr, APTR info_ptr);
BYTE png_get_filter_type(APTR png_ptr, APTR info_ptr);
BYTE png_get_interlace_type(APTR png_ptr, APTR info_ptr);
BYTE png_get_compression_type(APTR png_ptr, APTR info_ptr);

ULONG png_get_pixels_per_meter(APTR png_ptr, APTR info_ptr);
ULONG png_get_x_pixels_per_meter(APTR png_ptr, APTR info_ptr);
ULONG png_get_y_pixels_per_meter(APTR png_ptr, APTR info_ptr);

float png_get_pixel_aspect_ratio(APTR png_ptr, APTR info_ptr);

LONG png_get_x_offset_pixels(APTR png_ptr, APTR info_ptr);
LONG png_get_y_offset_pixels(APTR png_ptr, APTR info_ptr);
LONG png_get_x_offset_microns(APTR png_ptr, APTR info_ptr);
LONG png_get_y_offset_microns(APTR png_ptr, APTR info_ptr);

APTR png_get_signature(APTR png_ptr, APTR info_ptr);
ULONG png_get_bKGD(APTR png_ptr, APTR info_ptr, APTR *background);
void png_set_bKGD(APTR png_ptr,	APTR info_ptr, APTR background);
ULONG png_get_cHRM(APTR png_ptr, APTR info_ptr, double *white_x, double *white_y, double *red_x, double *red_y, double *green_x, double *green_y, double *blue_x, double *blue_y);
ULONG png_get_cHRM_fixed(APTR png_ptr, APTR info_ptr, LONG *int_white_x, LONG *int_white_y, LONG *int_red_x, LONG *int_red_y, LONG *int_green_x, LONG *int_green_y, LONG *int_blue_x, LONG *int_blue_y);
void png_set_cHRM(APTR png_ptr, APTR info_ptr, double white_x, double white_y, double red_x, double red_y, double green_x, double green_y, double blue_x, double blue_y);
void png_set_cHRM_fixed(APTR png_ptr, APTR info_ptr, LONG int_white_x, LONG int_white_y, LONG int_red_x, LONG int_red_y, LONG int_green_x, LONG int_green_y, LONG int_blue_x, LONG int_blue_y);

ULONG png_get_gAMA(APTR png_ptr, APTR info_ptr, double *file_gamma);
ULONG png_get_gAMA_fixed(APTR png_ptr, APTR info_ptr, LONG *int_file_gamma);

void png_set_gAMA(APTR png_ptr, APTR info_ptr, double file_gamma);
void png_set_gAMA_fixed(APTR png_ptr, APTR info_ptr, LONG int_file_gamma);

ULONG png_get_hIST(APTR png_ptr, APTR info_ptr, APTR *hist);
void png_set_hIST(APTR png_ptr, APTR info_ptr, APTR hist);
ULONG png_get_IHDR(APTR png_ptr, APTR info_ptr, ULONG *width, ULONG *height, LONG *bit_depth, LONG *color_type, LONG *interlace_method, LONG *compression_method, LONG *filter_method);
void png_set_IHDR(APTR png_ptr, APTR info_ptr, ULONG width, ULONG height, LONG bit_depth, LONG color_type, LONG interlace_method, LONG compression_method, LONG filter_method);
ULONG png_get_oFFs(APTR png_ptr, APTR info_ptr, LONG *offset_x, LONG *offset_y, LONG *unit_type);
void png_set_oFFs(APTR png_ptr, APTR info_ptr, LONG offset_x, LONG offset_y, LONG unit_type);
ULONG png_get_pCAL(APTR png_ptr, APTR info_ptr, APTR *purpose, LONG *X0, LONG *X1, LONG *type, LONG *nparams, APTR *units, APTR **params);
void png_set_pCAL(APTR png_ptr, APTR info_ptr, APTR purpose, LONG X0, LONG X1, LONG type, LONG nparams, APTR units, APTR *params);
ULONG png_get_pHYs(APTR png_ptr, APTR info_ptr, ULONG *res_x, ULONG *res_y, LONG *unit_type);
void png_set_pHYs(APTR png_ptr, APTR info_ptr, ULONG res_x, ULONG res_y, LONG unit_type);
ULONG png_get_PLTE(APTR png_ptr, APTR info_ptr, APTR *palette, LONG *num_palette);
void png_set_PLTE(APTR png_ptr, APTR info_ptr, APTR palette, LONG num_palette);
ULONG png_get_sBIT(APTR png_ptr, APTR info_ptr, APTR *sig_bit);
void png_set_sBIT(APTR png_ptr, APTR info_ptr, APTR sig_bit);
ULONG png_get_sRGB(APTR png_ptr, APTR info_ptr, LONG *intent);

void png_set_sRGB(APTR png_ptr, APTR info_ptr, LONG intent);
void png_set_sRGB_gAMA_and_cHRM(APTR png_ptr, APTR info_ptr, LONG intent);

ULONG png_get_iCCP(APTR png_ptr, APTR info_ptr, APTR *name, LONG *compression_type, APTR *profile, ULONG *proflen);
void png_set_iCCP(APTR png_ptr, APTR info_ptr, APTR name, LONG compression_type, APTR profile, ULONG proflen);
ULONG png_get_sPLT(APTR png_ptr, APTR info_ptr, APTR entries);
void png_set_sPLT(APTR png_ptr, APTR info_ptr, APTR entries, LONG nentries);
ULONG png_get_text(APTR png_ptr, APTR info_ptr, APTR *text_ptr, LONG *num_text);
void png_set_text(APTR png_ptr, APTR info_ptr, APTR text_ptr, LONG num_text);
ULONG png_get_tIME(APTR png_ptr, APTR info_ptr, APTR *mod_time);
void png_set_tIME(APTR png_ptr, APTR info_ptr, APTR mod_time);
ULONG png_get_tRNS(APTR png_ptr, APTR info_ptr, APTR *trans, LONG *num_trans, APTR *trans_values);
void png_set_tRNS(APTR png_ptr, APTR info_ptr, APTR trans, LONG num_trans, APTR trans_values);

//ULONG png_get_sCAL(APTR png_ptr, APTR info_ptr, LONG *unit, double *width, double *height);
//void png_set_sCAL(APTR png_ptr, APTR info_ptr, LONG unit, double width, double height);

void png_set_keep_unknown_chunks(APTR png_ptr, LONG keep, APTR chunk_list, LONG num_chunks);
void png_set_unknown_chunks(APTR png_ptr, APTR info_ptr, APTR unknowns, LONG num_unknowns);
void png_set_unknown_chunk_location(APTR png_ptr, APTR info_ptr, LONG chunk, LONG location);
ULONG png_get_unknown_chunks(APTR png_ptr, APTR info_ptr, APTR * entries);
LONG png_handle_as_unknown(APTR png_ptr, APTR chunk_name);

void png_set_invalid(APTR png_ptr, APTR info_ptr, LONG mask);

void png_read_png(APTR png_ptr, APTR info_ptr, LONG transforms,	APTR params);
void png_write_png(APTR png_ptr, APTR info_ptr, LONG transforms, APTR params);

APTR png_get_copyright(APTR png_ptr);
APTR png_get_header_ver(APTR png_ptr);
APTR png_get_header_version(APTR png_ptr);
APTR png_get_libpng_ver(APTR png_ptr);

void png_set_strip_error_numbers(APTR png_ptr, ULONG strip_mode);

#endif /* CLIB_PNGLIB_PROTOS_H */
