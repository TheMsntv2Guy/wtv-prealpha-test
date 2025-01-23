#! /tools/perl/perl5.001m/bin/perl
# -*-mode:Fundamental; tab-width:4-*-
#
# BuildBoxRom.pl - simple perl script to take the .o from the linker
#	and generate a ROM image suitable for downloading
#

# -----------------------------------------------------------
# Global variables
# -----------------------------------------------------------

($gScriptName			= $0) =~ s%^.*/%%;    # name of the calling script
$gCurrentDate			= `date "+%m.%d.%y"`;
$gDebugOption           = 0;  				# only determines if symbols go into the rom
$gVerboseOption			= 0;
$gAppROMOption			= 0;
$gBootROMOption			= 0;
$gUniROMOption			= 0;

$gScriptDebugging    	= 0;
$gNoExecution        	= 0;

$gBuildNumber        	= 0;
$gFullPathObjectFile	= "";
$gObjectDirectory		= "";
$gObjectFile			= "";
$gBaseObjectFile		= "";
$gRedirection			= '';


# -----------------------------------------------------------
# parse arguments
# -----------------------------------------------------------

while (@ARGV)
{
	print "\tRAN: BuildBoxRom.pl " . join(" ", @ARGV) . "\n";

	$gScriptDebugging = 1;
	$gVerboseOption = 1;

    if (@ARGV[0] =~ /^-/)               # does it begin with a '-'?
    {
        if (@ARGV[0] =~ /^-debug/i)
            { $gDebugOption = 1; }
        elsif (@ARGV[0] =~ /^-v/i)
            { $gVerboseOption = 1; }
        elsif (@ARGV[0] =~ /^-approm/i)
            { $gAppROMOption = 1; }
        elsif (@ARGV[0] =~ /^-bootrom/i)
            { $gBootROMOption = 1; }
        elsif (@ARGV[0] =~ /^-unirom/i)
            { $gUniROMOption = 1; }
        elsif (@ARGV[0] =~ /^-build=(.*)/i)
            { $gBuildNumber = $1; }
        elsif (@ARGV[0] =~ /^-debugScript/i)
            { $gScriptDebugging = 1; }
        elsif (@ARGV[0] =~ /^-NOEXEC/)
            { $gNoExecution = 1; }
        elsif (@ARGV[0] =~ /^-help/i)
            { &HelpMessage(0); }
        else
            { print STDERR "### $gScriptName: ERROR: Illegal option: \'@ARGV[0]\'\n"; &HelpMessage(1); }
    }
    else
    {
        $gFullPathObjectFile = @ARGV[0];
		($gObjectFile = $gFullPathObjectFile) =~ s%^(.*)/%%;
		$gObjectDirectory = $1;
		($gBaseObjectFile = $gObjectFile) =~ s%\..*$%%;
    }
    shift @ARGV;

}


#--------------------------------------------------
# Main - grand central
#--------------------------------------------------

	$starttime = time();
	
	&Setup();

	if ( $gAppROMOption )
	{
		&BuildAppROM();
	}
	elsif  ( $gBootROMOption )
	{
		&BuildBootROM();
	}
	elsif  ( $gUniROMOption )
	{
		&BuildUniROM();
	}

	$endtime = time();
	$buildTime = &CalculateElapsedTime( $starttime, $endtime );

	# print "# the total build time was: $buildTime\n\n";
	
# Done.



#--------------------------------------------------
# Setup - Initialize, error checks
#--------------------------------------------------
sub Setup
{
	if ( "$gObjectFile" eq "" )
	{
		print STDERR "### $gScriptName: ERROR: you must specify a object file to process\n\n\n";
		&HelpMessage(1);
	}
	if ( !($gAppROMOption) && !($gBootROMOption) && !($gUniROMOption) )
	{
		print STDERR "### $gScriptName: ERROR: you must specify either -approm, -bootrom or -unirom\n\n\n";
		&HelpMessage(1);
	}
	if ( ($gAppROMOption) && ($gBootROMOption) && ($gUniROMOption) )
	{
		print STDERR "### $gScriptName: ERROR: you must specify only one of -approm, -bootrom or -unirom\n\n\n";
		&HelpMessage(1);
	}
	if ( $gVerboseOption )
	{
		$gRedirection = '';
	}
	else
	{
		$gRedirection = ' > /dev/null';
	}

}



