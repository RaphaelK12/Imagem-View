#pragma once

// simplified types

typedef unsigned int			   Bool;
typedef   signed char			   schar;
typedef   signed char			   sbyte;
typedef unsigned char			   uchar;
typedef unsigned char			   byte;
typedef unsigned short			   ushort;
typedef   signed short			   sshort;
typedef unsigned int			   uint;
typedef   signed int			   sint;
typedef unsigned long long int	   ulong;
typedef   signed long long int	   slong;



#pragma pack(push, 1)   // n = 16, pushed to stack
// Truevision Targa File header
struct TGA {
	uchar 	id_length;  		// 1
	uchar	color_type; 		// 2		1=indexed
	uchar	image_type; 		// 3		1=raw index pallete,2=raw rgb rgba ,3=raw grey,
	ushort	color_start; 		// 4 5 9=rle idx, 10=rle rgb rgba,11=rle grey
	union {
		ushort	color_length, pal_length;	// 6 7
	};
	union {
		uchar	color_depth, pal_depth;		// 8
	};
	ushort	x_offset; 		// 9 10
	ushort	y_offset; 		// 11 12
	union {
		ushort	x, xres, width; // 13 14
	};
	union {
		ushort	y, yres, heigth; // 15 16
	};
	uchar 	bpp; 			// 17
	uchar	orientation; 	// 18
	// if color_length > 0 then pallete are here with lenght = (256 * color_depth / bpp) in bgr format
};

struct PCX {
	byte manufacturer;
	byte version;
	byte encoding;
	byte bitsperpixel;
	sshort xmin;
	sshort ymin;
	sshort xmax;
	sshort ymax;
	sshort hdpi;
	sshort vdpi;
	byte colormap[48];
	byte reserved;
	byte nplanes;
	sshort bytesperline;
	sshort paletteinfo;
	sshort hscreensize;
	sshort vscreensize;
	byte _filler[54];
};
// PCX header

// image formats
enum {
	PCX_1BIT,             // 1 bpp, 1 plane (1 bit)
	PCX_1BIT_x,             // 1 bpp, 1 plane (1 bit)
	PCX_2BIT,             // 2 bpp, 1 plane (2 bit)
	PCX_4BIT,             // 4 bpp, 1 plane (4 bit)
	PCX_8BIT,             // 8 bpp, 1 plane (8 bit)
	PCX_24BIT,            // 8 bpp, 3 planes (24 bit)
	PCX_32BIT             // 8 bpp, 4 planes (32 bit)
};



// Usefull structs

struct rgba_4 {
	ushort	r : 4;
	ushort	g : 4;
	ushort	b : 4;
	ushort	a : 4;
};
struct rgb_565 {
	ushort	r : 5;
	ushort	g : 6;
	ushort	b : 5;
};
struct rgba_5551 {
	ushort	r : 5;
	ushort	g : 5;
	ushort	b : 5;
	ushort	a : 1;
};
struct abgr_1555 {
	ushort	a : 1;
	ushort	b : 5;
	ushort	g : 5;
	ushort	r : 5;
};
struct rgb_8 {
	byte	r;
	byte	g;
	byte	b;
};
struct BGR_8 {
	byte	b;
	byte	g;
	byte	r;
};
struct BGRA_8 {
	byte	b;
	byte	g;
	byte	r;
	byte	a;
};
struct rgba_8888 {
	byte	r;
	byte	g;
	byte	b;
	byte	a;
};
struct la_44 {
	byte	r : 4;
	byte	g : 4;
};
#pragma pack(pop)   // n = 2 , stack popped

// test
//int a = sizeof(la_44);
//int b = sizeof(TGA);

// Base Class from Imagem2Lib
struct img_basis {
	img_basis() :
		dataSize(0),
		xres(0),
		yres(0),
		zres(0),
		palSize(0),
		dataFormat(0),
		bpp(0),
		chanels(0),
		orientation(0),
		isOk(0),
		packed(0),
		dataType(0),
		hasAlpha(0),
		flip(0),
		pal(0),
		pixels(0),
		id(0)
	{

	}
	~img_basis() {
		if (pal)
			delete[] pal;
		if (pixels)
			delete[] pixels;
		if (id)
			delete[] id;
	}
	void unflip();
	uint	 dataSize;
	ushort	 xres;
	ushort   yres;
	ushort   zres;
	ushort   palSize;
	uchar	 dataFormat;
	uchar    bpp;
	uchar    chanels;
	uchar	 orientation;

	uchar	 isOk;
	uchar	 packed;
	uchar	 dataType;
	uchar	 flip;
	uchar	 hasAlpha;


	byte* pal;
	byte* pixels;
	byte* id;
};



BOOL isInvalidTGA(TGA* h);
BOOL isUnsuportedTGA(TGA* h);
byte decodeTGA_RLE(byte* pData, byte bytes, size_t dataSize, FILE* f);
img_basis* loadTGA(FILE* f);
img_basis* loadTGA2(FILE* f);
LPSTR removeEndL(LPSTR str, uint size);
img_basis* read_JPEG_file(FILE* f);
img_basis* read_png(FILE* f);
img_basis* read_PCX_file(FILE* f);
img_basis* IMG_LoadPCX_RW(FILE* f);
img_basis* read_PNM_file(FILE* f);
