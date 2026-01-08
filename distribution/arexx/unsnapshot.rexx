/*
 *  $VER: unsnapshot.rexx 1.0 (08.08.2006)
 *
 *  synopsis: unsnapshots icon and window positions from all
 *            icon files in a given path
 *
 *  $Id: unsnapshot.rexx,v 1.2 2013/05/19 21:29:28 tokai Exp $
 */

parse arg path

tmpfile = 't:tmp'

address command 'list >t:tmp quick nohead '||path||#?.info

if(open(f, tmpfile, 'r')) then
do
	do while(~eof(f))
		line=readln(f)
		if line~='' then do
			address AMBIENT
			'unsnapshot path="'||line||'" icons window'
		end
	end
	close(f)
	address command 'delete >nil: '||tmpfile
end
