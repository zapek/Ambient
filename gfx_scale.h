#ifndef AMBIENT_GFX_SCALE_H
#define AMBIENT_GFX_SCALE_H
/*
 * $Id: gfx_scale.h,v 1.5 2007/02/11 22:35:28 fab Exp $
 */

/* private structure, do not use outside of the scaling routines */
struct gsi_info {
	ULONG smod;
	ULONG swidth;
	ULONG sheight;
	UWORD xfrac; /* x fractional value */
	UWORD yfrac; /* y fractional value */
};

enum {
	SCALETAG_Nearest = TAG_USER + 1, /* TRUE/FALSE; nearest neighbour */
	SCALETAG_Bilinear,               /* TRUE/FALSE; bilinear filtering */
	SCALETAG_Bicubic,                /* TRUE/FALSE; bicubic filtering */
	SCALETAG_Average,                /* TRUE/FALSE; average filter, has precedence over bilinear for downsampling < 2x */
	SCALETAG_Scale2x,                /* TRUE/FALSE; allows usage of Scale2x when upsampling, best combined with bilinear. can fail if there's not enough memory */
	SCALETAG_AspectX,                /* (ULONG *); keeps the same aspect ratio as the source (needs SCALETAG_AspectY too). If given NULL it uses the source */
	SCALETAG_AspectY,                /* (ULONG *); keeps the same aspect ratio as the source (needs SCALETAG_AspectX too). If given NULL it uses the source */
};

ULONG v_gfx_scale(APTR sbm, APTR tbm, ULONG txs, ULONG tys, struct TagItem *tags);
ULONG gfx_scale(APTR sbm, APTR tbm, ULONG txs, ULONG tys, ...);

void gfx_scale_calc_aspect(ULONG sxs, ULONG sys, ULONG *txs, ULONG *tys);
void gfx_scale_calc_aspect_constraints(ULONG sxs, ULONG sys, ULONG *txs, ULONG *tys);

#endif /* AMBIENT_GFX_SCALE_H */
