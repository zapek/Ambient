/*
 * Catmaker was written by Oliver Wagner, modified by various people like
 * Jon Bright and David Gerber. Many thanks to Oliver for allowing me
 * to include it in the Ambient GPL release.
 *
 * @ Oliver Wagner and others
 * © 2006 Ambient Open Source Team
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
 *
 * $Id: catmaker.c,v 1.4 2006/08/08 13:31:33 fab Exp $
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

/*  list stuff
 */
struct node {
	struct node *succ;
	struct node *pred;
};

struct list {
	struct node *head;
	struct node *tail;
	struct node *tailpred;
};

void newlist(struct list *l)
{
	l->tailpred = (struct node *)l;
	l->tail     = (struct node *)0;
	l->head 	= (struct node *)&l->tail;
}

void addtail(struct list *l, struct node *n)
{
	struct node *bak = l->tailpred;

	n->succ     = (struct node *)&l->tail;
	n->pred     = bak;
	bak->succ   = n;
	l->tailpred = n;
}

void addhead(struct list *l, struct node *n)
{
	struct node *bak = l->head;

	n->succ   = bak;
	bak->pred = n;
	n->pred   = (struct node *)l;
	l->head   = n;
}

void insert(struct list *l, struct node *n, struct node *ln)
{
	if (ln)
	{
		n->pred       = ln;
		n->succ       = ln->succ;
		n->succ->pred = n;
		ln->succ      = n;
	}
	else
	{
		struct node *bak = l->head;

		n->succ   = bak;
		bak->pred = n;
		n->pred   = (struct node *)l;
		l->head   = n;
	}
}

/*  a bit copy'n'paste from includes/macros/vapor.h, first 2 
 *  don't work on empty lists
 */
#define FIRSTNODE(l)   ((void *)(l)->head)
#define NEXTNODE(n)    ((void *)((struct node *)n)->succ)

#define ADDHEAD(l,n)   addhead(l, n)
#define ADDTAIL(l,n)   addtail(l, n)
#define NEWLIST(l)     newlist(l)
#define ISLISTEMPTY(l) (((l)->tailpred) == (struct node *)(l))
#define INSERT(l,n,ln) insert(l, n, ln)


struct stringnode {
	struct node n;
	struct node on;
	unsigned char *label;
	int len;
	struct stringnode *part;
	int partoffset;
	int number;
	unsigned char text[ 0 ];
};

static unsigned char buffer[ 8192 ], buffer2[ 256 ], buffer3[ 512 ];

static struct list sl, osl;

