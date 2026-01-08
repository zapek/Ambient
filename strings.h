/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef AMBIENT_STRING_H
#define AMBIENT_STRING_H

typedef struct _String	   String;

struct _String
{
	char *str;
	int  len;
	int  allocated_len;
};

#define GNUC_PRINTF( format_idx, arg_idx )    \
  __attribute__((__format__ (__printf__, format_idx, arg_idx)))


String*     string_new(const char *init);
String*     string_new_len(const char *init, int len);
String*     string_sized_new(int dfl_size);
char*       string_free(String *string, int return_c_string);
int         strine_gqual(const String *v, const String *v2);
unsigned int string_hash(const String *str);
String*     string_assign(String *string, const char *rval);
String*     string_truncate(String *string, int len);
String*     string_set_size(String *string, int len);
String*     string_insert_len(String *string, int pos, const char *val, int len);
String*     string_append(String     *string, const char *val);
String*     string_append_len(String *string, const char *val, int len);
String*     string_append_c(String *string, char c);
String*     string_prepend (String *string, const char *val);
String*     string_prepend_c(String *string, char c);
String*     string_prepend_len(String *string, const char *val, int len);
String*     string_insert(String *string, int pos,const char *val);
String*     string_insert_c(String *string, int pos, char c);
String*     string_erase(String  *string, int pos, int len);
String*     string_ascii_down(String *string);
String*     string_ascii_up(String *string);
void        string_printf(String *string, const char *format,...) GNUC_PRINTF (2, 3);
void        string_append_printf(String *string, const char *format, ...);
int         string_equal( const String *v, const String *v2 );

/* -- optimize striappend_c --- */

static inline String*
string_append_c_inline (String *gstring,
                          char    c)
{
	if (gstring->len + 1 < gstring->allocated_len)
	{
		gstring->str[gstring->len++] = c;
		gstring->str[gstring->len] = 0;
	}
	else
	{
		string_insert_c (gstring, -1, c);
	}
	return gstring;
}
#define stringappend_c(gstr,c)       stringappend_c_inline (gstr, c)



#define string_sprintf   string_printf
#define string_sprintfa  string_append_printf


#endif /* AMBIENT_STRINGS_H */

