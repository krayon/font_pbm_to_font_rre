//Confusing tool to turn .pbm fonts into compressed RREs for RFB

// Some improvements by cbm80amiga:
// - got rid of getline() function
// - code can be now compiled using TCC on Win machines
// - got rid of cw divisible by 8 limitation
// - added 24bit/rect mode for 64x64 pixels fonts
// - more debug information
// - decreased/improved rectangle overlapping
// - added ascii only font bitmap support (range 0x20-0x7f only - 6 lines of 16 chars)
// Some improvements by krayon:
// - added support for other common bitmap font formats (such as 8 lines of 16 chars)

#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHARS 256
#define MAX_RECTS_PER_CHAR 100

int lessOverlap = 1;
int mode24b = 0;

int n_chrs     = 0;
int chr_offset = 0;

int cw, ch;
int w, h;
int bytes;
uint8_t * buff;

struct MRect
{
	uint8_t x, y, w, h;
};

int ReadFile( const char * rn );

void DumpBuffer8( uint8_t * dat, int len, char * name )
{
	printf( "const unsigned char %s[%d] = {", name, len );
	int i;
	for( i = 0; i < len; i++ )
	{
		if( (i%16) == 0 )
		{
			printf( "\n\t" );
		}
		printf( "0x%02x, ", dat[i] );
	}
	printf( "\n};\n\n" );
}

void DumpBuffer16( uint16_t * dat, int len, char * name )
{
	printf( "const unsigned short %s[%d] = {", name, len );
	int i;
	for( i = 0; i < len; i++ )
	{
		if( (i%16) == 0 )
		{
			printf( "\n\t" );
		}
		printf( "0x%04x, ", dat[i] );
	}
	printf( "\n};\n\n" );
}

int TryCover( uint8_t * ibuff, struct MRect * r )
{
	int x, y;
	int count = 0;
	for( y = r->y; y < r->y+r->h; y++ )
	{
		for( x = r->x; x < r->x+r->w; x++ )
		{
			if( ibuff[x+y*cw] == 1 ) count++;
			else if( ibuff[x+y*cw] == 0 ) return -1;
		}
	}
	return count;
}

void DoCover( uint8_t * ibuff, struct MRect * r )
{
	int x, y;
	int count = 0;
	for( y = r->y; y < r->y+r->h; y++ )
	{
		for( x = r->x; x < r->x+r->w; x++ )
		{
			if( ibuff[x+y*cw] > 0 ) ibuff[x+y*cw]++;
		}
	}
}

void Fill( uint8_t * ibuff, struct MRect * r, int val )
{
	int x, y;
	int count = 0;
	for( y = r->y; y < r->y+r->h; y++ )
		for( x = r->x; x < r->x+r->w; x++ )
			ibuff[x+y*cw] = val;
}

int Covers( uint8_t * ibuff, struct MRect * rs )
{
	int i;
	int x, y, w, h;

	for( i = 0; i < MAX_RECTS_PER_CHAR; i++ )
	{
		struct MRect most_efficient, tmp;
		int most_efficient_count = 0;
		for( y = 0; y < ch; y++ )
		for( x = 0; x < cw; x++ )
		for( h = 1; h <= ch-y; h++ )
		for( w = 1; w <= cw-x; w++ )
		{
			if( !mode24b && (w > 16 || h > 16 || x > 16 || y > 16) ) continue;
			if(  mode24b && (w > 64 || h > 64 || x > 64 || y > 64 )) continue;
			tmp.x = x; tmp.y = y; tmp.w = w; tmp.h = h;
			int ct = TryCover( ibuff, &tmp );
			if( ct > most_efficient_count || (lessOverlap && ct==most_efficient_count && tmp.w*tmp.h<most_efficient.w*most_efficient.h) )
			{
				memcpy( &most_efficient, &tmp, sizeof( tmp ) );
				most_efficient_count = ct;
			}
		}

		if( most_efficient_count == 0 )
		{
					return i;
		}

		DoCover( ibuff, &most_efficient );
		memcpy( &rs[i], &most_efficient, sizeof( struct MRect ) );
	}
	fprintf( stderr, "Error: too many rects per char.\n ");
	exit( -22 );
}

int GreedyChar( int chr, int debug, struct MRect * RectSet )
{
	int x, y, i;
	uint8_t cbuff[ch*cw];
	uint8_t rbuff[ch*cw];
	int rectcount;
/*
	for( y = 0; y < ch; y++ )
	for( x = 0; x < cw/8; x++ )
	{
		int ppcx = w/cw;
		int xpos = (chr % ppcx)*cw;
		int ypos = (chr / ppcx)*ch;
		int idx = xpos/8+(ypos+y)*w/8+x;
		int inpos = buff[idx];

		for( i = 0; i < 8; i++ )
		{
			cbuff[x*8+y*cw+i] = (inpos&(1<<(7-i)))?0:1;
		}
	}
	*/
	for( i = 0; i < ch*cw; i++ ) cbuff[i] = rbuff[i] = 0;
	if (chr < n_chrs)
		for( y = 0; y < ch; y++ )
		for( x = 0; x < cw; x++ )
		{
			int ppcx = w/cw;
			int xpos = ((chr - chr_offset) % ppcx)*cw;
			int ypos = ((chr - chr_offset) / ppcx)*ch;
			int idx = ((ypos+y)*w+xpos+x)/8;
			int xbit = (xpos+x)&7;
	        cbuff[x+y*cw] = (buff[idx]&(1<<(7-xbit)))?0:1;
	    }


	//Greedily find the minimum # of rectangles that can cover this.
	rectcount = Covers( cbuff, RectSet );

	if( debug ) {
		int numRectPixels = 0;
		int numPixels = 0;
		for( i = 0; i < rectcount; i++ ) {
			numRectPixels+=RectSet[i].w*RectSet[i].h;
			Fill(rbuff,&RectSet[i],i+1);
		}
		for( i = 0; i < ch*cw; i++ ) if(rbuff[i]>0) numPixels++;
		printf( "----------------\nChar 0x%02x '%c' %d rects, %d pixels (%d overlapping) \n", chr, chr<32?'.':chr, rectcount, numRectPixels, numRectPixels-numPixels);
		for( i = 0; i < rectcount; i++ )
			printf( " %2d %2d %2d %2d   [%c]\n", RectSet[i].x, RectSet[i].y, RectSet[i].w, RectSet[i].h, i<9?i+'1':i+'A'-9);
		printf( "\n" );

		for( y = 0; y < ch; y++ )
		{
			for( x = 0; x < cw; x++ )
				printf( "%d", cbuff[x+y*cw] );
			printf( "    " );
			for( x = 0; x < cw; x++ )
				printf( "%c", rbuff[x+y*cw]<10?rbuff[x+y*cw]+'0':rbuff[x+y*cw]+'A'-10 );
			printf( "\n" );
		}
	}

	return rectcount;
}

