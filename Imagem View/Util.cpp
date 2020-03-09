#include "stdafx.h"
#include "Imagem View.h"
#include "Util.h"

#include <stdio.h>
#include <setjmp.h>
#include "../../../jpeg-9d/jpeglib.h"
#include "../../../lpng1637/png.h"
#include "../../../zlib/zlib.h"

#pragma comment(lib, "Msimg32.lib")

//#define _LIB_JEPG_DLL_IMPORT
#ifdef _LIB_JEPG_DLL_IMPORT
#ifdef _DEBUG
#pragma comment(lib, "Jpeg9d_d.lib")
#pragma comment(lib, "libpng16_d.lib")
#pragma comment(lib, "zlib_d.lib")
#else
#pragma comment(lib, "Jpeg9d.lib")
#pragma comment(lib, "libpng16.lib")
#pragma comment(lib, "zlib.lib")
#endif // DEBUG
#else // not _LIB_JEPG_DLL_IMPORT
#ifdef _DEBUG
#pragma comment(lib, "st/Jpeg9d_d.lib")
#pragma comment(lib, "st/libpng16_d.lib")
#pragma comment(lib, "st/zlib_d.lib")
#else
#pragma comment(lib, "st/Jpeg9d.lib")
#pragma comment(lib, "st/libpng16.lib")
#pragma comment(lib, "st/zlib.lib")
#endif // DEBUG
#endif // _LIB_JEPG_DLL_IMPORT
 

long fSize(FILE* f) {
	long t, o;
	o = ftell(f);
	fseek(f, 0, SEEK_END);
	t = ftell(f);
	fseek(f, o, SEEK_SET);
	//char text[100];
	//sprintf(text,"%i %i",sizeof(slong), sizeof(long));
	//MessageBox(0,text,"sizeof slong",0);
	return(t);
}

void img_basis::unflip() {
	if (!pixels || !xres || !yres || !bpp || !dataSize)
		//printf(0, 0);
		return;
	if (flip & 0x1) {
		byte* pi = (byte*)pixels;
		byte* pf = (byte*)pixels + dataSize - xres * (bpp / 8);
		byte* tmp = new byte[xres * (bpp / 8)];
		//if (!tmp) return;
		for (int i = 0; (i < yres) && (pi < pf); i++) {
			memcpy(tmp, pf, xres * (bpp / 8));
			memcpy(pf, pi, xres * (bpp / 8));
			memcpy(pi, tmp, xres * (bpp / 8));
			pi += xres * (bpp / 8);
			pf -= xres * (bpp / 8);
		}
		delete[] tmp;
	}
}

LPSTR removeEndL(LPSTR str, uint size) {
	int sz = size;
	while (--sz > 0) {
		if (str[sz] == 10) {
			str[sz] = 0;
		}
		if (str[sz] == 13) {
			str[sz] = 0;
		}
	}
	return str;
}



// TGA
BOOL isInvalidTGA(TGA* h) {
	//return 0;
	// next modification, translate ifs for switch to make more readable and ease to maintain and add more opitions to suported file format // not needed anymore, switch on loader.
	// it is only for initial security and later testes on unknown files.
	if (!(h->color_type == 0 || h->color_type == 1) ||
		(h->xres == 0 || h->yres == 0 || h->color_start != 0) ||
		!(h->bpp == 8 || h->bpp == 15 || h->bpp == 16 || h->bpp == 24 || h->bpp == 31 || h->bpp == 32) ||
		!(h->image_type == 1 || h->image_type == 2 || h->image_type == 3 || h->image_type == 9 || h->image_type == 10 || h->image_type == 11) ||
		!(h->color_length == 0 || h->color_length == 256) ||
		!(h->color_depth == 0 || h->color_depth == 16 || h->color_depth == 24) //&&
		// !(h->orientation == 0 || h->orientation == 1 || h->orientation == 8 ) // not required
		)
		return 1;//IMG_ERRO_UNSUPORTED_FORMAT;
	return 0;
}

BOOL isUnsuportedTGA(TGA* h) {
	//return 0;
	// second verirification (logic)
	if ((h->color_type == 1 &&
		(!(h->image_type == 1 || h->image_type == 9) || h->color_length < 1 || !(h->color_depth == 16 || h->color_depth == 24) || !(h->bpp == 8 || h->bpp == 16))) ||
		(h->image_type == 3 && h->bpp != 8)
		)
		return 1;
	return 0;
}

TGA * read_TGA_h(FILE* f) {

	if (!f) 
		return 0;
	TGA * h = new TGA;
	if (fread(h, 1, sizeof(TGA), f) != sizeof(TGA)) {
		delete h;
		return 0;
	}
	if (isInvalidTGA(h) || isUnsuportedTGA(h)) {
		delete h;
		return 0;
	}
	return h;
}

byte decodeTGA_RLE(byte* pData, byte bytes, size_t dataSize, FILE* f) {
	ulong cnt = 0;
	ulong  data = 0;
	int	len = 0;
	byte tmp = 0;
	long l = 0;
	while (cnt * bytes < dataSize)
	{
		//tmp = fgetc(f);
		tmp = 0;
		l = fgetc(f);
		if (l < 0)
		{
			//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
			return 1;
		}
		tmp = (byte)l;
		if ((tmp & 0x80))
		{
			len = tmp & 0x7F;
			//if (fread(&data, 1, bytes, f) != bytes)
			l = fread(&data, 1, bytes, f);
			if (l != bytes)
			{
				//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
				return 0;
			}
			while ((len >= 0) && (cnt * bytes < dataSize))
			{
				memcpy(pData, &data, bytes);
				pData += bytes;
				len--;
				cnt++;
			}
		}
		else
		{
			len = tmp & 0x7F;
			while ((len >= 0) && (cnt * bytes < dataSize))
			{
				//if (fread(pData, 1, bytes, f) != bytes)
				l = fread(pData, 1, bytes, f);
				if (l != bytes)
				{
					//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
					return 0;
				}
				pData += bytes;
				len--;
				cnt++;
			}
		}
	}
	return 0;
}

img_basis* read_TGA_header(FILE* f) {
	if (!f)
		return 0;
	TGA* h = new TGA;
	if (fread(h, 1, sizeof(TGA), f) != sizeof(TGA)) {
		delete h;
		return 0;
	}
	if (isInvalidTGA(h) || isUnsuportedTGA(h)) {
		delete h;
		return 0;
	}
	img_basis* img = new img_basis;
	img->xres = h->xres;
	img->yres = h->yres;
	img->bpp = h->bpp;
	img->flip =  (h->orientation & 0x20) ? 1 : 0;
	img->flip += (h->orientation & 0x10) ? 2 : 0;
	img->chanels = h->bpp / 8;
	img->zres = 1;
	img->palSize = h->pal_length * h->pal_depth / 8;
	return img;
}

