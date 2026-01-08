/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: libfunctable.c,v 1.3 2006/12/20 13:34:18 fab Exp $
 */

#include "globals.h"

/* public */

/* private */
#include "lib.h"
#include "clib/png_protos.h"

void LIB_Dummy(void);

void LIB_Dummy(void)
{
}

ULONG LibFuncTable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	(ULONG)&png_access_version_number,
	(ULONG)&png_set_sig_bytes,
	(ULONG)&png_sig_cmp,
	(ULONG)&png_check_sig,
	(ULONG)&png_create_read_struct,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	(ULONG)&LIB_Dummy,
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_create_write_struct,
	(ULONG)&png_get_compression_buffer_size,
	(ULONG)&png_set_compression_buffer_size,
	#endif
	(ULONG)&png_reset_zstream,
	(ULONG)&png_create_read_struct_2,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_create_write_struct_2,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_chunk,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_chunk_start,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_chunk_data,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_chunk_end,
	#endif
	(ULONG)&png_create_info_struct,
	(ULONG)&png_info_init_3,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_info_before_PLTE,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_info,
	#endif
	(ULONG)&png_read_info,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_convert_from_struct_tm,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_convert_from_time_t,
	#endif
	(ULONG)&png_set_expand,
	(ULONG)&png_set_gray_1_2_4_to_8,
	(ULONG)&png_set_palette_to_rgb,
	(ULONG)&png_set_tRNS_to_alpha,
	(ULONG)&png_set_bgr,
	(ULONG)&png_set_gray_to_rgb,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_rgb_to_gray,
	#endif
	(ULONG)&png_set_rgb_to_gray_fixed,
	(ULONG)&png_get_rgb_to_gray_status,
	(ULONG)&png_build_grayscale_palette,
	(ULONG)&png_set_strip_alpha,
	(ULONG)&png_set_swap_alpha,
	(ULONG)&png_set_invert_alpha,
	(ULONG)&png_set_filler,
	(ULONG)&png_set_swap,
	(ULONG)&png_set_packing,
	(ULONG)&png_set_packswap,
	(ULONG)&png_set_shift,
	(ULONG)&png_set_interlace_handling,
	(ULONG)&png_set_invert_mono,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_background,
	#endif
	(ULONG)&png_set_strip_16,
	(ULONG)&png_set_dither,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_gamma,
	#endif
	(ULONG)&png_permit_empty_plte,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_flush,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_flush,
	#endif
	(ULONG)&png_start_read_image,
	(ULONG)&png_read_update_info,
	(ULONG)&png_read_rows,
	(ULONG)&png_read_row,
	(ULONG)&png_read_image,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_row,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_rows,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_image,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_end,
	#endif
	(ULONG)&png_read_end,
	(ULONG)&png_destroy_info_struct,
	(ULONG)&png_destroy_read_struct,
	(ULONG)&png_read_destroy,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_destroy_write_struct,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_destroy,
	#endif
	(ULONG)&png_set_crc_action,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_filter,
	#endif
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_filter_heuristics,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_compression_level,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_compression_mem_level,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_compression_strategy,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_compression_window_bits,
	#endif
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_compression_method,
	#endif
	(ULONG)&png_set_error_fn,
	(ULONG)&png_get_error_ptr,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_write_fn,
	#endif
	(ULONG)&png_set_read_fn,
	(ULONG)&png_get_io_ptr,
	(ULONG)&png_set_read_status_fn,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_write_status_fn,
	#endif
	(ULONG)&png_set_mem_fn,
	(ULONG)&png_get_mem_ptr,
	(ULONG)&png_set_read_user_transform_fn,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_write_user_transform_fn,
	#endif
	(ULONG)&png_set_user_transform_info,
	(ULONG)&png_get_user_transform_ptr,
	(ULONG)&png_set_read_user_chunk_fn,
	(ULONG)&png_get_user_chunk_ptr,
	(ULONG)&png_malloc,
	(ULONG)&png_free,
	(ULONG)&png_zalloc,
	(ULONG)&png_zfree,
	(ULONG)&png_free_data,
	(ULONG)&png_data_freer,
	(ULONG)&png_malloc_default,
	(ULONG)&png_free_default,
	(ULONG)&png_memcpy_check,
	(ULONG)&png_memset_check,
	(ULONG)&png_error,
	(ULONG)&png_chunk_error,
	(ULONG)&png_warning,
	(ULONG)&png_chunk_warning,
	(ULONG)&png_get_valid,
	(ULONG)&png_get_rowbytes,
	(ULONG)&png_get_rows,
	(ULONG)&png_set_rows,
	(ULONG)&png_get_channels,
	(ULONG)&png_get_image_width,
	(ULONG)&png_get_image_height,
	(ULONG)&png_get_bit_depth,
	(ULONG)&png_get_color_type,
	(ULONG)&png_get_filter_type,
	(ULONG)&png_get_interlace_type,
	(ULONG)&png_get_compression_type,
	(ULONG)&png_get_pixels_per_meter,
	(ULONG)&png_get_x_pixels_per_meter,
	(ULONG)&png_get_y_pixels_per_meter,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_get_pixel_aspect_ratio,
	#endif
	(ULONG)&png_get_x_offset_pixels,
	(ULONG)&png_get_y_offset_pixels,
	(ULONG)&png_get_x_offset_microns,
	(ULONG)&png_get_y_offset_microns,
	(ULONG)&png_get_signature,
	(ULONG)&png_get_bKGD,
	(ULONG)&png_set_bKGD,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_get_cHRM,
	#endif
	(ULONG)&png_get_cHRM_fixed,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_cHRM,
	#endif
	(ULONG)&png_set_cHRM_fixed,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_get_gAMA,
	#endif
	(ULONG)&png_get_gAMA_fixed,
	#ifdef PNG_NO_FLOATING_POINT_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_gAMA,
	#endif
	(ULONG)&png_set_gAMA_fixed,
	(ULONG)&png_get_hIST,
	(ULONG)&png_set_hIST,
	(ULONG)&png_get_IHDR,
	(ULONG)&png_set_IHDR,
	(ULONG)&png_get_oFFs,
	(ULONG)&png_set_oFFs,
	(ULONG)&png_get_pCAL,
	(ULONG)&png_set_pCAL,
	(ULONG)&png_get_pHYs,
	(ULONG)&png_set_pHYs,
	(ULONG)&png_get_PLTE,
	(ULONG)&png_set_PLTE,
	(ULONG)&png_get_sBIT,
	(ULONG)&png_set_sBIT,
	(ULONG)&png_get_sRGB,
	(ULONG)&png_set_sRGB,
	(ULONG)&png_set_sRGB_gAMA_and_cHRM,
	(ULONG)&png_get_iCCP,
	(ULONG)&png_set_iCCP,
	(ULONG)&png_get_sPLT,
	(ULONG)&png_set_sPLT,
	(ULONG)&png_get_text,
	(ULONG)&png_set_text,
	(ULONG)&png_get_tIME,
	(ULONG)&png_set_tIME,
	(ULONG)&png_get_tRNS,
	(ULONG)&png_set_tRNS,
	#ifndef PNG_NO_READ_sCAL
	//(ULONG)&png_get_sCAL, /* XXX: grr.. those 2 were never part of the libtable.. fix that one day on some major version upgrade */
	#endif
	#ifndef PNG_NO_WRITE_sCAL
	//(ULONG)&png_set_sCAL,
	#endif
	(ULONG)&png_set_keep_unknown_chunks,
	(ULONG)&png_set_unknown_chunks,
	(ULONG)&png_set_unknown_chunk_location,
	(ULONG)&png_get_unknown_chunks,
	(ULONG)&png_handle_as_unknown,
	(ULONG)&png_set_invalid,
	(ULONG)&png_read_png,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_write_png,
	#endif
	(ULONG)&png_get_copyright,
	(ULONG)&png_get_header_ver,
	(ULONG)&png_get_header_version,
	(ULONG)&png_get_libpng_ver,
	#ifdef PNG_NO_WRITE_SUPPORTED
	(ULONG)&LIB_Dummy,
	#else
	(ULONG)&png_set_strip_error_numbers,
	#endif
	0xffffffff,
	FUNCARRAY_END
};
