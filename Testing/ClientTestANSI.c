// ===========================================================================
//	ClientTestANSI.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CLIENTTEST_H__
#include "ClientTest.h"
#endif


#ifdef TEST_CLIENTTEST
// ---------------------------------------------------------------------------

static Boolean ClientTestANSI_atol(ClientTestStream* stream);
static Boolean ClientTestANSI_ctype(ClientTestStream* stream);
static Boolean ClientTestANSI_memchr(ClientTestStream* stream);
static Boolean ClientTestANSI_memcmp(ClientTestStream* stream);
static Boolean ClientTestANSI_memcpy(ClientTestStream* stream);
static Boolean ClientTestANSI_memmove(ClientTestStream* stream);
static Boolean ClientTestANSI_memset(ClientTestStream* stream);
static Boolean ClientTestANSI_printf(ClientTestStream* stream);
static Boolean ClientTestANSI_sscanf(ClientTestStream* stream);
static Boolean ClientTestANSI_strcat(ClientTestStream* stream);
static Boolean ClientTestANSI_strchr(ClientTestStream* stream);
static Boolean ClientTestANSI_strcmp(ClientTestStream* stream);
static Boolean ClientTestANSI_strcpy(ClientTestStream* stream);
static Boolean ClientTestANSI_strlen(ClientTestStream* stream);
static Boolean ClientTestANSI_strncat(ClientTestStream* stream);
static Boolean ClientTestANSI_strncmp(ClientTestStream* stream);
static Boolean ClientTestANSI_strncpy(ClientTestStream* stream);
static Boolean ClientTestANSI_strrchr(ClientTestStream* stream);
static Boolean ClientTestANSI_strstr(ClientTestStream* stream);
static Boolean ClientTestANSI_strtol(ClientTestStream* stream);
static Boolean ClientTestANSI_strtoul(ClientTestStream* stream);
static Boolean ClientTestANSI_printSubUnitResult(
						ClientTestStream* stream, const char* testTitle,
						int testsPassed, int testsAttempted);

// ---------------------------------------------------------------------------
//	test atol()  (Since atoi is just coercing atol to int, I'm saving myself
//	a wee bit of time and space by not bothering to test atoi)

struct atolTestType {
	const char* string;
	long num;
};

static const atolTestType atolTests[] =
{
	{"0",					0},
	{"1",					1},
	{"+1",					1},
	{"-1",					-1},
	{"   1234",				1234},
	{"    +  1234",			0},
	{"           -1234",	-1234},
	{"32767",				32767},
	{"32768",				32768},
	{"-32767",				-32767},
	{"-32768",				-32768},
	{"2147483647",			2147483647},
	{"2147483648",			2147483647},
	{"2147483649",			2147483647},
	{"2147483650",			2147483647},
	{"-2147483647",			-2147483647},
	{"-2147483648",			-2147483648},
	{"-2147483649",			-2147483648},
	{"-2147483650",			-2147483648},
	{"1000000000000000",	LONG_MAX},
	{"-1000000000000000",	LONG_MIN}
};

static Boolean
ClientTestANSI_atol(ClientTestStream* stream)
{
	int numTests = sizeof(atolTests) / sizeof(atolTests[0]);
	int testsAttempted = 0;
	int testsPassed = 0;

	while (testsAttempted < numTests) {
		long value = atol(atolTests[testsAttempted].string);
		if (IsError(value != atolTests[testsAttempted].num)) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_atol():  [Test %d] atoi(%s) failed (returned %d)\n",
					testsAttempted+1, atolTests[testsAttempted].string, value);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_atol():  [Test %d] atoi(%s) passed\n",
					testsAttempted+1, atolTests[testsAttempted].string);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_atol",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test basic ctype properties

