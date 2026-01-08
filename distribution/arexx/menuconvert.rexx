/*
 *  $VER: menuconvert.rexx 1.0 (26.02.2008) © 2008 MorphOS Team
 *
 *  synopsis: This script generates menus from ToolsDaemon config file
 *  usage: rx convert.rexx <toolsdaemon config file>
 *
 *  $Id: menuconvert.rexx,v 1.3 2013/05/19 21:29:28 tokai Exp $
 */

parse arg input

if open(f, input, 'r') then
do

	address AMBIENT 'menu remove'

	line = ""

	title = ''
	item  = ''
	sub   = ''

	i = 0

	firsttime = 1

	do while ~eof(f) & line ~= "END"

		line = readln(f)
		line = strip(line, 'B', D2C(9))

		/* HOTKEY KEY and DESC are ignored */
		if(word(line, 1) ~= "HOTKEY" & word(line, 1) ~= "DESC") then
		do
			if firsttime = 0 then
			do
				preventry = word(prevline, 1)
				prevargument = word(prevline, words(prevline))
			end
			else do
				firsttime = 0
			end

			entry = word(line, 1)
			argument = word(line, words(line))

	        /* TITLE */
			if entry = "TITLE" then
			do
				i = i + 1
				title = argument
				parent = makeid(title)
				currentparent = parent

				address AMBIENT 'menu add id="'parent'" title="'title'" type=menu'
			end

	        /* ITEM */
			if entry = "ITEM" then 
			do
				i = i + 1
				parent = currentparent
				parse var line 'ITEM ['key'] 'trash

				if preventry = "ITEM" then
				do
					address AMBIENT 'menu add id="'makeid(prevargument)'" parentid="'parent'" title="'prevargument'" shortcut="'key'" type=item command=""'
				end
			end

	        /* SUB */
			if entry = "SUB" then 
			do
				i = i + 1
				parse var line 'SUB ['key'] 'trash
				if preventry = "ITEM" then
				do
					parent = makeid(prevargument)
					address AMBIENT 'menu add id="'parent'" parentid="'currentparent'" title="'prevargument'" type=menu'
				end

				if preventry = "SUB" then
				do
					address AMBIENT 'menu add id="'makeid(prevargument)'" parentid="'parent'" title="'prevargument'" shortcut="'key'" type=item'
				end
			end

	        /* ITEMBAR */
			if entry = "ITEMBAR" then 
			do
				i = i + 1
				parent = currentparent
				address AMBIENT 'menu add id="'makeid('separator')'" parentid="'parent'" type=separator'
			end

			/* SUBBAR */
			if entry = "SUBBAR" then 
			do
				i = i + 1
				address AMBIENT 'menu add id="'makeid('separator')'" parentid="'parent'" type=separator'
			end

	        /* Command */
			if entry = "(CLI)" | entry = "(WB)" then 
			do
				commandtype	= entry;
				command = line

				if commandtype = "(CLI)" then
				do
					command = word(command, 3)
					type = "amigados"
				end

				if commandtype = "(WB)" then
				do
					command = word(command, 2)
					type = "workbench"
				end

				address AMBIENT 'menu add id="'makeid(prevargument)'" parentid="'parent'" title="'prevargument'" shortcut="'key'" type=item command='command' commandtype='type''
			end

			prevline = line
		end
	end

	close(f)
end

exit

makeid: EXPOSE i
parse arg name
str = 'menu-'name'-'i
return str