static void readstrings( FILE *inf )
{
	unsigned char *p, *d;
	unsigned char c;
	struct stringnode *n, *rn;
	int linecnt = 0;
	int number = 0;
	int size = 0;

	for(;;)
	{
		int bufferlen;
		unsigned char slashnum[40];
		unsigned char *endp;
		int val;

		if( !fgets( buffer2, sizeof( buffer2 ), inf ) )
		{
			break;
		}
		linecnt++;
		if( buffer2[ 0 ] == ';' || buffer2[ 0 ] == '#' )
			continue;

		if( sscanf(buffer2, "%s (%39s)", buffer, slashnum) != 2 ||
		    (val = strtol(slashnum, (char **)&endp, 10), *endp != '/') )
		{
			fprintf( stderr, "error in line %d: %s", linecnt, buffer2 );
			exit( 20 ); /* RETURN_ERROR */
		}
		if( endp > slashnum )
		{
			/*
			 * Supporting this would require rewriting the whole lookup thing
			 * to use hash/btree instead of linear lookuptable (or using some
			 * really sucky linear search per string ref.. oh-no). If someone
			 * bothers, go ahead. - piru
			 */
			fprintf( stderr, "we don't support setting the catalog string ID. sorry.\n" );
			exit( 20 ); /* RETURN_ERROR */
		}
		strcpy(buffer2, buffer);

		buffer[ 0 ] = '\0';

		// now read actual string data
		for(;;)
		{
			if( !fgets( buffer3, sizeof( buffer3 ), inf ) )
				break;
			linecnt++;
			p = strrchr( buffer3, '\n' );
			if( p )
				*p = '\0';
			if( buffer3[ 0 ] )
			{
				p = strchr( buffer3, 0 ) - 1;
				if( *p == '\\' )
				{
					*p = '\0';
					strcat( buffer, buffer3 );
				}
				else
				{
					strcat( buffer, buffer3 );
					break;
				}
			}
		}

		// buffer has string, buffer2 has label
		p = buffer;
		d = buffer;
		while(( c = *p++ ))
		{
			unsigned char bf[ 4 ];
			unsigned long v;
			unsigned char *endp;

			if( c == '\\' ) switch( (c = *p++) )
			{
				case 'a':  /* bell */
					*d++ = 7;
					break;

				case 'b':  /* backspace */
					*d++ = 8;
					break;

				case 'c':  /* control sequence introducer */
					*d++ = 0x9b;
					break;

				case 'e':  /* escape */
					*d++ = '\e';
					break;

				case 'f':  /* form feed */
					*d++ = 12;
					break;

				case 'g':  /* bell (flexcat) */
					*d++ = 7;
					break;

				case 'n':  /* carriage return */
					*d++ = '\n';
					break;

				case 'r':  /* life feed */
					*d++ = '\r';
					break;

				case 't':  /* tab */
					*d++ = '\t';
					break;

				case 'v':  /* vertical tab */
					*d++ = 11;
					break;

				case '\\':
					*d++ = '\\';
					break;

				case '"': /* undocumented, should fail? */
					*d++ = '"';
					break;

				case 'x':
					bf[ 0 ] = *p++;
					bf[ 1 ] = *p++;
					bf[ 2 ] = '\0';

					v = strtoul( bf, (char **)&endp, 16 );
					if( endp != &bf[ 2 ] )
					{
						fprintf( stderr, "invalid hex number in line %d: %s\n", linecnt, p - 4 );
						exit( 20 );
					}
					if( !v )
						v = 0xff;
					*d++ = v;
					break;

				case '0':
				case '1':
				case '2':
				case '3': /* 255 in octal is 377, so first digit can only be 0-3. */
					bf[ 0 ] = c;
					bf[ 1 ] = *p++;
					bf[ 2 ] = *p++;
					bf[ 3 ] = '\0';

					v = strtoul( bf, (char **)&endp, 8 );
					if( endp != &bf[ 3 ] || v > 255 )
					{
						fprintf( stderr, "invalid octal number in line %d: %s\n", linecnt, p - 4 );
						exit( 20 );
					}
					if( !v )
						v = 0xff;
					*d++ = v;
					break;

				default:
					fprintf( stderr, "unknown format code in line %d: %s\n", linecnt, p - 2 );
					exit( 20 );
			}
			else
			{
				*d++ = c;
			}
		}
		*d = '\0';

		bufferlen = d - buffer;
		n = malloc( sizeof( *n ) + bufferlen + 1 + strlen(buffer2) + 1);
		if( !n )
		{
			fprintf( stderr, "out of memory\n" );
			exit( 20 ); /* RETURN_ERROR */
		}
		n->label = n->text + bufferlen + 1;
		strcpy( n->label, buffer2 );
		memcpy( n->text, buffer, bufferlen + 1);
		n->len = bufferlen;
		size += n->len;

		n->part = 0;
		n->partoffset = 0;

		n->number = number++;

		ADDTAIL( &osl, &n->on );

		// add node
		if( ISLISTEMPTY( &sl ) )
			ADDTAIL( &sl, &n->n );
		else
		{
			struct stringnode *rn2 = FIRSTNODE( &sl );

			if( n->len > rn2->len )
				ADDHEAD( &sl, &n->n );
			else
			{
				for( rn = rn2, rn2 = NEXTNODE( rn2 ); NEXTNODE( rn2 ); rn = rn2, rn2 = NEXTNODE( rn2 ) )
				{
					if( n->len >= rn2->len )
					{
						INSERT( &sl, &n->n, &rn->n );
						break;
					}
				}
				if( !NEXTNODE( rn2 ) )
					ADDTAIL( &sl, &n->n );
			}
		}
	}

	printf( "read %d strings, %d bytes\n", number, size );
}

static void optimizestrings( void )
{
	struct stringnode *n;
	int optcnt = 0, optsize = 0;

	for( n = FIRSTNODE( &sl ); NEXTNODE( n ); n = NEXTNODE( n ) )
	{
		struct stringnode *cmp;

		for( cmp = FIRSTNODE( &sl ); cmp != n; cmp = NEXTNODE( cmp ) )
		{
			if( !cmp->part && !strncmp( &cmp->text[ cmp->len - n->len ], n->text, n->len ) )
			{
				// found
				//printf( "found match '%s' (%s) in '%s' (%s)\n", n->text, n->label, cmp->text, cmp->label );
				n->part = cmp;
				n->partoffset = cmp->len - n->len;
				optcnt++;
				optsize += n->len;
				break;
			}
		}
	}
	printf( "optimized %d strings, %d bytes\n", optcnt, optsize );
}

static void writehfile( FILE *hf )
{
	struct node *node;
	int cnt = 0;

	for( node = FIRSTNODE( &osl ); NEXTNODE( node ); node = NEXTNODE( node ), cnt++ )
	{
		struct stringnode *n = (void *)((char *)node - offsetof( struct stringnode, on ));
		fprintf( hf, "#define %s %d\n", n->label, n->number );
	}

	fprintf( hf, "#define NUMCATSTRING %d\n", cnt );
}

