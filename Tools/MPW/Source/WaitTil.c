/*
 * WaitTil time
 *
 */


#include <stdio.h>
#include <CursorCtl.h>
#include <Script.h>
//#include <Events.h>
//#include <Dialogs.h>

char Usage[] = "# Usage - %s time\n";

#define SecsPerDay (24 * 60 * 60)

int main(int argc, char* argv[])
	{
	char* toolname = argv[0];
	char* time = argv[1];
	long timelen, lenused;
	DateCacheRecord datecache;
	String2DateStatus status;
	LongDateRec date;
	LongDateTime secs;
	unsigned long now, then;

	if (argc != 2)
		{
		  fprintf(stderr, "### %s - wrong number of arguments\n", toolname);
		  fprintf(stderr, Usage, toolname);
		  return 1;
		}

	InitDateCache(&datecache);

	c2pstr(time);
	timelen = Length(time);


#if 0
	status = String2Date(&time[1], timelen, &datecache, &lenused, &date);
fprintf(stderr, "### String2Date:  status = %#x, lenused = %d,\n", status, lenused);
fprintf(stderr, "\tyear = %d, month = %d, day = %d, hour = %d, minute = %d, second = %d,\n",
				date.ld.year, date.ld.month, date.ld.day, date.ld.hour, date.ld.minute, date.ld.second);
fprintf(stderr, "\tdayOfWeek = %d, dayOfYear = %d, weekOfYear = %d, pm = %d\n",
				date.ld.dayOfWeek, date.ld.dayOfYear, date.ld.weekOfYear, date.ld.pm);
#endif

	date.ld.era = 0;
	GetDateTime(&now);
	Secs2Date(now, &date.od.oldDate);

	status = String2Time(&time[1], timelen, &datecache, &lenused, &date);
#if 0
fprintf(stderr, "### String2Time:  status = %#x, lenused = %d,\n", status, lenused);
fprintf(stderr, "\tyear = %d, month = %d, day = %d, hour = %d, minute = %d, second = %d,\n",
				date.ld.year, date.ld.month, date.ld.day, date.ld.hour, date.ld.minute, date.ld.second);
fprintf(stderr, "\tdayOfWeek = %d, dayOfYear = %d, weekOfYear = %d, pm = %d\n",
				date.ld.dayOfWeek, date.ld.dayOfYear, date.ld.weekOfYear, date.ld.pm);
#endif
	if (status != 0)
		{
		fprintf(stderr, "### %s - CanÕt parse time:  Ò%PÓ\n", toolname, time);
		return 2;
		}

	LongDate2Secs(&date, &secs);
	then = secs;

	if (now > then)
		then += SecsPerDay;
	if (now > then)
		{
		fprintf(stderr, "### %s - internal wierdness:  time not today?\n", toolname);
		return 2;
		}

//	dp = GetNewDialog(128, nil, (WindowPtr)-1);

//	if (dp)
		while (now < then)
			{
			GetDateTime(&now);
			SpinCursor(1);
			}

//	DisposDialog(dp);

	return 0;
	}

void ecvt() {}
void fcvt() {}
