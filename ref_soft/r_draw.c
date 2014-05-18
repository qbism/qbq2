/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// draw.c

#include "r_local.h"

byte *thepalette = (byte *)d_8to24table;

image_t		*draw_chars;				// 8*8 graphic characters

#define COLORLEVELS 64
#define PALBRIGHTS 0  //qb: wow, Q2 doesn't have fullbrights?

//qb: - kmq2 fog variables////////////////////////
// global fog vars w/ defaults
int FogModels[3] = { 1, 2, 3 }; //qb: in gl mode it is GL_LINEAR, GL_EXP, GL_EXP2
float	r_fogdensity;
float	r_fogColor[3];


//=============================================================================

//qb: engoo fog //////////////////////////////////////////////
byte	*fogmap;
qboolean r_fogenabled;


void FogTableRefresh(void)
{

	//int ugly;
	static int		l, c, red, green, blue;
	static float	fogthik, frac, farc;

	byte *colmap;

	//qb: pretty wins, but mostly need to speed up this loop for 'real time' fog changes
	//ugly = (int)sw_transquality->value;

	fogthik = r_fogdensity * 0.01;
	//	Con_Printf ("Fog generating with %f %f %f %f", fogthick, fogcolr, fogcolg, fogcolb);
	colmap = fogmap;

	for (l = COLORLEVELS; l > 0; l--)
	{
		frac = (3.01 - 3.01 * (float)l / (COLORLEVELS)) * fogthik; //qb: increase density, was frac = 2 - 2 * (float)l / (COLORLEVELS);
		farc = 1 - ((frac / 2) * fogthik);
		if (farc < 0)
			farc = 0;
		for (c = 0; c<256 - PALBRIGHTS; c++)
		{
			red = ((int)((float)thepalette[c * 4] * farc) + (r_fogColor[0] / 2 * frac ));
			green = ((int)((float)thepalette[c * 4 + 1] * farc) + (r_fogColor[1] / 2 * frac * fogthik));
			blue = ((int)((float)thepalette[c * 4 + 2] * farc) + (r_fogColor[2] / 2 * frac * fogthik));
			if (red>255)red = 255;
			if (green>255)green = 255;
			if (blue > 255)blue = 255;
			if (red < 0)red = 0;
			if (green < 0)green = 0;
			if (blue < 0)blue = 0;
			//if (!ugly)
			//{
				*colmap++ = FindColor(red, green, blue);
			//}
			//else
			//{
			//	*colmap++ = BestColor(red, green, blue, 1, 254);
			//}
		}
	}
	SetFogMap(); // set the static.
};





/*
================
Draw_FindPic
================
*/
image_t *Draw_FindPic(char *name)
{
	image_t	*image;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		image = R_FindImage(fullname, it_pic);
	}
	else
		image = R_FindImage(name + 1, it_pic);

	return image;
}