static void writeasmfile( FILE *af )
{
	struct stringnode *n;
	struct node *node;

	fprintf( af, "\tsection _NOMERGE,data\n" );

	for( n = FIRSTNODE( &sl ); NEXTNODE( n ); n = NEXTNODE( n ) )
	{
		if( !n->part )
		{
			unsigned char *p = n->text;
			int cnt = 0;
			fprintf( af, "msg%d: ", n->number );
			while( *p )
			{
				if( !cnt )
					fprintf( af, " dc.b " );
				else
					fprintf( af, "," );

				if( *p == 0xff )
				{
					fprintf( af, "0");
					p++;
				}
				else
					fprintf( af, "%lu", (unsigned long)*p++ );
				if( ++cnt == 20 )
				{
					fprintf( af, "\n" );
					cnt = 0;
				}
			}
			fprintf( af, "\n\tdc.b 0\n" );
		}
	}

	fprintf( af, "\tsection __MERGED,data\n\txdef ___stringtable\n___stringtable:\n" );

	for( node = FIRSTNODE( &osl ); NEXTNODE( node ); node = NEXTNODE( node ) )
	{
		n = (void *)((char *)node - offsetof( struct stringnode, on ));
		if( !n->part )
			fprintf( af, "\tdc.l msg%d\n", n->number );
		else
			fprintf( af, "\tdc.l msg%d+%ld\n", n->part->number, (unsigned long)n->partoffset );
	}

	fprintf( af, "\n\tend\n" );
}

static void writecfile( FILE *cf )
{
	struct stringnode *n;
	struct node *node;
	int    stroffset = 0;
	int    cnt = 0;

	fprintf( cf, "unsigned char __strings[]=\n\t\"");

	for( n = FIRSTNODE( &sl ); NEXTNODE( n ); n = NEXTNODE( n ) )
	{
		n->number = stroffset;

		if( !n->part )
		{
			unsigned char *p = n->text;
			while( *p )
			{
				fprintf( cf, "\\x%02x", *p == 0xff ? '\0' : *p);

				p++;
				stroffset++;

				if (++cnt == 20)
				{
					fprintf( cf, "\"\n\t\"");
					cnt = 0;
				}
			}
			fprintf( cf, "\\x00" );
			stroffset++;

			if (++cnt == 20)
			{
				fprintf( cf, "\"\n\t\"");
				cnt = 0;
			}
		}
	}

	fprintf( cf, "\";\n\n");

	fprintf( cf, "unsigned char *__stringtable[] = {\n\t");

	for( node = FIRSTNODE( &osl ); NEXTNODE( node ); node = NEXTNODE( node ) )
	{
		n = (void *)((char *)node - offsetof( struct stringnode, on ));
		if( !n->part )
			fprintf( cf, "__strings+%ld", (unsigned long)n->number );
		else
			fprintf( cf, "__strings+%ld", (unsigned long)(n->part->number+n->partoffset) );

		if( NEXTNODE( NEXTNODE( node ) ) )
			fprintf( cf, ",\n\t" );
	}
	fprintf( cf, "\n};\n\n" );
}

int main( int argc, char **argv )
{
	FILE *inf, *hf, *af = 0, *cf = 0;

	if( argc < 3 || argc > 4 )
	{
		fprintf( stderr, "usage: %s cdfile .h-file ['cfile']\n", argv[0] );
		exit( 20 );
	}

	inf = fopen( argv[ 1 ], "r" );
	if( !inf )
	{
		fprintf( stderr, "can't open %s\n", argv[ 1 ] );
		exit( 10 );
	}

	hf = fopen( argv[ 2 ], "w" );
	if( !hf )
	{
		fprintf( stderr, "can't open %s\n", argv[ 2 ] );
		exit( 10 );
	}

	if (argc == 3)
	{
		af = fopen( "cattmp.a", "w" );
		if( !af )
		{
			fprintf( stderr, "can't open cattmp.a\n" );
			exit( 10 );
		}
	}
	else
	{
		cf = fopen( "cattmp.c", "w" );
		if( !cf )
		{
			fprintf( stderr, "can't open cattmp.c\n" );
			exit( 10 );
		}
	}

	NEWLIST( &sl );
	NEWLIST( &osl );

	readstrings( inf );
	optimizestrings();
	writehfile( hf );

	if (argc == 3)
	{
		writeasmfile( af );
	}
	else
	{
		writecfile( cf );
	}

	//printf( "done\n" );
	return 0;
}