// initial TGA image loader 583 lines
img_basis* loadTGA(FILE* f) {
	long fp = ftell(f);

	TGA* h = read_TGA_h(f);
	if (!h) {
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	img_basis* img = new img_basis;
	if (!img) {
		fseek(f, fp, SEEK_SET);
		delete h;
		return 0;
	}
	uchar ret = 0,
		inv = 0,
		* pr = 0, * pg = 0, * pb = 0, * pa = 0,
		* bpal = 0,
		* pbytes = 0,
		* data = 0,
		* pdata = 0; // data is stored here
	//FILE* f = 0;
	int r = 0, g = 0, b = 0,
		cnt1 = 0, cnt2 = 0, tmp = 0;
	size_t size = 0;
	ULONG l = 0;
	img->xres = h->x;
	img->yres = h->y;
	if (h->bpp == 15)
		h->bpp = 16;
	img->bpp = h->bpp;
	img->chanels = h->bpp / 8;
	if (h->pal_length && h->color_depth && h->color_type)
		img->palSize = h->pal_length * (h->color_depth / 8);

	img->flip = (h->orientation & 0x20) ? 1 : 0;
	img->flip += (h->orientation & 0x10) ? 2 : 0;

	inv = (h->orientation & 0x20) ? 1 : 0;
	img->orientation = h->orientation;
	if (h->id_length) {
		img->id = new byte[h->id_length + 1];
		if (!img->id) {
			delete h;
			delete img;
			return 0;
		}
		img->id[h->id_length] = 0;
		fread(img->id, 1, h->id_length, f);
	}
	if (h->pal_length || h->pal_depth) {
		if (h->pal_depth == 15) h->pal_depth = 16;
		img->palSize = h->pal_length * h->pal_depth / 8;
		img->pal = new byte[img->palSize];
		if (!img->pal) {
			delete h;
			delete img;
			return 0;
		}
		fread(img->pal, 1, img->palSize, f);
	}


	switch (h->image_type) // win hbitmap = BGRA_8 BGR_8
	{
		case 1: // 1=raw index pallete // OK
		{
			switch (h->bpp)
			{
			case 8: // DT_IDX_8
			{
				img->dataSize = h->x * h->y * 1;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				if (h->pal_depth == 16) {
					rgb_565* pal = (rgb_565*)img->pal;
					for (int i = 0; i < h->x * h->y; i++) {
						data[i * 4 + 0] = pal[img->pixels[i]].r << 4;
						data[i * 4 + 1] = pal[img->pixels[i]].g << 3;
						data[i * 4 + 2] = pal[img->pixels[i]].b << 4;
						data[i * 4 + 3] = 255;
					}
				}
				else {
					BGR_8* pal = (BGR_8*)img->pal;
					for (int i = 0; i < h->x * h->y; i++) {
						data[i * 4 + 0] = pal[img->pixels[i]].b;
						data[i * 4 + 1] = pal[img->pixels[i]].g;
						data[i * 4 + 2] = pal[img->pixels[i]].r;
						data[i * 4 + 3] = 255;
					}
				}
				delete img->pal;
				img->pal = 0;
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 16: // suported now // // DT_IDXA_8
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
					data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
					data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
					data[i * 4 + 3] = img->pal[img->pixels[i * 2 + 1] + 2];
				}
				delete img->pal;
				img->pal = 0;
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		case 2: // 2=raw rgb rgba // OK
		{
			switch (h->bpp)
			{
			case 16: // rgba5551 // OK // DT_B5G5R5A1
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				rgba_5551* x = (rgba_5551*)img->pixels;
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = (byte)x[i].r << 3;
					data[i * 4 + 1] = (byte)x[i].g << 3;
					data[i * 4 + 2] = (byte)x[i].b << 3;
					data[i * 4 + 3] = (byte)x[i].a << 7;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 24: // RGB // OK // DT_BGR_8
			{
				img->dataSize = h->x * h->y * 3;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pixels[i * 3 + 0];
					data[i * 4 + 1] = img->pixels[i * 3 + 1];
					data[i * 4 + 2] = img->pixels[i * 3 + 2];
					data[i * 4 + 3] = 255;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 32: // RGBA // OK // DT_BGRA_8
			{
				img->dataSize = h->x * h->y * 4;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				img->bpp = 32;
				delete h;
				return img;
			}
			case 31: // RGBA // OK // DT_RGBA_8
			{
				img->pixels = new byte[(h->x * h->y * 4)];
				if (!img->pixels)
				{
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, (h->x * h->y * 4), f);
				img->dataSize = h->x * h->y * 4;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		case 3: // 3=raw grey // OK
		{
			switch (h->bpp)
			{
			case 8: // A // DT_L_8
			{
				img->dataSize = h->x * h->y * 1;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pixels[i];
					data[i * 4 + 1] = img->pixels[i];
					data[i * 4 + 2] = img->pixels[i];
					data[i * 4 + 3] = 255;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 16: // LA // DT_LA_8
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				l = fread(img->pixels, 1, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pixels[i];
					data[i * 4 + 1] = img->pixels[i];
					data[i * 4 + 2] = img->pixels[i];
					data[i * 4 + 3] = img->pixels[i + 1];
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		case 9:// 9=rle idx // OK
		{
			switch (h->bpp)
			{
			case 8: // IDX // OK // DT_IDX_8
			{
				img->dataSize = h->x * h->y * 1;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}

				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
				if (h->pal_depth == 16) {
					rgba_5551* pal = (rgba_5551*)img->pal;
					uint i = 0;
					for (i = 0; i < uint(h->x * h->y); i++) {	// edit
						data[i * 4 + 0] = pal[(img->pixels[i])].r << 3;
						data[i * 4 + 1] = pal[(img->pixels[i])].g << 3;
						data[i * 4 + 2] = pal[(img->pixels[i])].b << 3;
						data[i * 4 + 3] = 255;
					}
				}
				if (h->pal_depth == 24) {
					BGR_8* pal = (BGR_8*)img->pal;
					uint i = 0;
					for (i = 0; i < uint(h->x * h->y); i++) {	// edit
						data[i * 4 + 0] = pal[(img->pixels[i])].r;
						data[i * 4 + 1] = pal[(img->pixels[i])].g;
						data[i * 4 + 2] = pal[(img->pixels[i])].b;
						data[i * 4 + 3] = 255;
					}
				}
				delete img->pal;
				img->pal = 0;
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 16: // AIDX // OK // DT_IDXA_8
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
					data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
					data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
					data[i * 4 + 3] = img->pixels[i * 2 + 1];
				}
				delete img->pal;
				img->pal = 0;
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		case 10: // 10=rle rgb rgba // OK
		{
			switch (h->bpp)
			{
			case 16: // rgba5551 // OK // DT_B5G5R5A1
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
				rgba_5551* x = (rgba_5551*)img->pixels;
				for (uint i = 0; i < uint(h->x * h->y); i++) {
					data[i * 4 + 0] = (byte)x[i].r << 3;
					data[i * 4 + 1] = (byte)x[i].g << 3;
					data[i * 4 + 2] = (byte)x[i].b << 3;
					data[i * 4 + 3] = (byte)x[i].a << 7;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 24: // rgb // OK // DT_BGR_8
			{
				img->dataSize = h->x * h->y * 3;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 3, img->dataSize, f);
				for (uint i = 0; i < uint(h->x * h->y); i++) {
					data[i * 4 + 0] = img->pixels[i * 3 + 0];
					data[i * 4 + 1] = img->pixels[i * 3 + 1];
					data[i * 4 + 2] = img->pixels[i * 3 + 2];
					data[i * 4 + 3] = 255;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 32: // rgba // OK // DT_BGRA_8
			{
				img->dataSize = h->x * h->y * 4;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 4, img->dataSize, f);
				//for (int i = 0; i < h->x * h->y * 4; i += 4) {
					//byte tmp = img->pixels[i];
					//img->pixels[i + 1] = img->pixels[i + 3];
					//img->pixels[i + 2] = img->pixels[i + 0];
					//img->pixels[i + 2] = img->pixels[i + 3];
					//img->pixels[i + 3] = tmp;
				// }
				img->bpp = 32;
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		case 11: // 11=rle grey // OK
		{
			switch (h->bpp)
			{
			case 8: // A // OK // DT_L_8
			{
				img->dataSize = h->x * h->y * 1;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pixels[i];
					data[i * 4 + 1] = img->pixels[i];
					data[i * 4 + 2] = img->pixels[i];
					data[i * 4 + 3] = 255;
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			case 16: // LA // OK // DT_LA_8
			{
				img->dataSize = h->x * h->y * 2;
				img->pixels = new byte[(img->dataSize)];
				if (!img->pixels) {
					delete h;
					delete img;
					return 0;
				}
				byte* data = new byte[h->x * h->y * 4];
				if (!data) {
					delete h;
					delete img;
					return 0;
				}
				decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pixels[i * 2];
					data[i * 4 + 1] = img->pixels[i * 2];
					data[i * 4 + 2] = img->pixels[i * 2];
					data[i * 4 + 3] = img->pixels[i * 2 + 1];
				}
				delete img->pixels;
				img->pixels = data;
				img->bpp = 32;
				img->dataSize = h->x * h->y * (img->bpp / 8);
				delete h;
				return img;
			}
			default:
			{
				delete h;
				delete img;
				return 0;
			}
			}
		}
		default:
		{
			delete h;
			delete img;
			return 0;
		}
	}
	return 0;
}

// more advanced function to load TGA image witout hardcoded switchs 215 lines
img_basis* loadTGA2(FILE* f) { // Good code removal
	TGA* h = 0;
	size_t size = 0;
	img_basis* img = 0;
	byte* data = 0;
	ulong l = 0;
	long fp = 0;
	fp = ftell(f);
	h = read_TGA_h(f);
	if (!h) {
		printf("*** UNSUPORTED TARGA FILE ***\n\n");
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	if (true) {
		printf("*** Loading TARGA FILE ***\n id_length   = %i\n color_type  = %i\n image_type  = %i\n "
			   "color_start = %i\n pal_length  = %i\n pal_depth   = %i\n "
			   "x_offset = %i\n y_offset = %i\n xres = %i\n yres = %i\n "
			   "bpp  = %i\n orientation = %i\n *** *** ***\n",
			   h->id_length, h->color_type, h->image_type,
			   h->color_start, h->pal_length, h->pal_depth,
			   h->x_offset, h->y_offset, h->xres, h->yres,
			   h->bpp, h->orientation);
	}
	img = new img_basis;
	if (!img) {
		fseek(f, fp, SEEK_SET);
		delete h;
		return 0;
	}
	img->xres = h->x;
	img->yres = h->y;
	if (h->bpp == 15)
		h->bpp = 16;
	if (h->bpp == 31)
		h->bpp = 32;
	img->bpp = h->bpp;
	img->chanels = h->bpp / 8;
	if (h->pal_length && h->color_depth && h->color_type)
		img->palSize = h->pal_length * (h->color_depth / 8);
	img->flip = (h->orientation & 0x20) ? 1 : 0;
	img->flip += (h->orientation & 0x10) ? 2 : 0;
	img->orientation = h->orientation;
	if (h->id_length) {
		img->id = new byte[h->id_length + 1];
		if (!img->id) {
			delete h;
			delete img;
			return 0;
		}
		img->id[h->id_length] = 0;
		fread(img->id, 1, h->id_length, f);
	}
	if (h->pal_length || h->pal_depth) {
		if (h->pal_depth == 15) h->pal_depth = 16;
		img->palSize = h->pal_length * h->pal_depth / 8;
		img->pal = new byte[img->palSize];
		if (!img->pal) {
			delete h;
			delete img;
			fseek(f, fp, SEEK_SET);
			return 0;
		}
		fread(img->pal, 1, img->palSize, f);
	}
	img->dataSize = h->x * h->y * h->bpp / 8;
	img->pixels = new byte[(img->dataSize)];
	if (!img->pixels) {
		delete h;
		delete img;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	if (h->bpp != 32) {
		data = new byte[h->x * h->y * 4];
		if (!data) {
			delete h;
			delete img;
			fseek(f, fp, SEEK_SET);
			return 0;
		}
	}
	if (h->image_type >= 1 && h->image_type <= 3)
		l = fread(img->pixels, 1, img->dataSize, f);
	else if (h->image_type >= 9 && h->image_type <= 11)
		decodeTGA_RLE(img->pixels, h->bpp / 8, img->dataSize, f);
	else {
		delete h;
		delete img;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	if (img->pal) {
		if (h->bpp == 8) {
			if (h->pal_depth == 16) {
				rgb_565* pal = (rgb_565*)img->pal;
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = pal[img->pixels[i]].r << 4;
					data[i * 4 + 1] = pal[img->pixels[i]].g << 3;
					data[i * 4 + 2] = pal[img->pixels[i]].b << 4;
					data[i * 4 + 3] = 255;
				}
			}
			else {
				BGR_8* pal = (BGR_8*)img->pal;
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = pal[img->pixels[i]].b;
					data[i * 4 + 1] = pal[img->pixels[i]].g;
					data[i * 4 + 2] = pal[img->pixels[i]].r;
					data[i * 4 + 3] = 255;
				}
			}
			delete img->pal;
			img->pal = 0;
		}
		else if (h->bpp == 16) {
			if (h->pal_depth == 16) {
				rgb_565* pal = (rgb_565*)img->pal;
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
					data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
					data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
					data[i * 4 + 3] = img->pal[img->pixels[i * 2 + 1] + 2];
				}
				img->hasAlpha = 1;
			}
			else {
				BGR_8* pal = (BGR_8*)img->pal;
				for (int i = 0; i < h->x * h->y; i++) {
					data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
					data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
					data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
					data[i * 4 + 3] = img->pal[img->pixels[i * 2 + 1] + 2];
				}
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h->x * h->y * (img->bpp / 8);
			delete h;
			return img;
		}
	}
	else {
		if ((h->image_type == 2 || h->image_type == 10) && h->bpp == 16) {
			rgba_5551* x = (rgba_5551*)img->pixels;
			for (int i = 0; i < h->x * h->y; i++) {
				data[i * 4 + 0] = (byte)x[i].r << 3;
				data[i * 4 + 1] = (byte)x[i].g << 3;
				data[i * 4 + 2] = (byte)x[i].b << 3;
				data[i * 4 + 3] = (byte)x[i].a << 7;
			}
			img->hasAlpha = 1;
		}
		else {
			switch (h->bpp) {
				case 8:
				{
					for (int i = 0; i < h->x * h->y; i++) {
						data[i * 4 + 0] = img->pixels[i];
						data[i * 4 + 1] = img->pixels[i];
						data[i * 4 + 2] = img->pixels[i];
						data[i * 4 + 3] = 255;
					}
					break;
				}
				case 16:
				{
					for (int i = 0; i < h->x * h->y; i++) {
						data[i * 4 + 0] = img->pixels[i];
						data[i * 4 + 1] = img->pixels[i];
						data[i * 4 + 2] = img->pixels[i];
						data[i * 4 + 3] = img->pixels[i + 1];
					}
					img->hasAlpha = 1;
					break;
				}
				case 24:
				{
					for (int i = 0; i < h->x * h->y; i++) {
						data[i * 4 + 0] = img->pixels[i * 3 + 0];
						data[i * 4 + 1] = img->pixels[i * 3 + 1];
						data[i * 4 + 2] = img->pixels[i * 3 + 2];
						data[i * 4 + 3] = 255;
					}
					break;
				}
				case 32:
				{
					img->hasAlpha = 1;
					//delete[] data;
					//data = 0;
					break;
				}
				default:
				{
					delete h;
					delete img;
					delete[] data;
					return 0;
					break;
				}
			}
		}
	}
	if (img->pixels && data) {
		delete img->pixels;
		img->pixels = data;
	}
	img->bpp = 32;
	img->dataSize = h->x * h->y * (img->bpp / 8);
	delete h;
	return img;
}



// JPEG
struct my_error_mgr { // JPEG error manager, needed in any error in jpeg read
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

// JPEG error function to avoid JPEG lib call exit() and shut down te entire program
void my_error_exit(j_common_ptr cinfo) { 
	my_error_mgr* myerr = (my_error_mgr*)cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

// read JPEG image info using img_basis to get only header info
img_basis* read_JPEG_header(FILE* f) {
	if (!f)
		return 0;
	img_basis* img = new img_basis;
	if (!img)
		return 0;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		delete img;
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);
	(void)jpeg_read_header(&cinfo, TRUE);
	img->dataSize = cinfo.output_width * cinfo.output_height * cinfo.output_components;
	img->chanels = cinfo.output_components;
	img->xres = cinfo.output_width;
	img->yres = cinfo.output_height;
	img->zres = 1;
	img->bpp = cinfo.output_components * 8;
	jpeg_destroy_decompress(&cinfo);
	return img;
}

// read JPEG image info and pixel data
img_basis* read_JPEG_file(FILE* f) {
	long fp = ftell(f);
	if (!f)
		return 0;
	img_basis* img = new img_basis;
	if (!img) {
		return 0;
	}
	byte* data = 0;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	//byte* buffer = 0;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		fseek(f, fp, SEEK_SET);
		jpeg_destroy_decompress(&cinfo);
		delete img;
		if (data)
			delete[] data;
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);
	(void)jpeg_read_header(&cinfo, TRUE);
	(void)jpeg_start_decompress(&cinfo);
	//cinfo.do_block_smoothing = 1;
	row_stride = cinfo.output_width * cinfo.output_components;
	img->dataSize = cinfo.output_width * cinfo.output_height * cinfo.output_components;
	img->pixels = new byte[img->dataSize/*+ cinfo.output_height*4*/];
	if (!img->pixels) {
		fseek(f, fp, SEEK_SET);
		jpeg_destroy_decompress(&cinfo);
		delete img;
		return 0;
	}
	img->chanels = cinfo.output_components;
	img->xres = cinfo.output_width;
	img->yres = cinfo.output_height;
	img->zres = 1;
	img->bpp = cinfo.output_components * 8;

	//buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	byte* p = img->pixels;
	while (cinfo.output_scanline < cinfo.output_height) {
		(void)jpeg_read_scanlines(&cinfo, &p, 1);
		//memcpy(p, buffer[0], row_stride);
		p += row_stride;
		//int x = (int)p;
		//p += x % 4;
	}
	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	if (img->chanels == 4) {
		img->hasAlpha = 1;
	}
	else if (img->chanels == 3) {
		data = new byte[img->xres * img->yres * 4];
		if (!data) {
			delete img;
			return 0;
		}
		for (int i = 0; i < img->xres * img->yres; i++) {
			data[i * 4 + 0] = img->pixels[i * 3 + 2];
			data[i * 4 + 1] = img->pixels[i * 3 + 1];
			data[i * 4 + 2] = img->pixels[i * 3 + 0];
			data[i * 4 + 3] = 255;
		}
		delete img->pixels;
		img->pixels = data;
		img->bpp = 32;
		img->chanels = 4;
		img->dataSize = img->xres * img->yres * 4;
	}
	else if (img->chanels == 1) {
		data = new byte[img->xres * img->yres * 4];
		if (!data) {
			delete img;
			return 0;
		}
		for (int i = 0; i < img->xres * img->yres; i++) {
			data[i * 4 + 0] = img->pixels[i];
			data[i * 4 + 1] = img->pixels[i];
			data[i * 4 + 2] = img->pixels[i];
			data[i * 4 + 3] = 255;
		}
		delete img->pixels;
		img->pixels = data;
		img->bpp = 32;
		img->chanels = 4;
		img->dataSize = img->xres * img->yres * 4;
	}
	img->flip = 1;
	return img;
}



// PNG
#define PNG_BYTES_TO_CHECK 8
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->png_jmpbuf)
#endif

int check_if_png(FILE* f){
	char buf[PNG_BYTES_TO_CHECK];
	long p = ftell(f);
	if (fread(buf, 1, PNG_BYTES_TO_CHECK, f) != PNG_BYTES_TO_CHECK) /* Read in some of the signature bytes. */
		return 0;
	fseek(f, p, SEEK_SET);
	/* Compare the first PNG_BYTES_TO_CHECK bytes of the signature. Return nonzero (true) if they match. */
	return(!png_sig_cmp((png_const_bytep)buf, 0, PNG_BYTES_TO_CHECK));
}

img_basis* read_PNG_header(FILE* f) {
	return 0;



}

img_basis* read_PNG_file(FILE* f) {
	return 0;
}

img_basis* read_png(FILE* f/*, int sig_read*/){
	long fp = ftell(f);

	png_structp png_ptr;
	png_infop info_ptr;

	png_uint_32 width, 
		height;
	int bit_depth, 
		color_type, 
		interlace_type;

	img_basis* img = 0;
	int row = 0;
	png_bytep* row_pointers = 0;

	if (!f)
		return 0;
	img = new img_basis;
	if (!img)
		return 0;

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.  REQUIRED.
	 */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
		//png_voidp user_error_ptr, user_error_fn, user_warning_fn);

	if (png_ptr == NULL)
	{
		delete img;
		return 0;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		delete img;
		return 0;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in the png_create_read_struct() earlier.
	 */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* Free all of the memory associated with the png_ptr and info_ptr. */
		fseek(f, fp, SEEK_SET);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		/* If we get here, we had a problem reading the file. */
		delete img;
		if (row_pointers)
			delete[] row_pointers;
		return (0);
	}

	/* One of the following I/O initialization methods is REQUIRED. */
//#ifdef streams /* PNG file I/O method 1 */
   /* Set up the input control if you are using standard C streams. */
	png_init_io(png_ptr, f);

//#else no_streams /* PNG file I/O method 2 */
   /* If you are using replacement read functions, instead of calling
	* png_init_io(), you would call:
	*/
	//png_set_read_fn(png_ptr, (void*)user_io_ptr, user_read_fn);
	/* where user_io_ptr is a structure you want available to the callbacks. */
//#endif no_streams /* Use only one I/O method! */

   /* If we have already read some of the signature */
	//png_set_sig_bytes(png_ptr, sig_read);

//#ifdef hilevel
	/* If you have enough memory to read in the entire image at once,
	 * and you need to specify only transforms that can be controlled
	 * with one of the PNG_TRANSFORM_* bits (this presently excludes
	 * quantizing, filling, setting background, and doing gamma
	 * adjustment), then you can read the entire image (including
	 * pixels) into the info structure with this call:
	 */

	//png_read_png(png_ptr, info_ptr, png_transforms, NULL);

//#else
	/* OK, you're doing it the hard way, with the lower-level functions. */

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED.
	 */
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);

	/* Set up the data transformations you want.  Note that these are all
	 * optional.  Only call them if you want/need them.  Many of the
	 * transformations only work on specific types of images, and many
	 * are mutually exclusive.
	 */

	 /* Tell libpng to strip 16 bits/color files down to 8 bits/color.
	  * Use accurate scaling if it's available, otherwise just chop off the
	  * low byte.
	  */
//#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
	png_set_scale_16(png_ptr);
//#else
//	png_set_strip_16(png_ptr);
//#endif

	/* Strip alpha bytes from the input data without combining with the
	 * background (not recommended).
	 */
	//png_set_strip_alpha(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2 or 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* Change the order of packed pixels to least significant bit first
	 * (not useful if you are using png_set_packing).
	 */
	png_set_packswap(png_ptr);

	/* Expand paletted colors into true RGB triplets. */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	/* Expand grayscale images to the full 8 bits from 1, 2 or 4 bits/pixel. */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	/* Expand paletted or RGB images with transparency to full alpha channels
	 * so the data will be available as RGBA quartets.
	 */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0)
		png_set_tRNS_to_alpha(png_ptr);

	/* Set the background color to draw transparent and alpha images over.
	 * It is possible to set the red, green and blue components directly
	 * for paletted images, instead of supplying a palette index.  Note that,
	 * even if the PNG file supplies a background, you are not required to
	 * use it - you should use the (solid) application background if it has one.
	 */
	//png_color_16 my_background, * image_background;

	//if (png_get_bKGD(png_ptr, info_ptr, &image_background) != 0)
	//	png_set_background(png_ptr, image_background,
	//		PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
	//else
	//	png_set_background(png_ptr, &my_background,
	//		PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

	/* Some suggestions as to how to get a screen gamma value.
	 *
	 * Note that screen gamma is the display_exponent, which includes
	 * the CRT_exponent and any correction for viewing conditions.
	 */
	//if (/* We have a user-defined screen gamma value */)
	//	screen_gamma = user - defined screen_gamma;
	///* This is one way that applications share the same screen gamma value. */
	//else if ((gamma_str = getenv("SCREEN_GAMMA")) != NULL)
	//	screen_gamma = atof(gamma_str);
	///* If we don't have another value */
	//else
	// {
	//	screen_gamma = PNG_DEFAULT_sRGB; /* A good guess for a PC monitor
	//										in a dimly lit room */
	//	screen_gamma = PNG_GAMMA_MAC_18 or 1.0; /* Good guesses for Mac
	//											   systems */
	// }

	/* Tell libpng to handle the gamma conversion for you.  The final call
	 * is a good guess for PC generated images, but it should be configurable
	 * by the user at run time.  Gamma correction support in your application
	 * is strongly recommended.
	 */

	//int intent;

	//if (png_get_sRGB(png_ptr, info_ptr, &intent) != 0)
	//	png_set_gamma(png_ptr, screen_gamma, PNG_DEFAULT_sRGB);
	//else
	// {
	//	double image_gamma;
	//	if (png_get_gAMA(png_ptr, info_ptr, &image_gamma) != 0)
	//		png_set_gamma(png_ptr, screen_gamma, image_gamma);
	//	else
	//		png_set_gamma(png_ptr, screen_gamma, 0.45455);
	// }

//#ifdef PNG_READ_QUANTIZE_SUPPORTED
//	/* Quantize RGB files down to 8-bit palette, or reduce palettes
//	 * to the number of colors available on your screen.
//	 */
//	if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
//	{
//		int num_palette;
//		png_colorp palette;
//
//		/* This reduces the image to the application-supplied palette. */
//		if (/* We have our own palette */)
//		{
//			/* An array of colors to which the image should be quantized. */
//			png_color std_color_cube[MAX_SCREEN_COLORS];
//			png_set_quantize(png_ptr, std_color_cube, MAX_SCREEN_COLORS,
//				MAX_SCREEN_COLORS, NULL, 0);
//		}
//		/* This reduces the image to the palette supplied in the file. */
//		else if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) != 0)
//		{
//			png_uint_16p histogram = NULL;
//			png_get_hIST(png_ptr, info_ptr, &histogram);
//			png_set_quantize(png_ptr, palette, num_palette,
//				max_screen_colors, histogram, 0);
//		}
//	}
//#endif /* READ_QUANTIZE */

	/* Invert monochrome files to have 0 as white and 1 as black. */
	//png_set_invert_mono(png_ptr);

	/* If you want to shift the pixel values from the range [0,255] or
	 * [0,65535] to the original [0,7] or [0,31], or whatever range the
	 * colors were originally in:
	 */
	//if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT) != 0)
	// {
	//	png_color_8p sig_bit_p;
	//	png_get_sBIT(png_ptr, info_ptr, &sig_bit_p);
	//	png_set_shift(png_ptr, sig_bit_p);
	// }

	/* Flip the RGB pixels to BGR (or RGBA to BGRA). */
	if ((color_type & PNG_COLOR_MASK_COLOR) != 0)
		png_set_bgr(png_ptr);

	/* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR). */
	//png_set_swap_alpha(png_ptr);

	/* Swap bytes of 16-bit files to least significant byte first. */
	//png_set_swap(png_ptr);

	/* Add filler (or alpha) byte (before/after each RGB triplet). */
	//png_set_filler(png_ptr, 0xffff, PNG_FILLER_AFTER);

//#ifdef PNG_READ_INTERLACING_SUPPORTED
//	/* Turn on interlace handling.  REQUIRED if you are not using
//	 * png_read_image().  To see how to handle interlacing passes,
//	 * see the png_read_row() method below:
//	 */
	int number_passes = png_set_interlace_handling(png_ptr);
//#else /* !READ_INTERLACING */
//	number_passes = 1;
//#endif /* READ_INTERLACING */

	/* Optional call to gamma correct and add the background to the palette
	 * and update info structure.  REQUIRED if you are expecting libpng to
	 * update the palette for you (i.e. you selected such a transform above).
	 */
	//printf("interlace type: %i\ninterlace number_passes: %i\n", png_get_interlace_type(png_ptr, info_ptr), number_passes);
	png_read_update_info(png_ptr, info_ptr);
	size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	byte* p = NULL;
	/* Allocate the memory to hold the image using the fields of info_ptr. */
	img->chanels = png_get_channels(png_ptr, info_ptr);
	img->xres = width;
	img->yres = height;
	img->bpp = png_get_channels(png_ptr, info_ptr)*8;
	img->dataSize = img->xres * img->yres * img->chanels;
	img->pixels = new byte[png_get_rowbytes(png_ptr, info_ptr) * img->yres];
	if (!img->pixels) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		delete img;
		return 0;
	}
	row_pointers = new png_bytep[height];
	if (!row_pointers) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		delete img;
		return 0;
	}

	//int row = 0;
	//png_bytep* row_pointers;
	//row_pointers = new png_bytep[height];
	//for (row = 0; row < height; row++)
	//	row_pointers[row] = NULL; /* Clear the pointer array */
	//for (row = 0; row < height; row++)
	//	row_pointers[row] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
	

	/* Now it's time to read the image.  One of these methods is REQUIRED. */
//#ifdef entire /* Read the entire image in one go */
	p = img->pixels;
	for (row = 0; row < height; row++) {
		row_pointers[row] = p;
		p += rowbytes;
	}
	png_read_image(png_ptr, row_pointers);
	delete[] row_pointers;
//#else no_entire /* Read the image one or more scanlines at a time */
//   /* The other way to read images - deal with interlacing: */
	//for (int pass = 0; pass < number_passes; pass++)
	// {

//#ifdef single /* Read the image a single row at a time */
	//for (int y = 0; y < height; y++) {
	//	png_read_row(png_ptr, p, NULL);
	//	p += rowbytes;
	// }
	if (1) {
		if (img->chanels == 4) {
			img->hasAlpha = 1;
		}
		if (img->chanels == 3) {
			byte* data = new byte[img->xres * img->yres * 4];
			if (!data) {
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				delete img;
				return 0;
			}
			for (int i = 0; i < img->xres * img->yres; i++) {
				data[i * 4 + 0] = img->pixels[i * 3 + 0];
				data[i * 4 + 1] = img->pixels[i * 3 + 1];
				data[i * 4 + 2] = img->pixels[i * 3 + 2];
				data[i * 4 + 3] = 0;
			}
			delete img->pixels;
			img->pixels = data;
			data = 0;
			img->chanels = 4;
			img->bpp = 32;
			img->dataSize = img->xres * img->yres * 4;
		}
		if (img->chanels == 1) {
			byte* data = new byte[img->xres * img->yres * 4];
			if (!data) {
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				delete img;
				return 0;
			}
			for (int i = 0; i < img->xres * img->yres; i++) {
				data[i * 4 + 0] = img->pixels[i * 1 + 0];
				data[i * 4 + 1] = img->pixels[i * 1 + 0];
				data[i * 4 + 2] = img->pixels[i * 1 + 0];
				data[i * 4 + 3] = 0;
			}
			delete img->pixels;
			img->pixels = data;
			data = 0;
			img->chanels = 4;
			img->bpp = 32;
			img->dataSize = img->xres * img->yres * 4;
		}
		if (img->chanels == 2) {
			byte* data = new byte[img->xres * img->yres * 4];
			if (!data) {
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				delete img;
				return 0;
			}
			for (int i = 0; i < img->xres * img->yres; i++) {
				data[i * 4 + 0] = img->pixels[i * 2 + 0];
				data[i * 4 + 1] = img->pixels[i * 2 + 0];
				data[i * 4 + 2] = img->pixels[i * 2 + 0];
				data[i * 4 + 3] = img->pixels[i * 2 + 1];;
			}
			delete img->pixels;
			img->pixels = data;
			data = 0;
			img->chanels = 4;
			img->bpp = 32;
			img->dataSize = img->xres * img->yres * 4;
		}

	}
//#else no_single /* Read the image several rows at a time */
//		for (y = 0; y < height; y += number_of_rows)
//		{
//#ifdef sparkle /* Read the image using the "sparkle" effect. */
//			png_read_rows(png_ptr, &row_pointers[y], NULL,
//				number_of_rows);
//#else no_sparkle /* Read the image using the "rectangle" effect */
//			png_read_rows(png_ptr, NULL, &row_pointers[y],
//				number_of_rows);
//#endif no_sparkle /* Use only one of these two methods */
//		}
 
//		/* If you want to display the image after every pass, do so here. */
//#endif no_single /* Use only one of these two methods */
	// }
//#endif no_entire /* Use only one of these two methods */

	/* Read rest of file, and get additional chunks in info_ptr.  REQUIRED. */
	png_read_end(png_ptr, info_ptr);
//#endif hilevel

	/* At this point you have read the entire image. */

	/* Clean up after the read, and free any memory allocated.  REQUIRED. */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	/* Close the file. */

	/* That's it! */
	img->flip = 1;
	return img;
}



// PCX reading and saving
static const byte bPCXDefaultPalette[3 * 16] ={
	0,   0,   0,
	0,   0, 255,
	0, 255,   0,
	0, 255, 255,
  255,   0,   0,
  255,   0, 255,
  255, 255,   0,
  255, 255, 255,
   85,  85, 255,
   85,  85,  85,
	0, 170,   0,
  170,   0,   0,
   85, 255, 255,
  255,  85, 255,
  255, 255,  85,
  255, 255, 255
};

void RLEencode(byte* p, uint size, FILE* f) {
	uint last, cont;
	last = (byte) * (p++);
	cont = 1;
	size--;
	while (size-- > 0) {
		uint data;
		data = (byte) * (p++);
		// Up to 63 bytes with the same value can be stored using
		// a single { cont, value } pair.
		if ((data == last) && (cont < 63)) {
			cont++;
		}
		else {
			if ((cont > 1) || ((last & 0xC0) == 0xC0))
				fputc((cont | 0xC0), f); // write a 'counter' byte
			fputc((byte)last, f);
			last = data;
			cont = 1;
		}
	}
	// write the last one and return;
	if ((cont > 1) || ((last & 0xC0) == 0xC0))
		fputc((char)(cont | 0xC0), f);
	fputc((char)last, f);
}

void RLEdecode(byte* p, uint size, FILE* f){
	while (size > 0)	{
		int c = fgetc(f);
			if (c < 0)
				return;
		uint data = (uint)c;
		if ((data & 0xC0) != 0xC0) {// If ((data & 0xC0) != 0xC0), then the value read is a data byte		
			*(p++) = (byte)data;
			size--;
		}
		else { // Then it is a counter (cont = val & 0x3F) and the next byte is the data byte.		
			uint cont = data & 0x3F;
			if (cont > size) // the file is corrupted or malformed
				break;
			c = fgetc(f);
			if (c == EOF)
				return;
			data = (byte)c;
			for (uint i = 1; i <= cont && size > 0; i++)
				*(p++) = (byte)data;
			size -= cont;
		}
	}
}

img_basis* read_PCX_file(FILE* f) {
	long fp = ftell(f);
	byte* p;               // space to store one scanline
	byte* dst;             // pointer into wxImage data
	uint width, height;     // size of the image
	//uint i, j;
	//unsigned int bytesperline;      // bytes per line (each plane)
	//int version;               // bits per pixel (each plane)
	//int bitsperpixel;               // bits per pixel (each plane)
	//int nplanes;                    // number of planes
	//int encoding;                   // is the image RLE encoded?
	int format;                     // image format (8 bit, 24 bit)
	PCX* h = new PCX;
	byte color_bust = 0;
	if (!h)
		return 0;
	fread(h, 1, 128, f);
	//version = h->version;
	//encoding = h->encoding;
	//nplanes = h->nplanes;
	//bitsperpixel = h->bitsperpixel;
	//bytesperline = h->bytesperline;
	width = h->xmax - h->xmin + 1;
	height = h->ymax - h->ymin + 1;
	
	if (true)
	{
		printf(	"*** READING IMAGE PCX FILE  ***\n"
				"*** Manufacturer %i\n"
				"*** Version %i\n"
				"*** Encoding %i\n"
				"*** BitsPerPixel %i\n"
				"*** Xmin %i\n"
				"*** Ymin %i\n"
				"*** Xmax %i\n"
				"*** Ymax %i\n"
				"*** HDpi %i\n"
				"*** VDpi %i\n"
				"*** Reserved %i\n"
				"*** NPlanes %i\n"
				"*** BytesPerLine %i\n"
				"*** PaletteInfo %i\n"
				"*** HscreenSize %i\n"
				"*** VscreenSize %i\n"
				"*** xres %i\n"
				"*** yres %i\n"
				"*** *** *** *** *** *** *** ***\n"
			,
			h->manufacturer,
			h->version,
			h->encoding,
			h->bitsperpixel,
			h->xmin,
			h->ymin,
			h->xmax,
			h->ymax,
			h->hdpi,
			h->vdpi,
			h->reserved,
			h->nplanes,
			h->bytesperline,
			h->paletteinfo,
			h->hscreensize,
			h->vscreensize,
			width,
			height
		);
	}

	if ((h->manufacturer != 0xA) || (h->version > 5)) {
		delete h;
		return 0;
	}
	if ((h->nplanes == 3) && (h->bitsperpixel == 8))
		format = PCX_24BIT;
	else if ((h->nplanes == 4) && (h->bitsperpixel == 8))
		format = PCX_32BIT;
	else if ((h->nplanes == 1) && (h->bitsperpixel == 8))
		format = PCX_8BIT;
	else if ((h->nplanes == 1) && (h->bitsperpixel == 4))
		format = PCX_4BIT;
	else if ((h->nplanes == 1) && (h->bitsperpixel == 2))
		format = PCX_2BIT;
	else if ((h->nplanes == 1) && (h->bitsperpixel == 1))
		format = PCX_1BIT;
	else if ((h->nplanes == 2) && (h->bitsperpixel == 1))
		format = PCX_1BIT_x;
	else if ((h->nplanes == 3) && (h->bitsperpixel == 1))
		format = PCX_1BIT_x;
	else if ((h->nplanes == 4) && (h->bitsperpixel == 1))
		format = PCX_1BIT_x;
	else {
		delete h;
		fseek(f, fp, SEEK_SET);
		return 0;
	}

	img_basis* img = new img_basis;
	if (!img) {
		delete h;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	img->xres = width;
	img->yres = height;
	img->bpp = 32;
	img->dataSize = width * height * 4;
	img->pixels = new byte[width * height * 4];
	if (!img->pixels) {
		delete h;
		delete img;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	img->pal = new byte[768];
	if (!img->pal) {
		delete h;
		delete img;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	p = new unsigned char[h->bytesperline * h->nplanes];
	if (!p){
		delete h;
		delete img;
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	dst = img->pixels;
	memset(img->pixels, 0, img->dataSize);

	for (int j = height; j; j--)
	{
		if (h->encoding)
			RLEdecode(p, h->bytesperline * h->nplanes, f);
		else
			fread(p, 1, h->bytesperline * h->nplanes, f);

		switch (format)
		{
			case PCX_1BIT_x:
			{
				byte* innerSrc = p;
				for (int plane = 0; plane < h->nplanes; plane++) {
					int x = 0;
					for (int bpline = 0; bpline < h->bytesperline; bpline++) {
						byte bits = *innerSrc++;
						for (int k = 7; k >= 0; k--) {
							unsigned bit = (bits >> k) & 1;							
							if (bpline * 8 + k >= width) /* skip padding bits */
								continue;
							dst[4*x] |= bit << plane;
						x++;
						}
					}
				}
				dst += img->xres * 4;
				break;
			}
			case PCX_1BIT:
			case PCX_2BIT:
			case PCX_4BIT:
			{
				for (int i = 0; i < width; i++) {
					if (h->bitsperpixel == 1) {
						*(dst) = ((p[i / 8] >> (7 - i % 8)) & 0x01);
						dst += 4;
					}
					else if (h->bitsperpixel == 2) {
						*(dst) = ((p[i / 4] >> ((3 - i % 4) * 2)) & 0x03);
						dst += 4;
					}
					else if (h->bitsperpixel == 4) {
						*(dst) = ((p[i / 2] >> ((1 - i % 2) * 4)) & 0x0F);
						dst += 4;
					}
				}
				break;
			}
			case PCX_8BIT:
			{
				for (int i = 0; i < width; i++)
				{					
					*dst = p[i];// first pass, just store the colour index
					dst += 4;
				}
				break;
			}
			case PCX_24BIT:
			{
				for (int i = 0; i < width; i++)
				{
					*(dst++) = p[i + 2 * h->bytesperline];
					*(dst++) = p[i + h->bytesperline];
					*(dst++) = p[i];
					*(dst++) = 255;
				}
				break;
			}
			case PCX_32BIT:
			{
				img->hasAlpha = 1;
				for (int i = 0; i < width; i++)
				{
					*(dst++) = p[i + 2 * h->bytesperline];
					*(dst++) = p[i + h->bytesperline];
					*(dst++) = p[i];
					*(dst++) = p[i + 3 * h->bytesperline];;
				}
				break;
			}
		}
	}
	delete[] p;
	int ExtPal = 0;
	if (h->bitsperpixel == 1 && h->nplanes == 1 && h->version != 3)
	{
		if ((h->colormap[0]== h->colormap[3] && h->colormap[1] == h->colormap[4] && h->colormap[2] == h->colormap[5])||
			(h->colormap[0]<17 && h->colormap[3] < 17 && h->colormap[1] < 17 && h->colormap[4] < 17 && h->colormap[2] < 17 && h->colormap[5] < 17)
			) {
			img->pal[0] = 0; img->pal[1] = 0; img->pal[2] = 0;
			img->pal[3] = 255; img->pal[4] = 255; img->pal[5] = 255;
			printf("*** USING MONOCROME 2 COLORS PALETTE ***\n");
		}
		else {
			memcpy(img->pal, bPCXDefaultPalette, 48);
			memcpy(img->pal, h->colormap, 6);
			printf("*** USING PCX 2 COLORS HEADER PALETTE ***\n");
		}
	}
	//else if ((h->bitsperpixel == 1 || h->bitsperpixel == 2 || h->bitsperpixel == 4 ) && (h->nplanes == 1 || h->nplanes == 2 || h->nplanes == 3 || h->nplanes == 4))
	// {
	//	color_bust = h->colormap[0] >>5;
	//	memcpy(img->pal, iPCXDefaultPalette, sizeof(iPCXDefaultPalette));
	//	printf("*** USING DEFAULT 16 COLORS PCX PALETTE ***\n");
	// }
	else{
		ExtPal = fgetc(f);
		printf("ExtPal %i\n", ExtPal);
		if (ExtPal == 12) {
			fread(img->pal, 1, 768, f);
			printf("*** USING EXTENDED 256 COLORS PALETTE ***\n");
		}
		else if (h->version == 3)
		{
			memcpy(img->pal, bPCXDefaultPalette, sizeof(bPCXDefaultPalette));
			printf("*** USING DEFAULT 16 COLORS PCX PALETTE ***\n");
		}
		else if (ExtPal == -1 && h->version == 5 && h->paletteinfo == 2) {
			//for (int i = 0; i < 256; i++) {
			//	img->pal[i * 3 + 0] = i;
			//	img->pal[i * 3 + 1] = i;
			//	img->pal[i * 3 + 2] = i;
			// }
			//printf("*** USING GENERATED GRAYSCALE PALETTE ***\n");
			printf("*** NOT USING PALETTE, USE DE INDEX AS A GRAYSCALE ***\n");
		}
		else if (ExtPal == -1 && h->version != 3) {
			memcpy(img->pal, h->colormap, 48);
			printf("*** USING PCX 16 COLORS HEADER PALETTE ***\n");
		}
		else {
			memcpy(img->pal, bPCXDefaultPalette, sizeof(bPCXDefaultPalette));
			printf("*** USING DEFAULT 16 COLORS PCX PALETTE 2 ***\n");
		}
	}
	if (format == PCX_1BIT || format == PCX_2BIT || format == PCX_4BIT || format == PCX_8BIT || format == PCX_1BIT_x)
	{
		p = img->pixels;
		if (h->version == 5 && h->paletteinfo == 2 && ExtPal == -1) {
			for (unsigned long k = height * width; k; k--)
			{
				unsigned char index;
				index = *p;
				*(p++) = index;
				*(p++) = index;
				*(p++) = index;
				*(p++) = 255;
			}
		}
		//else if(color_bust)
		//	for (unsigned long k = height * width; k; k--)
		//	{
		//		unsigned char index;
		//		index = *p;
		//		*(p++) = img->pal[byte(3 * (index >> color_bust) + 2)];
		//		*(p++) = img->pal[byte(3 * (index >> color_bust) + 1)];
		//		*(p++) = img->pal[byte(3 * (index >> color_bust)    )];
		//		*(p++) = 0;
		//	}
		else 
			for (unsigned long k = height * width; k; k--)
			{
				unsigned char index;
				index = *p;
				*(p++) = img->pal[3 * index + 2];
				*(p++) = img->pal[3 * index + 1];
				*(p++) = img->pal[3 * index];
				*(p++) = 255;
			}
		delete[] img->pal;
		img->pal = 0;
	}
	img->flip = 1;
	delete h;
	return img;
}

//void RLEdecode(unsigned char* p, unsigned int size, FILE* f) {
//	while (size != 0) {
//		unsigned int data = (unsigned char)fgetc(f);
//		if ((data & 0xC0) != 0xC0) {
//			*(p++) = (unsigned char)data;
//			size--;
//		}
//		else {
//			unsigned int cont = data & 0x3F;
//			if (cont > size) // can happen only if the file is malformed
//				break;
//			data = (unsigned char)fgetc(f);
//			for (unsigned int i = 1; i <= cont; i++)
//				*(p++) = (unsigned char)data;
//			size -= cont;
//		}
//	}
//}

//void RLEdecode(unsigned char* p, unsigned int size, FILE* f) {
//	// Read 'size' bytes. The PCX official specs say there will be
//	// a decoding break at the end of each scanline (but not at the
//	// end of each plane inside a scanline). Only use this function
//	// to read one or more _complete_ scanlines. Else, more than
//	// 'size' bytes might be read and the buffer might overflow.
//
//	while (size != 0) {
//		unsigned int data = (unsigned char)fgetc(f);
//
//		// If ((data & 0xC0) != 0xC0), then the value read is a data
//		// byte. Else, it is a counter (cont = val & 0x3F) and the
//		// next byte is the data byte.
//
//		if ((data & 0xC0) != 0xC0) {
//			*(p++) = (unsigned char)data;
//			size--;
//		}
//		else {
//			unsigned int cont = data & 0x3F;
//			if (cont > size) // can happen only if the file is malformed
//				break;
//			data = (unsigned char)fgetc(f);
//			for (unsigned int i = 1; i <= cont; i++)
//				*(p++) = (unsigned char)data;
//			size -= cont;
//		}
//	}
//}

/*
img_basis* IMG_LoadPCX_RW(FILE* f) {
	if (!f)
		return 0;
	char fname[1024];
	FILE* file = 0;
	PCX * h = new PCX;
	if (!h)
		return 0;
	FILE* lst = fopen("lst_PCX.txt", "w");
	if (!lst) {
		delete h;
		return 0;
	}
	while (fgets(fname, 1020, f))
	{
		file = fopen(removeEndL(fname, 1022), "r");
		if (file) {
			if (fread(h, 1, sizeof(PCX), file) == sizeof(PCX)) {
				int width = h->xmax - h->xmin + 1;
				int height = h->ymax - h->ymin + 1;
				if (true){
					fprintf(lst,
						//"Manufacturer %i		"
						//"Version %i		"
						//"Encoding %i		"
						//"BitsPerPixel %i		"
						//"Xmin %i		"
						//"Ymin %i		"
						//"Xmax %i		"
						//"Ymax %i		"
						//"HDpi %i		"
						//"VDpi %i		"
						//"Reserved %i		"
						//"NPlanes %i		"
						//"BytesPerLine %i		"
						//"PaletteInfo %i		"
						//"HscreenSize %i		"
						//"VscreenSize %i		"
						//"width %i		"
						//"height %i		"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%i	"
						"%s\n"
						,
						h->manufacturer,
						h->version,
						h->encoding,
						h->bitsperpixel,
						h->xmin,
						h->ymin,
						h->xmax,
						h->ymax,
						h->hdpi,
						h->vdpi,
						h->reserved,
						h->nplanes,
						h->bytesperline,
						h->paletteinfo,
						h->hscreensize,
						h->vscreensize,
						width,
						height,
						fname
					);

				}
			}
			fclose(file);
		}
	}
	delete h;
	fclose(lst);
	return 0;
}   
*/


// PNM

// skip line and return the last character, if End Of File return -1
int skipLine(FILE* f) {
	int c = 0;
	while (true) {
		c = fgetc(f); // get one character from file
		if (c==EOF) // end of file
			return -1;
		if (c == '\n' || c == '\r') // new line or line return
			break; // goto next loop
	}
	while (true) {
		c = fgetc(f);
		if (c==EOF)
			return -1;
		if (c == '\n' || c == '\r')
			continue;
		else
		{
			fseek(f, -1, SEEK_CUR);
			return 0;
		}
	}
}

int skipComent(FILE* f) {
	int c = 0;
	while (true) {
		c = fgetc(f);
		if (c == '#') {
			if (skipLine(f)==-1) // End Of File
				return -1;
			continue; // restart loop
		}
		else {
			fseek(f, -1, SEEK_CUR);
			return 0;
		}
	}
}

int readInt(FILE* f) {
	int c = 0;
	int n = 0;
	int count = 0;
	while (true) {
		c = fgetc(f);
		if (c >= '0' && c <= '9') {
			n *= 10;
			n += c - '0';
			count++;
		}
		else if (c == ' ' || c == '	' || c == '\n' || c == '\r')
			if (count > 0) {
				fseek(f, -1, SEEK_CUR);
				return n;
			}
			else
				continue;
		else if (c == EOF)
			if (count == 0)
				return -1;
			else
				return n;
		else if (c == '#'){
			if (count == 0) {
				skipLine(f);
			}
			else {
				fseek(f, -1, SEEK_CUR);
				return n;
			}
		}
	}
}

img_basis* read_PNM_file(FILE* f) {
	long fp = ftell(f);
	int compression = 0;
	int c = 0;
	int xres = 0, yres = 0, bpp = 0, maxval=0;
	img_basis* img = 0;
	printf("*** Reading PNM file ***\n");
	if (!f)
		return 0;
	c = fgetc(f);
	if (c == EOF || c != 'p' && c != 'P') {
		printf("*** NOT SUPORTED PNM file ***\n");
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	printf("format = %c", byte(c));
	c = fgetc(f);
	printf("%c\n", byte(c));
	if ((c < '1' || c > '7')){ // c == 1 or 2, 3, 4, 5 or 6
		printf("*** NOT SUPORTED PNM file ***\n");
		fseek(f, fp, SEEK_SET);
		return 0;
	}	//if (skipLine(f) == -1 || skipComent(f) == -1)
	//	return 0; // EOF
	if ((xres = readInt(f)) == -1 || (yres = readInt(f)) == -1) {
		printf("*** NOT SUPORTED PNM file, SMALL FILE ***\n");
		fseek(f, fp, SEEK_SET);
		return 0;
	}
	//if(!fscanf(f, "%i", &xres))		return 0;
	//if(!fscanf(f, "%i", &yres))		return 0;
	img = new img_basis;
	if (!img)	return 0;
	img->xres = xres;
	img->yres = yres;

	img->flip=1;

	switch (c) {
		case '1': // monocrome  0 or 1 in text format
		{
			skipLine(f); // \n
			skipComent(f); // any comment
			img->dataSize = xres * yres;
			img->bpp = 8;
			img->chanels = 1;
			img->pixels = new byte[img->dataSize];
			if (!img->pixels) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			byte* p = img->pixels;
			for (int i = 0; i < img->dataSize; ) {
				c = fgetc(f);
				if (c == '1') {
					*(p++) = 255;
					i++;
				}
				else if (c == '0') {
					*(p++) = 0;
					i++;
				}
				else if (c == '#') {
					skipLine(f);
					continue;
				}
				else if (c == EOF)
					break;
				else if (c == '\n' || c == '\r' || c == ' ' || c == '	') // new line or space
					continue;
				else // unknow?
					continue;
			}
			break;
		}
		case '2': // grayscale 0 to 255 in text format
		{
			//c = fgetc(f);
			//if (c == '\n' || c == '\r') {
			//	fseek(f, -1, SEEK_CUR);
			//	if (skipLine(f) == -1) {
			//		delete img;
			//		return 0;
			//	}
			//}
			if ((maxval = readInt(f)) == -1) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			if(maxval<1 || maxval > 255) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			float mul = 255 / maxval;

			img->dataSize = xres * yres;
			img->bpp = 8;
			img->chanels = 1;
			img->pixels = new byte[img->dataSize];
			if (!img->pixels) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			byte* p = img->pixels;
			for (int i = 0; i < img->dataSize; ) {
				if ((c = readInt(f)) == -1)
					break;
				*(p++) = (byte)c*mul;
				i++;
			}
			break;
		}
		case '3': // color 0 to 255 in text format
		{
			//c = fgetc(f);
			//if (c == '\n' || c == '\r') {
			//	fseek(f, -1, SEEK_CUR);
			//	if (skipLine(f) == -1) {
			//		delete img;
			//		return 0;
			//	}
			//}
			maxval = readInt(f);
			if(maxval<1 || maxval > 255) {
				delete img;
				fseek(f, fp, SEEK_SET);
				return 0;
			}
			float mul = 255 / maxval;

			img->dataSize = xres * yres * 3;
			img->bpp = 24;
			img->chanels = 3;
			img->pixels = new byte[img->dataSize];
			if (!img->pixels) {
				delete img;
				fseek(f, fp, SEEK_SET);
				return 0;
			}
			byte* p = img->pixels;
			for (int i = 0; i < img->dataSize; ) {
				if ((c = readInt(f)) == -1)
					break;
				*(p++) = (byte)c*mul;
				i++;
			}
			break;
		}
		case '4': // binary format 1 bpp
		{
			//c = fgetc(f);
			//if (c == '\n' || c == '\r') {
			//	fseek(f, -1, SEEK_CUR);
			//	if (skipLine(f) == -1) {
			//		delete img;
			//		return 0;
			//	}
			//}
			maxval = readInt(f);
			if (maxval < 1 || maxval > 255) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			float mul = 255 / maxval;

			img->dataSize = xres * yres * 1;
			img->bpp = 8;
			img->chanels = 1;
			img->pixels = new byte[img->dataSize];
			if (!img->pixels) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			byte* data = new byte[img->xres];
			if (!data) {
				fseek(f, fp, SEEK_SET);
				delete img;
				return 0;
			}
			byte* p = img->pixels;
			for (int height = 0; height < img->yres; height++) { // binary format 1 bpp
				fread(data, 1, (img->xres + 7) / bpp, f);
				for (int i = 0; i < img->xres; i++) {
					if (bpp == 1) {
						*(p++) = ((data[i / 8] >> (7 - i % 8)) & 0x01);
					}
					else if (bpp == 2) {
						*(p++) = ((data[i / 4] >> ((3 - i % 4) * 2)) & 0x03);
					}
					else if (bpp == 4) {
						*(p++) = ((data[i / 2] >> ((1 - i % 2) * 4)) & 0x0F);
					}
				}

				//if ((c = readInt(f)) == -1)
				//	break;
				//*(p++) = (byte)c * mul;
				//i++;
			}
			delete[] data;
			break;
		}
		default:{
			fseek(f, fp, SEEK_SET);
			printf("*** NOT SUPORTED PNM file YET ***\n");
			delete img;
			return 0;
		}
	}
	// make conversons here at end of function
	// if(img->bpp ! 32){
	//		do conversion
	// }
	if (img->chanels < 4) {
		byte* p = new byte[img->xres * img->yres * 4];
		if (!p) {
			fseek(f, fp, SEEK_SET);
			delete img;
			return 0;
		}
		switch (img->chanels) {
			case 1:
			{
				for (int i = 0; i < img->xres * img->yres; i++) {
					p[i * 4 + 0] = img->pixels[i];
					p[i * 4 + 1] = img->pixels[i];
					p[i * 4 + 2] = img->pixels[i];
					p[i * 4 + 3] = 255;
				}
				delete[] img->pixels;
				img->pixels = p;
				p = 0;
				img->bpp = 32;
				img->chanels = 4;
				img->dataSize = img->xres * img->yres * img->bpp / 8;
				break;
			}
			case 2:
			{
				for (int i = 0; i < img->xres * img->yres; i++) {
					p[i * 4 + 0] = img->pixels[i*2];
					p[i * 4 + 1] = img->pixels[i*2];
					p[i * 4 + 2] = img->pixels[i*2];
					p[i * 4 + 3] = img->pixels[i*2+1];
				}
				delete[] img->pixels;
				img->pixels = p;
				p = 0;
				img->bpp = 32;
				img->chanels = 4;
				img->dataSize = img->xres * img->yres * img->bpp / 8;
				break;
			}
			case 3:
			{
				for (int i = 0; i < img->xres * img->yres; i++) {
					p[i * 4 + 0] = img->pixels[i*3+0];
					p[i * 4 + 1] = img->pixels[i*3+1];
					p[i * 4 + 2] = img->pixels[i*3+2];
					p[i * 4 + 3] = 255;
				}
				delete[] img->pixels;
				img->pixels = p;
				p = 0;
				img->bpp = 32;
				img->chanels = 4;
				img->dataSize = img->xres * img->yres * img->bpp / 8;
				break;
			}
			case 4:
			{
				if (p)
					delete[] p;
				break;
			}
			default:
			{
				fseek(f, fp, SEEK_SET);
				delete img;
				if (p)
					delete[] p;
				return 0;
			}
		}
	}
	return img;
}









