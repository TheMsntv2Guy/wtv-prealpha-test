CHeaderOptions	=	-i {WTV:}Interfaces: -i {WTV:}Storage: -i {MPW}Interfaces:CIncludes:
CDefinitions	=	-d WEBTV -d DEBUG -d PRERELEASE �
					-d FOR_MAC -d `Echo {User} | Translate ' '`
COptions		=	{CDefinitions} {CHeaderOptions} -b2 -r -mc68020
CCompiler		=	C

ToolFiles = {WTV.o:}SerialDownload {WTV.o:}SRec2Nub {WTV.o:}Romify {WTV.o:}WaitTil {WTV.o:}RezCat

ToolLibraries = �
		"{Libraries}"Interface.o �
		"{Libraries}"Runtime.o �
		"{CLibraries}"StdCLib.o �
		"{CLibraries}"CSANELib.o �
		"{CLibraries}"Math.o �
		"{Libraries}"Interface.o �
		"{Libraries}"ToolLibs.o �

Install							� {ToolFiles}
	Duplicate -y {ToolFiles} ::
	
{WTV.o:}SerialDownload			�� {WTV.o:}SerialDownload.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}SerialDownload.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}SerialDownload.c.o		� SerialDownload.c
	C -b2 SerialDownload.c -o {Targ}.New
		Rename -y {Targ}.New {Targ}

{WTV.o:}SRec2Nub				�� {WTV.o:}SRec2Nub.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}SRec2Nub.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}SRec2Nub.c.o			� SRec2Nub.c
	C -b2 SRec2Nub.c -o {Targ}.New
		Rename -y {Targ}.New {Targ}

{WTV.o:}Romify					�� {WTV.o:}Romify.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}Romify.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}Romify.c.o				� Romify.c
	C {COptions} -o {Targ}.New Romify.c
		Rename -y {Targ}.New {Targ}

{WTV.o:}WaitTil					�� {WTV.o:}WaitTil.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}WaitTil.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}WaitTil.c.o				� WaitTil.c
	C {COptions} -o {Targ}.New WaitTil.c
		Rename -y {Targ}.New {Targ}

{WTV.o:}RezCat					�� {WTV.o:}RezCat.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}RezCat.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}RezCat.c.o				� RezCat.c
	C {COptions} -o {Targ}.New RezCat.c
		Rename -y {Targ}.New {Targ}