int main( int argc, char ** argv )
{
	int r, i, x, y, j;

	if( argc != 4 )
	{
		fprintf( stderr, "Got %d args.\nUsage: [tool] [input_pbm] [char w] [char h]\n", argc );
		return -1;
	}

	if( (r = ReadFile( argv[1] )) )
	{
		return r;
	}

	cw = atoi( argv[2] );
	ch = atoi( argv[3] );

	printf( "#define FONT_CHAR_W %d\n", cw );
	printf( "#define FONT_CHAR_H %d\n\n", ch );

	if(cw>16 || ch>16) mode24b = 1; else mode24b = 0;
	printf("mode24=%d\n",mode24b);
	n_chrs = (w/cw) * (h/ch);
	if (n_chrs >= MAX_CHARS) n_chrs = MAX_CHARS;
	if (n_chrs <= (16 * 6)) chr_offset = 32;

// 	if( ( cw % 8 ) != 0 )
// 	{
// 		fprintf( stderr, "Error: CW MUST be divisible by 8.\n" );
// 	}

//	struct MRect MRect rs;
//	GreedyChar( 0xdc, 1, &rs );

	struct MRect MRects[MAX_CHARS*MAX_RECTS_PER_CHAR];
	uint16_t places[MAX_CHARS+1];
	int place = 0;
	for( i = 0; i < n_chrs; i++ )
	//i = 'H';
	{
		places[i] = place;
		place += GreedyChar( i, 1, &MRects[place] );
	}
	places[i] = place;

	uint8_t outbuffer[MAX_CHARS*MAX_RECTS_PER_CHAR*2*3];
	for( i = 0; i < place; i++ )
	{
		int x = MRects[i].x;
		int y = MRects[i].y;
		int w = MRects[i].w;
		int h = MRects[i].h;
		if( w == 0 || w > 64 ) { fprintf( stderr, "Error: invalid W value\n" ); return -5; }
		if( h == 0 || h > 64 ) { fprintf( stderr, "Error: invalid H value\n" ); return -5; }
		if( x > 63 ) { fprintf( stderr, "Error: invalid X value\n" ); return -5; }
		if( y > 63 ) { fprintf( stderr, "Error: invalid Y value\n" ); return -5; }
		w--;
		h--;
		if(!mode24b)
			((uint16_t*)outbuffer)[i] = x | (y<<4) | (w<<8) | (h<<12);
		else {
			outbuffer[i*3+0] = x | ((h<<6) & 0xc0);
			outbuffer[i*3+1] = y | ((h<<4) & 0xc0);
			outbuffer[i*3+2] = w | ((h<<2) & 0xc0);
		}
	}

	fprintf( stderr, "Total places: %d\n", place );
	printf( "#ifndef __font__h__\n" );
	printf( "#define __font__h__\n\n" );
	if(!mode24b)
		DumpBuffer16( (uint16_t*)outbuffer, place, "font_Rects" );
	else
		DumpBuffer8( outbuffer, place*3, "font_Rects" );
	DumpBuffer16( places, MAX_CHARS+1, "font_CharOffs" );
	printf( "#endif\n" );

	return 0;
}

int ReadFile( const char * rn )
{
	int r;
	char ct[1024];
	FILE * f = fopen( rn, "r" );

	if( !f )
	{
		fprintf( stderr, "Error: Cannot open file.\n" );
		return -11;
	}

	if( fscanf( f, "%1023s", ct ) != 1 || strcmp( ct, "P4" ) != 0 )
	{
		fprintf( stderr, "Error: Expected P4 header.\n" );
		return -2;
	}

	char c = fgetc(f);
	while(c!=10 && c!=13 && !feof(f)) c=fgetc(f);

	if( (r = fscanf( f, "%d %d\n", &w, &h )) != 2 || w <= 0 || h <= 0 )
	{
		fprintf( stderr, "Error: Need w and h in pbm file.  Got %d params.  (%d %d)\n", r, w, h );
		return -4;
	}
	printf("PBM: %dx%d\n",w,h);

	bytes = (w*h)>>3;
	buff = malloc( bytes );
	r = fread( buff, 1, bytes, f );
	if( r != bytes )
	{
		fprintf( stderr, "Error: Ran into EOF when reading file.  (%d)\n",r  );
		return -5;
	}

	fclose( f );
	return 0;
}