void	Draw_8to24(byte *palette)
{
	byte	*pal;
	unsigned r, g, b;
	unsigned v;
	unsigned short i;
	byte	*table;
	float gamma = 0;

	//
	// 8 8 8 encoding
	//
	pal = palette;
	table = (byte *)d_8to24tabble;
	for (i = 0; i<256; i++)
	{
		//		Con_Printf (".");	// loop an indicator
		r = pal[0];
		g = pal[1];
		b = pal[2];

		if (r>255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;
		pal += 3;
		v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		*table++ = v;
	}

	// The 15-bit table we use is actually made elsewhere (it's palmap)

	d_8to24tabble[255] &= 0xffffff;	// 255 is transparent
	//qb: gray is the new black.. d_8to24tabble[0] &= 0x000000;	// black is black
}



// leilei - Colored Lights
byte	palmap2[64][64][64];		// Higher quality for lighting

//Sys_Error("butts");
// this is just a lookup table version of the above

int FindColor(int r, int g, int b)
{
	int		bestcolor;

	if (r > 255)r = 255; if (r < 0)r = 0;
	if (g > 255)g = 255; if (g < 0)g = 0;
	if (b > 255)b = 255; if (b < 0)b = 0;
	bestcolor = palmap2[r >> 3][g >> 3][b >> 3];
	return bestcolor;
}




// o^_^o

/*
===============
BestColor

comes from lumpy
===============
*/


/*
=============
R_CalcPalette

=============

byte		*thepalette;

void R_GetPalette(void)
{
	thepalette = (byte *)d_8to24table;
}
*/

byte BestColor(int r, int g, int b, int start, int stop)
{
	int	i;
	int	dr, dg, db;
	int	bestdistortion, distortion;
	int	berstcolor;
	byte	*pal;

	//
	// let any color go to 0 as a last resort
	//
	// R_GetPalette();
	bestdistortion = 256 * 256 * 4;
	berstcolor = 0;


	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;

	pal = (byte *)thepalette + start * 4;
	for (i = start; i <= stop; i++)
	{
		dr = r - (int)pal[0];
		dg = g - (int)pal[1];
		db = b - (int)pal[2];
		pal += 4;
		distortion = dr*dr + dg*dg + db*db + dr * 5 + dg * 5 + db * 5; //qb: this will increase color sensitity at low brightness.  Added + dr + dg + db
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return i;		// perfect match

			bestdistortion = distortion;
			berstcolor = i;
		}
	}


	return berstcolor;
}


void Draw_InitRGBMap(void)
{
	int		r, g, b;
	float ra, ga, ba, ia;
	int		beastcolor;
	float mypow = 1 / 1.3;
	float mydiv = 200;
	float mysat = 1.6;

	// Make the 18-bit lookup table here
	// This is a HUGE 256kb table, the biggest there is here
	// TODO: Option to enable this 

	{
		Draw_8to24((byte *)d_8to24table);
		for (r = 0; r < 256; r += 4)
		{
			for (g = 0; g < 256; g += 4)
			{
				for (b = 0; b < 256; b += 4)
				{
					// 3dfx gamma hack, trying to match the saturation and gamma of the refgl+3dfxgl combo so many q2 players are familiar with

					ra = pow(r / mydiv, mypow) * mydiv;
					ga = pow(g / mydiv, mypow) * mydiv;
					ba = pow(b / mydiv, mypow) * mydiv;

					ia = (ra * 0.333) + (ga * 0.333) + (ba * 0.333);
					ra = ia + (ra - ia) * mysat;
					ga = ia + (ga - ia) * mysat;
					ba = ia + (ba - ia) * mysat;
					//beastcolor = BestColor (pow(ra / mydiv, mypow) * mydiv, pow(ga / mydiv, mypow) * mydiv, pow(ba / mydiv, mypow) * mydiv, 1, 254);
					beastcolor = BestColor((int)ra, (int)ga, (int)ba, 1, 254);
					//beastcolor = BestColor (ra, ga, ba, 1, 254);
					palmap2[r >> 2][g >> 2][b >> 2] = beastcolor;

				}
			}
		}
	}
}

void GrabAlphamap(void) //qb: based on Engoo
{
	int c, l, r, g, b;
	float ay, ae;
	byte *colmap;

	ay = 0.666667;
	ae = 1.0 - ay;				// base pixels
	colmap = vid.alphamap; // alphamap;

	for (l = 0; l < 256; l++)
	{
		for (c = 0; c < 256; c++)
		{
			r = (int)(((float)thepalette[c * 4] * ae) + ((float)thepalette[l * 4] * ay));
			g = (int)(((float)thepalette[c * 4 + 1] * ae) + ((float)thepalette[l * 4 + 1] * ay));
			b = (int)(((float)thepalette[c * 4 + 2] * ae) + ((float)thepalette[l * 4 + 2] * ay));
			*colmap++ = BestColor(r, g, b, 1, 254); // High quality color tables get best color
		}
	}
}