static Boolean
ClientTestANSI_ctype(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;
	const char* s;
	int c;

	/* test basic types for inclusion */
	
	for (s="0123456789"; *s; ++s) {
		if (IsError(!(isdigit(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  isdigit(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  isdigit(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	for (s="0123456789abcdefABCDEF"; *s; ++s) {
		if (IsError(!(isxdigit(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  isxdigit(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  isxdigit(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	for (s="abcdefghijklmnopqrstuvwxyz"; *s; ++s) {
		if (IsError(!(islower(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  islower(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  islower(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	for (s="ABCDEFGHIJKLMNOPQRSTUVWXYZ"; *s; ++s) {
		if (IsError(!(isupper(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  isupper(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  isupper(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	for (s="!\"#%&'();<=>?[//]*+,-./:^_{|}~"; *s; ++s) {
		if (IsError(!(ispunct(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  ispunct(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  ispunct(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}
	
	for (s=" \f\n\r\t\v"; *s; ++s) {
		if (IsError(!(isspace(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  isspace(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  isspace(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}

	for (s="\a\b\f\n\r\t\v"; *s; ++s) {
		if (IsError(!(iscntrl(*s)))) {
			stream->LevelPrintf(kClientTestPrintLevelFailedTests,
					"ClientTestANSI_ctype():  iscntrl(ch) for ch=%d failed\n",
					*s);
		} else {
			stream->LevelPrintf(kClientTestPrintLevelAllTests,
					"ClientTestANSI_ctype():  iscntrl(ch) for ch=%d passed\n",
					*s);
			testsPassed++;
		}
		testsAttempted++;
	}

	/* test for membership */

	for (c=-1; c<=255; ++c) {
		if (isupper(c) || islower(c)) {
			if (IsError(!(isalpha(c)))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  isupper(c) or islower(c) is true, so isalpha(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  isupper(c) or islower(c) is true, so isalpha(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
		if (isdigit(c) || isalpha(c)) {
			if (IsError(!(isalnum(c)))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  isdigit(c) or isalpha(c) is true, so isalnum(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  isdigit(c) or isalpha(c) is true, so isalnum(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
		if (isalnum(c) || ispunct(c)) {
			if (IsError(!(isgraph(c)))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  isalnum(c) or ispunct(c) is true, so isgraph(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  isalnum(c) or ispunct(c) is true, so isgraph(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
		if (isgraph(c) || (c==' ')) {
			if (IsError(!(isprint(c)))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  isgraph(c) is true, so isprint(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  isgraph(c) is true, so isprint(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
		if (isspace(c) && (c!=' ')) {
			if (IsError(isprint(c))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  isspace(c) is true, so isprint(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  isspace(c) is true, so isprint(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
		if (iscntrl(c)) {
			if (IsError(isalnum(c))) {
				stream->LevelPrintf(kClientTestPrintLevelFailedTests,
						"ClientTestANSI_ctype():  iscntrl(c) is true, so isalnum(c) for c=%d failed\n",
						c);
			} else {
				stream->LevelPrintf(kClientTestPrintLevelAllTests,
						"ClientTestANSI_ctype():  iscntrl(c) is true, so isalnum(c) for c=%d passed\n",
						c);
				testsPassed++;
			}
			testsAttempted++;
		}
	}
	
	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_ctype",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test memchr()

static Boolean
ClientTestANSI_memchr(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	const char testString[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	for (int start=0; start<sizeof(long); start++) {
		for (int length=0; length<13+sizeof(long); length++) {
			testsAttempted++;
			const char* expectedResult = (start + length > 'M' - 'A') ? &(testString['M'-'A']) : nil;
			const char* actualResult = (const char*)memchr(&(testString[start]), 'M', length);
			Boolean err = IsError(actualResult != expectedResult);
			
			stream->LevelPrintf(err ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
						"ClientTestANSI_memchr():  test[%d]  memchr(\"%.*s...\", 'M', %d) = %s found -- %s\n",
						testsAttempted,
						length+2,
						&(testString[start]),
						length,
						(actualResult == nil) ? "not " : "",
						err ? "failed" : "passed");
			if (!err)
				testsPassed++;
		}
	}
	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_memchr",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test memcmp()

static Boolean
ClientTestANSI_memcmp(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;
	const char testString1[] = "1234\06789";
	const char testString2[] = "1234\067891234\067891234\067891234\067891234\06789";

	for (int start=0; start<(sizeof(testString1)-1)*4; start++) {
		for (int length=0; length<=sizeof(testString1); length++) {
			int expected      = ((length == sizeof(testString1)) ||
						         ((length > 0) && (start % (sizeof(testString1)-1) != 0)))
								 ? -1 : 0;
			int result        = memcmp(testString1, &(testString2[start]), length);
			char expectedChar = (expected > 0) ? '>' : ((expected==0) ? '=' : '<');
			char resultChar   = (result > 0)   ? '>' : ((result==0)   ? '=' : '<');
			Boolean err       = IsError(expectedChar != resultChar);
			
			testsAttempted++;
			stream->LevelPrintf(err ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
						"ClientTestANSI_memcmp():  test[%d] memchr(s1, &s2[%d], %d)%c0, expected %c0 -- %s\n",
						testsAttempted,
						start,
						length,
						resultChar,
						expectedChar,
						err ? "failed" : "passed");
			if (!err) testsPassed++;
			
			expected     = -expected;
			result       = memcmp(&(testString2[start]), testString1, length);
			expectedChar = (expected > 0) ? '>' : ((expected==0) ? '=' : '<');
			resultChar   = (result > 0)   ? '>' : ((result==0)   ? '=' : '<');
			err          = IsError(expectedChar != resultChar);
			
			testsAttempted++;
			stream->LevelPrintf(err ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
						"ClientTestANSI_memcmp():  test[%d] memchr(s2[%d], s1, %d)%c0, expected %c0 -- %s\n",
						testsAttempted,
						start,
						length,
						resultChar,
						expectedChar,
						err ? "failed" : "passed");
			if (!err) testsPassed++;
		}
	}

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_memcmp",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test memcpy()

static Boolean
ClientTestANSI_memcpy(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	char buffer[26];
	const char* const kTestString[] = {
		"abcdefghijklmnopqrstuvwxyz",
		"aabcdefghijklmnopqrstuvwxz",
		"aaabcdefghijklmnopqrstuvxz",
		"aaaabcdefghijklmnopqrstvxz",
		"aaaaabcdefghijklmnopqrtvxz",
		"aaaaaabcdefghijklmnoprtvxz",
		"aaaaaaabcdefghijklmnprtvxz",
		"aaaaaaaabcdefghijklnprtvxz",
		"aaaaaaaaabcdefghijlnprtvxz",
		"aaaaaaaaaabcdefghjlnprtvxz",
		"aaaaaaaaaaabcdefhjlnprtvxz",
		"aaaaaaaaaaaabcdfhjlnprtvxz",
		"aaaaaaaaaaaaabdfhjlnprtvxz",
		"aaaaaaaaaaaaabdfhjlnprtvxz"
	};
	
	int index = 0;
	int length = 26;
	
	while (length>=0) {
		memcpy(&(buffer[index]), &((kTestString[index])[index]), length);
		int match = memcmp(buffer, kTestString[index], 26);
		
		testsAttempted++;
		stream->LevelPrintf((match != 0) ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
					"ClientTestANSI_memcpy():  test[%d] memcpy(%x, %x, %d) %s\n",
					testsAttempted,
					&(buffer[index]),
					&((kTestString[index])[index]),
					length,
					(match != 0) ? "failed" : "passed");
		if (match == 0) testsPassed++;

		length -= 2;
		index++;
	}

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_memcpy",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test memmove()

static Boolean
ClientTestANSI_memmove(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	char buffer[26];
	const char* const kTestString[] = {
		"abcdefghijklmnopqrstuvwxyz",
		"aabcdefghijklmnopqrstuvwxz",
		"aaabcdefghijklmnopqrstuvxz",
		"aaaabcdefghijklmnopqrstvxz",
		"aaaaabcdefghijklmnopqrtvxz",
		"aaaaaabcdefghijklmnoprtvxz",
		"aaaaaaabcdefghijklmnprtvxz",
		"aaaaaaaabcdefghijklnprtvxz",
		"aaaaaaaaabcdefghijlnprtvxz",
		"aaaaaaaaaabcdefghjlnprtvxz",
		"aaaaaaaaaaabcdefhjlnprtvxz",
		"aaaaaaaaaaaabcdfhjlnprtvxz",
		"aaaaaaaaaaaaabdfhjlnprtvxz",
		"aaaaaaaaaaaaabdfhjlnprtvxz"
	};

	int index = 1;
	int length = 24;
	int match = 0;
	
	memcpy(buffer, kTestString[0], 26);
	while (length>=0) {
		memmove(&(buffer[index]), &(buffer[index-1]), length);
		match = memcmp(buffer, kTestString[index], 26);
		
		testsAttempted++;
		stream->LevelPrintf((match != 0) ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
					"ClientTestANSI_memmove():  test[%d] memmove(%x, %x, %d) %s\n",
					testsAttempted,
					&(buffer[index]),
					&((kTestString[index])[index]),
					length,
					(match != 0) ? "failed" : "passed");
		if (match == 0) testsPassed++;

		length -= 2;
		index++;
	}
	
	// try moving left and right overlapping
	const char* const kMoreTestString[] = {
		"abcdefghijklmnopqrstuvwxyz",
		"aabcdefghijklmnopqrstuvwxy",
		"abcdefghijklmnopqrstuvwxyy"
	};
	
	memmove(buffer, kMoreTestString[0], 26);
	
	memmove(&(buffer[1]), &(buffer[0]), 25);
	match = memcmp(buffer, kMoreTestString[1], 26);
	testsAttempted++;
	stream->LevelPrintf((match != 0) ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
				"ClientTestANSI_memmove():  test[%d] memmove(&buf[1], &buf[0], 25) %s\n",
				testsAttempted,
				(match != 0) ? "failed" : "passed");
	if (match == 0) testsPassed++;

	memmove(&(buffer[0]), &(buffer[1]), 25);
	match = memcmp(buffer, kMoreTestString[2], 26);
	testsAttempted++;
	stream->LevelPrintf((match != 0) ? kClientTestPrintLevelFailedTests : kClientTestPrintLevelAllTests,
				"ClientTestANSI_memmove():  test[%d] memmove(&buf[0], &buf[1], 25) %s\n",
				testsAttempted,
				(match != 0) ? "failed" : "passed");
	if (match == 0) testsPassed++;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_memmove",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test memset()

static Boolean
ClientTestANSI_memset(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_memset",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test printf() and variants

static Boolean
ClientTestANSI_printf(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_printf",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test scanf()

static Boolean
ClientTestANSI_sscanf(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_sscanf",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strcat()

static Boolean
ClientTestANSI_strcat(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strcat",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strchr()

static Boolean
ClientTestANSI_strchr(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strchr",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strcmp()

static Boolean
ClientTestANSI_strcmp(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strcmp",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strcpy()

static Boolean
ClientTestANSI_strcpy(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strcpy",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strlen()

static Boolean
ClientTestANSI_strlen(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strlen",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strncat()

static Boolean
ClientTestANSI_strncat(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strncat",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strncmp()

static Boolean
ClientTestANSI_strncmp(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strncmp",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strncpy()

static Boolean
ClientTestANSI_strncpy(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strncpy",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strrchr()

static Boolean
ClientTestANSI_strrchr(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strrchr",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strncpy()

static Boolean
ClientTestANSI_strstr(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strstr",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strtol()

static Boolean
ClientTestANSI_strtol(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strtol",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	test strtoul()

static Boolean
ClientTestANSI_strtoul(ClientTestStream* stream)
{
	int testsAttempted = 0;
	int testsPassed = 0;

	return ClientTestANSI_printSubUnitResult(stream, "ClientTestANSI_strtoul",
										testsPassed, testsAttempted);
}

// ---------------------------------------------------------------------------
//	print out results of a test, subunit

static Boolean
ClientTestANSI_printSubUnitResult(ClientTestStream* stream, const char* testTitle,
							int testsPassed, int testsAttempted)
{
	Boolean result = (testsAttempted == testsPassed);
	
	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail,
					"%s:  %s",
					testTitle,
					result ? "pass" : "***** fail *****");

	stream->LevelPrintf(kClientTestPrintLevelScore,
					" (passed %d of %d tests)", testsPassed, testsAttempted);

	stream->LevelPrintf(kClientTestPrintLevelSubUnitPassFail, "\n");

	return result;
}


// ---------------------------------------------------------------------------
//	test all of ANSI()

Boolean
ClientTestANSI(ClientTestStream* stream)
{
	Boolean fail = false;

	fail = (!ClientTestANSI_atol(stream)) || fail;
	fail = (!ClientTestANSI_ctype(stream)) || fail;
	fail = (!ClientTestANSI_memchr(stream)) || fail;
	fail = (!ClientTestANSI_memcmp(stream)) || fail;
	fail = (!ClientTestANSI_memcpy(stream)) || fail;
	fail = (!ClientTestANSI_memmove(stream)) || fail;
	fail = (!ClientTestANSI_memset(stream)) || fail;
	fail = (!ClientTestANSI_printf(stream)) || fail;
	fail = (!ClientTestANSI_sscanf(stream)) || fail;
	fail = (!ClientTestANSI_strcat(stream)) || fail;
	fail = (!ClientTestANSI_strchr(stream)) || fail;
	fail = (!ClientTestANSI_strcmp(stream)) || fail;
	fail = (!ClientTestANSI_strcpy(stream)) || fail;
	fail = (!ClientTestANSI_strlen(stream)) || fail;
	fail = (!ClientTestANSI_strncat(stream)) || fail;
	fail = (!ClientTestANSI_strncmp(stream)) || fail;
	fail = (!ClientTestANSI_strncpy(stream)) || fail;
	fail = (!ClientTestANSI_strrchr(stream)) || fail;
	fail = (!ClientTestANSI_strstr(stream)) || fail;
	fail = (!ClientTestANSI_strtol(stream)) || fail;
	fail = (!ClientTestANSI_strtoul(stream)) || fail;
	return !fail;
}

// ---------------------------------------------------------------------------
#endif /* TEST_CLIENTTEST */
