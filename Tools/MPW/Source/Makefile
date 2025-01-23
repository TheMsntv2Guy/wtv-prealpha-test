CHeaderOptions	=	-i {WTV:}Interfaces: -i {WTV:}Storage: -i {MPW}Interfaces:CIncludes:
CDefinitions	=	-d WEBTV -d DEBUG -d PRERELEASE ¶
					-d FOR_MAC -d `Echo {User} | Translate ' '`
COptions		=	{CDefinitions} {CHeaderOptions} -b2 -r -mc68020
CCompiler		=	C

ToolFiles = {WTV.o:}SerialDownload {WTV.o:}SRec2Nub {WTV.o:}Romify {WTV.o:}WaitTil {WTV.o:}RezCat

ToolLibraries = ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"Runtime.o ¶
		"{CLibraries}"StdCLib.o ¶
		"{CLibraries}"CSANELib.o ¶
		"{CLibraries}"Math.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶

Install							Ä {ToolFiles}
	Duplicate -y {ToolFiles} ::
	
{WTV.o:}SerialDownload			ÄÄ {WTV.o:}SerialDownload.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}SerialDownload.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}SerialDownload.c.o		Ä SerialDownload.c
	C -b2 SerialDownload.c -o {Targ}.New
		Rename -y {Targ}.New {Targ}

{WTV.o:}SRec2Nub				ÄÄ {WTV.o:}SRec2Nub.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}SRec2Nub.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}SRec2Nub.c.o			Ä SRec2Nub.c
	C -b2 SRec2Nub.c -o {Targ}.New
		Rename -y {Targ}.New {Targ}

{WTV.o:}Romify					ÄÄ {WTV.o:}Romify.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}Romify.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}Romify.c.o				Ä Romify.c
	C {COptions} -o {Targ}.New Romify.c
		Rename -y {Targ}.New {Targ}

{WTV.o:}WaitTil					ÄÄ {WTV.o:}WaitTil.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}WaitTil.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}WaitTil.c.o				Ä WaitTil.c
	C {COptions} -o {Targ}.New WaitTil.c
		Rename -y {Targ}.New {Targ}

{WTV.o:}RezCat					ÄÄ {WTV.o:}RezCat.c.o
	Link -w -t MPST -c 'MPS ' {WTV.o:}RezCat.c.o {ToolLibraries} -o {Targ}.New
		Rename -y {Targ}.New {Targ}
{WTV.o:}RezCat.c.o				Ä RezCat.c
	C {COptions} -o {Targ}.New RezCat.c
		Rename -y {Targ}.New {Targ}