void GrabColormap(void)  //qb: from super8
{
	int		l, c, red, green, blue;
	float	frac;
	byte *colmap;

	colmap = vid.colormap; // host_colormap;

	// shaded levels
	for (l = 0; l < COLORLEVELS; l++)
	{
		frac = (float)l / (COLORLEVELS - 1);
		frac = 1.0 - (frac);

		for (c = 0; c < 256 - PALBRIGHTS; c++)
		{
			red = (int)((float)thepalette[c * 3] * frac); //+ rscaled);
			green = (int)((float)thepalette[c * 3 + 1] * frac); //+ gscaled);
			blue = (int)((float)thepalette[c * 3 + 2] * frac); // + bscaled);

			// note: 254 instead of 255 because 255 is the transparent color, and we
			// don't want anything remapping to that

			*colmap++ = BestColor(red, green, blue, 1, 254); // High quality color tables get best color
		}
	}
}



/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal(void)
{
	draw_chars = Draw_FindPic("conchars");
	// Knightmare- error out instead of crashing if we can't load this
	if (!draw_chars)
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/conchars.pcx");
	// end Knightmare
}



/*
================
Draw_Char

Draws one 8*8 graphics character
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char(int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	int				drawline;
	int				row, col;

	num &= 255;

	if (num == 32 || num == 32 + 128)
		return;

	if (y <= -8)
		return;			// totally off screen

	//	if ( ( y + 8 ) >= vid.height )
	if ((y + 8) > vid.height)		// PGM - status text was missing in sw...
		return;

#ifdef PARANOID
	if (y > vid.height - 8 || x < 0 || x > vid.width - 8)
		ri.Sys_Error(ERR_FATAL, "Con_DrawCharacter: (%i, %i)", x, y);
	if (num < 0 || num > 255)
		ri.Sys_Error(ERR_FATAL, "Con_DrawCharacter: char %i", num);
#endif

	row = num >> 4;
	col = num & 15;
	source = draw_chars->pixels[0] + (row << 10) + (col << 3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128 * y;
		y = 0;
	}
	else
		drawline = 8;


	dest = vid.buffer + y*vid.rowbytes + x;

	while (drawline--)
	{
		if (source[0] != TRANSPARENT_COLOR)
			dest[0] = source[0];
		if (source[1] != TRANSPARENT_COLOR)
			dest[1] = source[1];
		if (source[2] != TRANSPARENT_COLOR)
			dest[2] = source[2];
		if (source[3] != TRANSPARENT_COLOR)
			dest[3] = source[3];
		if (source[4] != TRANSPARENT_COLOR)
			dest[4] = source[4];
		if (source[5] != TRANSPARENT_COLOR)
			dest[5] = source[5];
		if (source[6] != TRANSPARENT_COLOR)
			dest[6] = source[6];
		if (source[7] != TRANSPARENT_COLOR)
			dest[7] = source[7];
		source += 128;
		dest += vid.rowbytes;
	}
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPicImplementation
=============
*/
void Draw_StretchPicImplementation(int x, int y, int w, int h, image_t	*pic)
{
	byte			*dest, *source;
	int				v, u, sv;
	int				height;
	int				f, fstep;
	int				skip;

	w = (int)(w / 4) * 4; //qb: for DIB, sigh... probably should be 'ifdef DIB'
	if ((x < 0) ||
		(x + w > vid.width) ||
		(y + h > vid.height))
	{
		ri.Sys_Error(ERR_FATAL, "Draw_Pic: bad coordinates");
	}

	height = h;
	if (y < 0)
	{
		skip = -y;
		height += y;
		y = 0;
	}
	else
		skip = 0;

	dest = vid.buffer + y * vid.rowbytes + x;

	for (v = 0; v < height; v++, dest += vid.rowbytes)
	{
		sv = (skip + v)*pic->height / h;
		source = pic->pixels[0] + sv*pic->width;
		if (w == pic->width)
			memcpy(dest, source, w);
		else
		{
			f = 0;
			fstep = pic->width * 0x10000 / w;
			for (u = 0; u < w; u += 4)
			{
				dest[u] = source[f >> 16];
				f += fstep;
				dest[u + 1] = source[f >> 16];
				f += fstep;
				dest[u + 2] = source[f >> 16];
				f += fstep;
				dest[u + 3] = source[f >> 16];
				f += fstep;
			}
		}
	}
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic(int x, int y, int w, int h, char *name)
{
	image_t	*pic;

	pic = Draw_FindPic(name);
	if (!pic)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", name);
		return;
	}
	Draw_StretchPicImplementation(x, y, w, h, pic);
}