#--------------------------------------------------
# BuildAppROM - build the application box rom (ready for downlaod)
#--------------------------------------------------
sub BuildAppROM
{
	print STDOUT "     Creating ROMFS...\n";
	&Execute ("./objects/tools/romify -progress -romtop=0x9fffc000 ./ROM_APP > $gObjectDirectory/ROMFS") ;

	if ( $gDebugOption )
	{
		print STDOUT "     Sorting symbolic information...\n";
		&Execute( "./objects/tools/snarfsyms $gObjectDirectory/approm.sym -o $gObjectDirectory/approm.ss $gRedirection" );
	}

	print STDOUT "     Creating approm image...\n";
	&Execute( "./objects/tools/buildrom -p -s 0x001fc000 -a 0x001fc000 -n 0x9FDFFFC0 -v $gBuildNumber -b 0x9FE00000 -c $gObjectDirectory/approm.ld -f $gObjectDirectory/ROMFS -o $gObjectDirectory/approm.code.o $gRedirection");
	
	if ( $gDebugOption )
	{
		print STDOUT "     Adding symbolic information to the approm image...\n";
		&Execute( "cat $gObjectDirectory/approm.code.o $gObjectDirectory/approm.ss > $gObjectDirectory/approm.o" );
	}
	else
	{
		rename( "$gObjectDirectory/approm.code.o", "$gObjectDirectory/approm.o" );
	}
}


#--------------------------------------------------
# BuildBootROM - build the boot box rom (ready for downlaod)
#--------------------------------------------------
sub BuildBootROM
{
	print STDOUT "     Creating ROMFS...\n";
	&Execute ("./objects/tools/romify -progress -romtop=0x9FE00000 ./ROM_BOOT > $gObjectDirectory/ROMFS") ;

	print STDOUT "     Creating bootrom.o...\n";
	&Execute( "./objects/tools/buildrom -s 0x00200000 -v $gBuildNumber -b 0x9FC00000 -c $gObjectDirectory/bootrom.ld -f $gObjectDirectory/ROMFS -o $gObjectDirectory/bootrom.o $gRedirection" );
}



#--------------------------------------------------
# BuildUniROM - build the unified rom (ready for downlaod)
#--------------------------------------------------
sub BuildUniROM
{
	local($appRomPath) = $gObjectDirectory;
	local($bootRomPath) = $gObjectDirectory;
	$appRomPath =~ s|/unirom|/app| ;
	$bootRomPath =~ s|/unirom|/boot| ;
	&Execute( "cat $bootRomPath/bootrom.o $appRomPath/approm.o > $gFullPathObjectFile" );
}

#--------------------------------------------------
# Execute - execute a shell command
#    This routine exists only so every "system" call 
#    goes through here (and it's easy to turn off).
#--------------------------------------------------
sub Execute
{
    local($cmd) = @_;
    local($result);

    if ( $gNoExecution || $gScriptDebugging )
	{ 
		print STDERR "=> cmd is: ", $cmd, "\n"; 
	}

    if ( !$gNoExecution )
    {
		$result = system( $cmd );
		#if ($?) { exit ($? / 256); }
		if ($gScriptDebugging)
		{
			print STDERR "# finished executing the cmd: $cmd\n";
		}
		return $result; 	# pass on the result of the command...
    }
}


#--------------------------------------------------
# CalculateElapseTime - generate string with elapsed time
#--------------------------------------------------
sub CalculateElapsedTime
{
	local($starttime, $endtime) = @_;
	
	local($elapsedtime, $seconds, $minutes, $hours, $temp, $result);
	$elapsedtime = $endtime - $starttime;
	
	$seconds = $elapsedtime % 60;
	$temp = $elapsedtime / 60;
	$minutes = $temp % 60;
	$hours = $temp / 60;
	
	if ( $minutes < 1 )
	{
		$result = sprintf( "%d seconds", $seconds );
	}
	elsif ( $hours < 1 )
	{
		$result = sprintf( "%d:%02d (mm:ss)", $minutes, $seconds );
	}
	else
	{
		$result = sprintf( "%d:%02d:%02d (hh:mm:ss)", $hours, $minutes, $seconds );
	}
	$result;
}

#--------------------------------------------------
# HelpMessage - prints the legal options - help info
#--------------------------------------------------
sub HelpMessage
{
    local($exitCode) = @_;

	print "Usage: $gScriptName (options) \n";
	print "     -help               # this message \n";
	print "     -debug              # work on the debug version of the roms (as apposed to non-debug) \n";
	print "     -approm             # create the approm image \n";
	print "     -bootrom            # create the bootrom image \n";
	print "     -build=#            # set the build number in the rom image \n";
	print "     -debugScript        # help debug this script\n";
	print "     -NOEXEC             # DO NOT execute system calls\n";
	print "\n\n";
	print "Examples: \n";
	print "    $gScriptName -approm -build=1;  # builds an approm image and stuff #1 in the build long word \n";
	print "    $gScriptName -bootrom  # build the bootrom image. \n";
	print "\n\n";

    exit $exitCode;
}