/*
=============
Draw_StretchRaw
=============
*/
void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data)
{
	image_t	pic;

	pic.pixels[0] = data;
	pic.width = cols;
	pic.height = rows;
	Draw_StretchPicImplementation(x, y, w, h, &pic);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, char *name)
{
	image_t			*pic;
	byte			*dest, *source;
	int				v, u;
	int				tbyte;
	int				height;

	pic = Draw_FindPic(name);
	if (!pic)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", name);
		return;
	}

	if ((x < 0) ||
		(x + pic->width > vid.width) ||
		(y + pic->height > vid.height))
		return;	//	ri.Sys_Error (ERR_FATAL,"Draw_Pic: bad coordinates");

	height = pic->height;
	source = pic->pixels[0];
	if (y < 0)
	{
		height += y;
		source += pic->width*-y;
		y = 0;
	}

	dest = vid.buffer + y * vid.rowbytes + x;

	if (!pic->transparent)
	{
		for (v = 0; v < height; v++)
		{
			memcpy(dest, source, pic->width);
			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{
		if (pic->width & 7)
		{	// general
			for (v = 0; v < height; v++)
			{
				for (u = 0; u < pic->width; u++)
				if ((tbyte = source[u]) != TRANSPARENT_COLOR)
					dest[u] = tbyte;

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v = 0; v < height; v++)
			{
				for (u = 0; u < pic->width; u += 8)
				{
					if ((tbyte = source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
					if ((tbyte = source[u + 1]) != TRANSPARENT_COLOR)
						dest[u + 1] = tbyte;
					if ((tbyte = source[u + 2]) != TRANSPARENT_COLOR)
						dest[u + 2] = tbyte;
					if ((tbyte = source[u + 3]) != TRANSPARENT_COLOR)
						dest[u + 3] = tbyte;
					if ((tbyte = source[u + 4]) != TRANSPARENT_COLOR)
						dest[u + 4] = tbyte;
					if ((tbyte = source[u + 5]) != TRANSPARENT_COLOR)
						dest[u + 5] = tbyte;
					if ((tbyte = source[u + 6]) != TRANSPARENT_COLOR)
						dest[u + 6] = tbyte;
					if ((tbyte = source[u + 7]) != TRANSPARENT_COLOR)
						dest[u + 7] = tbyte;
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, char *name)
{
	int			i, j;
	byte		*psrc;
	byte		*pdest;
	image_t		*pic;
	int			x2;

	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;
	if (w <= 0 || h <= 0)
		return;

	pic = Draw_FindPic(name);
	if (!pic)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", name);
		return;
	}
	x2 = x + w;
	pdest = vid.buffer + y*vid.rowbytes;
	for (i = 0; i < h; i++, pdest += vid.rowbytes)
	{
		psrc = pic->pixels[0] + pic->width * ((i + y) & 63);
		for (j = x; j < x2; j++)
			pdest[j] = psrc[j & 63];
	}
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c)
{
	byte			*dest;
	int				u, v;

	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (w < 0 || h < 0)
		return;
	dest = vid.buffer + y*vid.rowbytes + x;
	for (v = 0; v < h; v++, dest += vid.rowbytes)
	for (u = 0; u < w; u++)
		dest[u] = c;
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
	int			x, y;
	byte		*pbuf;
	int	t;

	for (y = 0; y < vid.height; y++)
	{
		pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
		t = (y & 1) << 1;

		for (x = 0; x < vid.width; x++)
		{
			if ((x & 3) != t)
				pbuf[x] = 0;
		}
	}
}
