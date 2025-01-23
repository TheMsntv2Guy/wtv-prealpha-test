// ===========================================================================
//	MPFilter.c
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MPEGAUDIO_H__
#include "MPEGAudio.h"
#endif

/*
 *  @(#) synthesis_filter.cc 1.14, last edit: 6/21/94 11:22:20
 *  @(#) Copyright (C) 1993, 1994 Tobias Bading (bading@cs.tu-berlin.de)
 *  @(#) Berlin University of Technology
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *  Changes from version 1.1 to 1.2:
 *    - compute_new_v() uses a 32 point fast cosine transform as described by
 *      Byeong Gi Lee in IEEE Transactions ASSP-32 Part 2, August 1984,
 *      "A New Algorithm to Compute the Discrete Cosine Transform"
 *      instead of the matrix-vector multiplication in V1.1
 *    - loop unrolling done in ComputeSamples()
 *    - if ULAW is defined, the synthesis filter does a downsampling
 *      to 8 kHz by dropping samples and ignoring subbands above 4 kHz
 */

#define real double

#define kCOSBits 11

#ifdef INTDCT

#define cos1_64 1025 //   0.500603 to   0.500488
#define cos3_64 1035 //   0.505471 to   0.505371
#define cos5_64 1056 //   0.515447 to   0.515625
#define cos7_64 1088 //   0.531043 to   0.531250
#define cos9_64 1133 //   0.553104 to   0.553223
#define cos11_64 1194 //   0.582935 to   0.583008
#define cos13_64 1275 //   0.622504 to   0.622559
#define cos15_64 1382 //   0.674808 to   0.674805
#define cos17_64 1525 //   0.744536 to   0.744629
#define cos19_64 1719 //   0.839350 to   0.839355
#define cos21_64 1992 //   0.972568 to   0.972656
#define cos23_64 2395 //   1.169440 to   1.169434
#define cos25_64 3040 //   1.484165 to   1.484375
#define cos27_64 4214 //   2.057781 to   2.057617
#define cos29_64 6979 //   3.407608 to   3.407715
#define cos31_64 20869 //  10.190008 to  10.189941
#define cos1_32 1029 //   0.502419 to   0.502441
#define cos3_32 1070 //   0.522499 to   0.522461
#define cos5_32 1161 //   0.566944 to   0.566895
#define cos7_32 1325 //   0.646822 to   0.646973
#define cos9_32 1614 //   0.788155 to   0.788086
#define cos11_32 2172 //   1.060678 to   1.060547
#define cos13_32 3528 //   1.722447 to   1.722656
#define cos15_32 10447 //   5.101149 to   5.101074
#define cos1_16 1044 //   0.509796 to   0.509766
#define cos3_16 1232 //   0.601345 to   0.601562
#define cos5_16 1843 //   0.899976 to   0.899902
#define cos7_16 5249 //   2.562915 to   2.562988
#define cos1_8 1108 //   0.541196 to   0.541016
#define cos3_8 2676 //   1.306563 to   1.306641
#define cos1_4 1448 //   0.707107 to   0.707031
#define cos1_64 1025 //   0.500603 to   0.500488
#define cos3_64 1035 //   0.505471 to   0.505371
#define cos5_64 1056 //   0.515447 to   0.515625
#define cos7_64 1088 //   0.531043 to   0.531250
#define cos9_64 1133 //   0.553104 to   0.553223
#define cos11_64 1194 //   0.582935 to   0.583008
#define cos13_64 1275 //   0.622504 to   0.622559
#define cos15_64 1382 //   0.674808 to   0.674805
#define cos17_64 1525 //   0.744536 to   0.744629
#define cos19_64 1719 //   0.839350 to   0.839355
#define cos21_64 1992 //   0.972568 to   0.972656
#define cos23_64 2395 //   1.169440 to   1.169434
#define cos25_64 3040 //   1.484165 to   1.484375
#define cos27_64 4214 //   2.057781 to   2.057617
#define cos29_64 6979 //   3.407608 to   3.407715
#define cos31_64 20869 //  10.190008 to  10.189941
#define cos1_32 1029 //   0.502419 to   0.502441
#define cos3_32 1070 //   0.522499 to   0.522461
#define cos5_32 1161 //   0.566944 to   0.566895
#define cos7_32 1325 //   0.646822 to   0.646973
#define cos9_32 1614 //   0.788155 to   0.788086
#define cos11_32 2172 //   1.060678 to   1.060547
#define cos13_32 3528 //   1.722447 to   1.722656
#define cos15_32 10447 //   5.101149 to   5.101074
#define cos1_16 1044 //   0.509796 to   0.509766
#define cos3_16 1232 //   0.601345 to   0.601562
#define cos5_16 1843 //   0.899976 to   0.899902
#define cos7_16 5249 //   2.562915 to   2.562988
#define cos1_8 1108 //   0.541196 to   0.541016
#define cos3_8 2676 //   1.306563 to   1.306641
#define cos1_4 1448 //   0.707107 to   0.707031

#else

static const real MY_PI = 3.14159265358979323846;

static const real cos1_64  = 1.0 / (2.0 * cos(MY_PI        / 64.0));
static const real cos3_64  = 1.0 / (2.0 * cos(MY_PI * 3.0  / 64.0));
static const real cos5_64  = 1.0 / (2.0 * cos(MY_PI * 5.0  / 64.0));
static const real cos7_64  = 1.0 / (2.0 * cos(MY_PI * 7.0  / 64.0));
static const real cos9_64  = 1.0 / (2.0 * cos(MY_PI * 9.0  / 64.0));
static const real cos11_64 = 1.0 / (2.0 * cos(MY_PI * 11.0 / 64.0));
static const real cos13_64 = 1.0 / (2.0 * cos(MY_PI * 13.0 / 64.0));
static const real cos15_64 = 1.0 / (2.0 * cos(MY_PI * 15.0 / 64.0));
static const real cos17_64 = 1.0 / (2.0 * cos(MY_PI * 17.0 / 64.0));
static const real cos19_64 = 1.0 / (2.0 * cos(MY_PI * 19.0 / 64.0));
static const real cos21_64 = 1.0 / (2.0 * cos(MY_PI * 21.0 / 64.0));
static const real cos23_64 = 1.0 / (2.0 * cos(MY_PI * 23.0 / 64.0));
static const real cos25_64 = 1.0 / (2.0 * cos(MY_PI * 25.0 / 64.0));
static const real cos27_64 = 1.0 / (2.0 * cos(MY_PI * 27.0 / 64.0));
static const real cos29_64 = 1.0 / (2.0 * cos(MY_PI * 29.0 / 64.0));
static const real cos31_64 = 1.0 / (2.0 * cos(MY_PI * 31.0 / 64.0));

static const real cos1_32  = 1.0 / (2.0 * cos(MY_PI        / 32.0));
static const real cos3_32  = 1.0 / (2.0 * cos(MY_PI * 3.0  / 32.0));
static const real cos5_32  = 1.0 / (2.0 * cos(MY_PI * 5.0  / 32.0));
static const real cos7_32  = 1.0 / (2.0 * cos(MY_PI * 7.0  / 32.0));
static const real cos9_32  = 1.0 / (2.0 * cos(MY_PI * 9.0  / 32.0));
static const real cos11_32 = 1.0 / (2.0 * cos(MY_PI * 11.0 / 32.0));
static const real cos13_32 = 1.0 / (2.0 * cos(MY_PI * 13.0 / 32.0));
static const real cos15_32 = 1.0 / (2.0 * cos(MY_PI * 15.0 / 32.0));

static const real cos1_16  = 1.0 / (2.0 * cos(MY_PI        / 16.0));
static const real cos3_16  = 1.0 / (2.0 * cos(MY_PI * 3.0  / 16.0));
static const real cos5_16  = 1.0 / (2.0 * cos(MY_PI * 5.0  / 16.0));
static const real cos7_16  = 1.0 / (2.0 * cos(MY_PI * 7.0  / 16.0));

static const real cos1_8   = 1.0 / (2.0 * cos(MY_PI        / 8.0));
static const real cos3_8   = 1.0 / (2.0 * cos(MY_PI * 3.0  / 8.0));
static const real cos1_4   = 1.0 / (2.0 * cos(MY_PI / 4.0));

#endif



#ifdef INTSYNTH

const long SynthesisFilter::fD[512] = {
     0,   -29,   213,  -459,  2037, -5153,  6574,-37489, 75038, 37489,  6574,  5153,  2037,   459,   213,    29,
    -1,   -31,   218,  -519,  2000, -5517,  5959,-39336, 74992, 35640,  7134,  4788,  2063,   401,   208,    26,
    -1,   -35,   222,  -581,  1952, -5879,  5288,-41176, 74856, 33791,  7640,  4425,  2080,   347,   202,    24,
    -1,   -38,   225,  -645,  1893, -6237,  4561,-43006, 74630, 31947,  8092,  4063,  2087,   294,   196,    21,
    -1,   -41,   227,  -711,  1822, -6589,  3776,-44821, 74313, 30112,  8492,  3705,  2085,   244,   190,    19,
    -1,   -45,   228,  -779,  1739, -6935,  2935,-46617, 73908, 28289,  8840,  3351,  2075,   197,   183,    17,
    -1,   -49,   228,  -848,  1644, -7271,  2037,-48390, 73415, 26482,  9139,  3004,  2057,   153,   176,    16,
    -2,   -53,   227,  -919,  1535, -7597,  1082,-50137, 72835, 24694,  9389,  2663,  2032,   111,   169,    14,
    -2,   -58,   224,  -991,  1414, -7910,    70,-51853, 72169, 22929,  9592,  2330,  2001,    72,   161,    13,
    -2,   -63,   221, -1064,  1280, -8209,  -998,-53534, 71420, 21189,  9750,  2006,  1962,    36,   154,    11,
    -2,   -68,   215, -1137,  1131, -8491, -2122,-55178, 70590, 19478,  9863,  1692,  1919,     2,   147,    10,
    -3,   -73,   208, -1210,   970, -8755, -3300,-56778, 69679, 17799,  9935,  1388,  1870,   -29,   139,     9,
    -3,   -79,   200, -1283,   794, -8998, -4533,-58333, 68692, 16155,  9966,  1095,  1817,   -57,   132,     8,
    -4,   -85,   189, -1356,   605, -9219, -5818,-59838, 67629, 14548,  9959,   814,  1759,   -83,   125,     7,
    -4,   -91,   177, -1428,   402, -9416, -7154,-61289, 66494, 12980,  9916,   545,  1698,  -106,   117,     7,
    -5,   -97,   163, -1498,   185, -9585, -8540,-62684, 65290, 11455,  9838,   288,  1634,  -127,   111,     6,
    -5,  -104,   146, -1567,   -45, -9727, -9975,-64019, 64019,  9975,  9727,    45,  1567,  -146,   104,     5,
    -6,  -111,   127, -1634,  -288, -9838,-11455,-65290, 62684,  8540,  9585,  -185,  1498,  -163,    97,     5,
    -7,  -117,   106, -1698,  -545, -9916,-12980,-66494, 61289,  7154,  9416,  -402,  1428,  -177,    91,     4,
    -7,  -125,    83, -1759,  -814, -9959,-14548,-67629, 59838,  5818,  9219,  -605,  1356,  -189,    85,     4,
    -8,  -132,    57, -1817, -1095, -9966,-16155,-68692, 58333,  4533,  8998,  -794,  1283,  -200,    79,     3,
    -9,  -139,    29, -1870, -1388, -9935,-17799,-69679, 56778,  3300,  8755,  -970,  1210,  -208,    73,     3,
   -10,  -147,    -2, -1919, -1692, -9863,-19478,-70590, 55178,  2122,  8491, -1131,  1137,  -215,    68,     2,
   -11,  -154,   -36, -1962, -2006, -9750,-21189,-71420, 53534,   998,  8209, -1280,  1064,  -221,    63,     2,
   -13,  -161,   -72, -2001, -2330, -9592,-22929,-72169, 51853,   -70,  7910, -1414,   991,  -224,    58,     2,
   -14,  -169,  -111, -2032, -2663, -9389,-24694,-72835, 50137, -1082,  7597, -1535,   919,  -227,    53,     2,
   -16,  -176,  -153, -2057, -3004, -9139,-26482,-73415, 48390, -2037,  7271, -1644,   848,  -228,    49,     1,
   -17,  -183,  -197, -2075, -3351, -8840,-28289,-73908, 46617, -2935,  6935, -1739,   779,  -228,    45,     1,
   -19,  -190,  -244, -2085, -3705, -8492,-30112,-74313, 44821, -3776,  6589, -1822,   711,  -227,    41,     1,
   -21,  -196,  -294, -2087, -4063, -8092,-31947,-74630, 43006, -4561,  6237, -1893,   645,  -225,    38,     1,
   -24,  -202,  -347, -2080, -4425, -7640,-33791,-74856, 41176, -5288,  5879, -1952,   581,  -222,    35,     1,
   -26,  -208,  -401, -2063, -4788, -7134,-35640,-74992, 39336, -5959,  5517, -2000,   519,  -218,    31,     1
};

#else

const real SynthesisFilter::fD[512] = {
  // Note: These values are not in the same order
  // as in Annex 3-B.3 of the ISO/IEC DIS 11172-3
   0.000000000, -0.000442505,  0.003250122, -0.007003784,
   0.031082153, -0.078628540,  0.100311279, -0.572036743,
   1.144989014,  0.572036743,  0.100311279,  0.078628540,
   0.031082153,  0.007003784,  0.003250122,  0.000442505,
  -0.000015259, -0.000473022,  0.003326416, -0.007919312,
   0.030517578, -0.084182739,  0.090927124, -0.600219727,
   1.144287109,  0.543823242,  0.108856201,  0.073059082,
   0.031478882,  0.006118774,  0.003173828,  0.000396729,
  -0.000015259, -0.000534058,  0.003387451, -0.008865356,
   0.029785156, -0.089706421,  0.080688477, -0.628295898,
   1.142211914,  0.515609741,  0.116577148,  0.067520142,
   0.031738281,  0.005294800,  0.003082275,  0.000366211,
  -0.000015259, -0.000579834,  0.003433228, -0.009841919,
   0.028884888, -0.095169067,  0.069595337, -0.656219482,
   1.138763428,  0.487472534,  0.123474121,  0.061996460,
   0.031845093,  0.004486084,  0.002990723,  0.000320435,
  -0.000015259, -0.000625610,  0.003463745, -0.010848999,
   0.027801514, -0.100540161,  0.057617188, -0.683914185,
   1.133926392,  0.459472656,  0.129577637,  0.056533813,
   0.031814575,  0.003723145,  0.002899170,  0.000289917,
  -0.000015259, -0.000686646,  0.003479004, -0.011886597,
   0.026535034, -0.105819702,  0.044784546, -0.711318970,
   1.127746582,  0.431655884,  0.134887695,  0.051132202,
   0.031661987,  0.003005981,  0.002792358,  0.000259399,
  -0.000015259, -0.000747681,  0.003479004, -0.012939453,
   0.025085449, -0.110946655,  0.031082153, -0.738372803,
   1.120223999,  0.404083252,  0.139450073,  0.045837402,
   0.031387329,  0.002334595,  0.002685547,  0.000244141,
  -0.000030518, -0.000808716,  0.003463745, -0.014022827,
   0.023422241, -0.115921021,  0.016510010, -0.765029907,
   1.111373901,  0.376800537,  0.143264771,  0.040634155,
   0.031005859,  0.001693726,  0.002578735,  0.000213623,
  -0.000030518, -0.000885010,  0.003417969, -0.015121460,
   0.021575928, -0.120697021,  0.001068115, -0.791213989,
   1.101211548,  0.349868774,  0.146362305,  0.035552979,
   0.030532837,  0.001098633,  0.002456665,  0.000198364,
  -0.000030518, -0.000961304,  0.003372192, -0.016235352,
   0.019531250, -0.125259399, -0.015228271, -0.816864014,
   1.089782715,  0.323318481,  0.148773193,  0.030609131,
   0.029937744,  0.000549316,  0.002349854,  0.000167847,
  -0.000030518, -0.001037598,  0.003280640, -0.017349243,
   0.017257690, -0.129562378, -0.032379150, -0.841949463,
   1.077117920,  0.297210693,  0.150497437,  0.025817871,
   0.029281616,  0.000030518,  0.002243042,  0.000152588,
  -0.000045776, -0.001113892,  0.003173828, -0.018463135,
   0.014801025, -0.133590698, -0.050354004, -0.866363525,
   1.063217163,  0.271591187,  0.151596069,  0.021179199,
   0.028533936, -0.000442505,  0.002120972,  0.000137329,
  -0.000045776, -0.001205444,  0.003051758, -0.019577026,
   0.012115479, -0.137298584, -0.069168091, -0.890090942,
   1.048156738,  0.246505737,  0.152069092,  0.016708374,
   0.027725220, -0.000869751,  0.002014160,  0.000122070,
  -0.000061035, -0.001296997,  0.002883911, -0.020690918,
   0.009231567, -0.140670776, -0.088775635, -0.913055420,
   1.031936646,  0.221984863,  0.151962280,  0.012420654,
   0.026840210, -0.001266479,  0.001907349,  0.000106812,
  -0.000061035, -0.001388550,  0.002700806, -0.021789551,
   0.006134033, -0.143676758, -0.109161377, -0.935195923,
   1.014617920,  0.198059082,  0.151306152,  0.008316040,
   0.025909424, -0.001617432,  0.001785278,  0.000106812,
  -0.000076294, -0.001480103,  0.002487183, -0.022857666,
   0.002822876, -0.146255493, -0.130310059, -0.956481934,
   0.996246338,  0.174789429,  0.150115967,  0.004394531,
   0.024932861, -0.001937866,  0.001693726,  0.000091553,
  -0.000076294, -0.001586914,  0.002227783, -0.023910522,
  -0.000686646, -0.148422241, -0.152206421, -0.976852417,
   0.976852417,  0.152206421,  0.148422241,  0.000686646,
   0.023910522, -0.002227783,  0.001586914,  0.000076294,
  -0.000091553, -0.001693726,  0.001937866, -0.024932861,
  -0.004394531, -0.150115967, -0.174789429, -0.996246338,
   0.956481934,  0.130310059,  0.146255493, -0.002822876,
   0.022857666, -0.002487183,  0.001480103,  0.000076294,
  -0.000106812, -0.001785278,  0.001617432, -0.025909424,
  -0.008316040, -0.151306152, -0.198059082, -1.014617920,
   0.935195923,  0.109161377,  0.143676758, -0.006134033,
   0.021789551, -0.002700806,  0.001388550,  0.000061035,
  -0.000106812, -0.001907349,  0.001266479, -0.026840210,
  -0.012420654, -0.151962280, -0.221984863, -1.031936646,
   0.913055420,  0.088775635,  0.140670776, -0.009231567,
   0.020690918, -0.002883911,  0.001296997,  0.000061035,
  -0.000122070, -0.002014160,  0.000869751, -0.027725220,
  -0.016708374, -0.152069092, -0.246505737, -1.048156738,
   0.890090942,  0.069168091,  0.137298584, -0.012115479,
   0.019577026, -0.003051758,  0.001205444,  0.000045776,
  -0.000137329, -0.002120972,  0.000442505, -0.028533936,
  -0.021179199, -0.151596069, -0.271591187, -1.063217163,
   0.866363525,  0.050354004,  0.133590698, -0.014801025,
   0.018463135, -0.003173828,  0.001113892,  0.000045776,
  -0.000152588, -0.002243042, -0.000030518, -0.029281616,
  -0.025817871, -0.150497437, -0.297210693, -1.077117920,
   0.841949463,  0.032379150,  0.129562378, -0.017257690,
   0.017349243, -0.003280640,  0.001037598,  0.000030518,
  -0.000167847, -0.002349854, -0.000549316, -0.029937744,
  -0.030609131, -0.148773193, -0.323318481, -1.089782715,
   0.816864014,  0.015228271,  0.125259399, -0.019531250,
   0.016235352, -0.003372192,  0.000961304,  0.000030518,
  -0.000198364, -0.002456665, -0.001098633, -0.030532837,
  -0.035552979, -0.146362305, -0.349868774, -1.101211548,
   0.791213989, -0.001068115,  0.120697021, -0.021575928,
   0.015121460, -0.003417969,  0.000885010,  0.000030518,
  -0.000213623, -0.002578735, -0.001693726, -0.031005859,
  -0.040634155, -0.143264771, -0.376800537, -1.111373901,
   0.765029907, -0.016510010,  0.115921021, -0.023422241,
   0.014022827, -0.003463745,  0.000808716,  0.000030518,
  -0.000244141, -0.002685547, -0.002334595, -0.031387329,
  -0.045837402, -0.139450073, -0.404083252, -1.120223999,
   0.738372803, -0.031082153,  0.110946655, -0.025085449,
   0.012939453, -0.003479004,  0.000747681,  0.000015259,
  -0.000259399, -0.002792358, -0.003005981, -0.031661987,
  -0.051132202, -0.134887695, -0.431655884, -1.127746582,
   0.711318970, -0.044784546,  0.105819702, -0.026535034,
   0.011886597, -0.003479004,  0.000686646,  0.000015259,
  -0.000289917, -0.002899170, -0.003723145, -0.031814575,
  -0.056533813, -0.129577637, -0.459472656, -1.133926392,
   0.683914185, -0.057617188,  0.100540161, -0.027801514,
   0.010848999, -0.003463745,  0.000625610,  0.000015259,
  -0.000320435, -0.002990723, -0.004486084, -0.031845093,
  -0.061996460, -0.123474121, -0.487472534, -1.138763428,
   0.656219482, -0.069595337,  0.095169067, -0.028884888,
   0.009841919, -0.003433228,  0.000579834,  0.000015259,
  -0.000366211, -0.003082275, -0.005294800, -0.031738281,
  -0.067520142, -0.116577148, -0.515609741, -1.142211914,
   0.628295898, -0.080688477,  0.089706421, -0.029785156,
   0.008865356, -0.003387451,  0.000534058,  0.000015259,
  -0.000396729, -0.003173828, -0.006118774, -0.031478882,
  -0.073059082, -0.108856201, -0.543823242, -1.144287109,
   0.600219727, -0.090927124,  0.084182739, -0.030517578,
   0.007919312, -0.003326416,  0.000473022,  0.000015259
};

#endif


const kScaleFactor = 32768;


SynthesisFilter::SynthesisFilter()
{
  // initialize v1[] and v2[]:

#ifdef INTSYNTH
	ZeroMemory(fV1,sizeof(short) * 512);
	ZeroMemory(fV2,sizeof(short) * 512);
#else
  register real *floatp, *floatp2;
  for (floatp = fV1 + 512, floatp2 = fV2 + 512; floatp > fV1; )
    *--floatp = *--floatp2 = 0.0;
    range_violations = 0;
#endif

  fWrittenSamples = 0;
  fActualV = fV1;
  fWritePos = 15;

//	Make an integer version of d

#if 0
	ManipulateCos();
	printf("\n");
	printf("\n");
	for (short i = 0; i < 512; i++) {
		double r;
		long x;
		if (i % 0x10 == 0)
			printf("\n");
		if (d[i] >= 0)
			x = (int)((d[i] * 65536) + 0.5);
		else
			x = (int)((d[i] * 65536) - 0.5);
		printf("%6d,",x);
	}
#endif
}



#if 1

//#ifdef INTSYNTH
#ifdef INTDCT
#define IMUL(_cos,_val) (((short)_cos*(short)_val) >> kCOSBits)
#else
#define IMUL(_cos,_val) (_cos*_val)
#endif

#define TestSamples(_x)

//void Report()
//{
//	printf("No data\n");
//}

#else

double gMinVal = 1000.0;
double gMaxVal = -1000.0;

double gMinCos = 1000.0;
double gMaxCos = -1000.0;

double gMinTotal = 1000.0;
double gMaxTotal = -1000.0;

double gMinSamp = 1000.0;
double gMaxSamp = -1000.0;

//void Report()
//{
//	printf("IMUL called between %f and %f\n",gMinVal,gMaxVal);
//	printf("IMUL cos between %f and %f\n",gMinCos,gMaxCos);
//	printf("IMUL result called between %f and %f\n",gMinTotal,gMaxTotal);
//	printf("Samples between %f and %f\n",gMinSamp,gMaxSamp);
//}

void TestSamples(double *samples)
{
	for (short i = 0; i < 32; i++) {
		if (samples[i] < gMinSamp)
			gMinSamp = samples[i]; 
		if (samples[i] > gMaxSamp)
			gMaxSamp = samples[i];
	}
}

double IMUL(double _cos,double _val)
{
	if (_val < gMinVal)
		gMinVal = _val; 
	if (_val > gMaxVal)
		gMaxVal = _val; 
	
	_val = _cos*_val;

	if (_val < gMinTotal)
		gMinTotal = _val; 
	if (_val > gMaxTotal)
		gMaxTotal = _val; 

	if (_cos < gMinCos)
		gMinCos = _cos; 
	if (_cos > gMaxCos)
		gMaxCos = _cos; 

	return _val;
}
#endif

void SynthesisFilter::ComputeNewV(long *samples)
{


#ifdef INTDCT
  register short *x1, *x2;
  register short tmp;
  short p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15;
  short pp0, pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13, pp14, pp15;
  short new_v[32];
  
  short samp[32];
	for (short i = 0; i < 32; i++)
		//samp[i] = (short)(samples[i] * (16384.0));
		samp[i] = (short)(samples[i] >> 1);
//		samp[i] = (short)(samples[i] * (32768.0));
#else
  real new_v[32];		// new V[0-15] and V[33-48] of Figure 3-A.2 in ISO DIS 11172-3
  register real *x1, *x2;
  register real tmp;
  real p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15;
  real pp0, pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13, pp14, pp15;
  double *samp = samples;
#endif

  // compute new values via a fast cosine transform:
  x1 = samp;
  x2 = samp + 31;
  p0  = *x1++ + *x2;   p1  = *x1++ + *--x2; p2  = *x1++ + *--x2; p3  = *x1++ + *--x2;
  p4  = *x1++ + *--x2; p5  = *x1++ + *--x2; p6  = *x1++ + *--x2; p7  = *x1++ + *--x2;
  p8  = *x1++ + *--x2; p9  = *x1++ + *--x2; p10 = *x1++ + *--x2; p11 = *x1++ + *--x2;
  p12 = *x1++ + *--x2; p13 = *x1++ + *--x2; p14 = *x1++ + *--x2; p15 = *x1   + *--x2;

  pp0  = p0 + p15; pp1 = p1 + p14; pp2 = p2 + p13; pp3 = p3 + p12;
  pp4  = p4 + p11; pp5 = p5 + p10; pp6 = p6 + p9;  pp7 = p7 + p8;
  pp8  = IMUL(cos1_32  , (p0 - p15));
  pp9  = IMUL(cos3_32  , (p1 - p14));
  pp10 = IMUL(cos5_32  , (p2 - p13));
  pp11 = IMUL(cos7_32  , (p3 - p12));
  pp12 = IMUL(cos9_32  , (p4 - p11));
  pp13 = IMUL(cos11_32 , (p5 - p10));
  pp14 = IMUL(cos13_32 , (p6 - p9));
  pp15 = IMUL(cos15_32 , (p7 - p8));

  p0  = pp0 + pp7; p1 = pp1 + pp6; p2 = pp2 + pp5; p3 = pp3 + pp4;
  p4  = IMUL(cos1_16 , (pp0 - pp7));
  p5  = IMUL(cos3_16 , (pp1 - pp6));
  p6  = IMUL(cos5_16 , (pp2 - pp5));
  p7  = IMUL(cos7_16 , (pp3 - pp4));
  p8  = pp8 + pp15; p9 = pp9 + pp14; p10 = pp10 + pp13; p11 = pp11 + pp12;
  p12 = IMUL(cos1_16 , (pp8  - pp15));
  p13 = IMUL(cos3_16 , (pp9  - pp14));
  p14 = IMUL(cos5_16 , (pp10 - pp13));
  p15 = IMUL(cos7_16 , (pp11 - pp12));

  pp0  = p0 + p3; pp1 = p1 + p2;
  pp2  = IMUL(cos1_8 , (p0 - p3));
  pp3  = IMUL(cos3_8 , (p1 - p2));
  pp4  = p4 + p7; pp5 = p5 + p6;
  pp6  = IMUL(cos1_8 , (p4 - p7));
  pp7  = IMUL(cos3_8 , (p5 - p6));
  pp8  = p8 + p11; pp9 = p9 + p10;
  pp10 = IMUL(cos1_8 , (p8 - p11));
  pp11 = IMUL(cos3_8 , (p9 - p10));
  pp12 = p12 + p15; pp13 = p13 + p14;
  pp14 = IMUL(cos1_8 , (p12 - p15));
  pp15 = IMUL(cos3_8 , (p13 - p14));

  p0 = pp0 + pp1;
  p1 = IMUL(cos1_4 , (pp0 - pp1));
  p2 = pp2 + pp3;
  p3 = IMUL(cos1_4 , (pp2 - pp3));
  p4 = pp4 + pp5;
  p5 = IMUL(cos1_4 , (pp4 - pp5));
  p6 = pp6 + pp7;
  p7 = IMUL(cos1_4 , (pp6 - pp7));
  p8  = pp8 + pp9;
  p9  = IMUL(cos1_4 , (pp8 - pp9));
  p10 = pp10 + pp11;
  p11 = IMUL(cos1_4 , (pp10 - pp11));
  p12 = pp12 + pp13;
  p13 = IMUL(cos1_4 , (pp12 - pp13));
  p14 = pp14 + pp15;
  p15 = IMUL(cos1_4 , (pp14 - pp15));

  tmp          = p6 + p7;
  new_v[36-17] = -(p5 + tmp);
  new_v[44-17] = -(p4 + tmp);
  tmp          = p11 + p15;
  new_v[10]    = tmp;
  new_v[6]     = p13 + tmp;
  tmp          = p14 + p15;
  new_v[46-17] = -(p8  + p12 + tmp);
  new_v[34-17] = -(p9  + p13 + tmp);
  tmp         += p10 + p11;
  new_v[38-17] = -(p13 + tmp);
  new_v[42-17] = -(p12 + tmp);
  new_v[2]     = p9 + p13 + p15;
  new_v[4]     = p5 + p7;
  new_v[48-17] = -p0;
  new_v[0]     = p1;
  new_v[8]     = p3;
  new_v[12]    = p7;
  new_v[14]    = p15;
  new_v[40-17] = -(p2  + p3);

  x1 = samp;
  x2 = samp + 31;
  p0  = IMUL(cos1_64  , (*x1++ - *x2));   p1  = IMUL(cos3_64  , (*x1++ - *--x2));
  p2  = IMUL(cos5_64  , (*x1++ - *--x2)); p3  = IMUL(cos7_64  , (*x1++ - *--x2));
  p4  = IMUL(cos9_64  , (*x1++ - *--x2)); p5  = IMUL(cos11_64 , (*x1++ - *--x2));
  p6  = IMUL(cos13_64 , (*x1++ - *--x2)); p7  = IMUL(cos15_64 , (*x1++ - *--x2));
  p8  = IMUL(cos17_64 , (*x1++ - *--x2)); p9  = IMUL(cos19_64 , (*x1++ - *--x2));
  p10 = IMUL(cos21_64 , (*x1++ - *--x2)); p11 = IMUL(cos23_64 , (*x1++ - *--x2));
  p12 = IMUL(cos25_64 , (*x1++ - *--x2)); p13 = IMUL(cos27_64 , (*x1++ - *--x2));
  p14 = IMUL(cos29_64 , (*x1++ - *--x2)); p15 = IMUL(cos31_64 , (*x1   - *--x2));

  pp0  = p0 + p15; pp1 = p1 + p14; pp2 = p2 + p13; pp3 = p3 + p12;
  pp4  = p4 + p11; pp5 = p5 + p10; pp6 = p6 + p9;  pp7 = p7 + p8;
  pp8  = IMUL(cos1_32  , (p0 - p15));
  pp9  = IMUL(cos3_32  , (p1 - p14));
  pp10 = IMUL(cos5_32  , (p2 - p13));
  pp11 = IMUL(cos7_32  , (p3 - p12));
  pp12 = IMUL(cos9_32  , (p4 - p11));
  pp13 = IMUL(cos11_32 , (p5 - p10));
  pp14 = IMUL(cos13_32 , (p6 - p9));
  pp15 = IMUL(cos15_32 , (p7 - p8));
  
  p0  = pp0 + pp7; p1 = pp1 + pp6; p2 = pp2 + pp5; p3 = pp3 + pp4;
  p4  = IMUL(cos1_16 , (pp0 - pp7));
  p5  = IMUL(cos3_16 , (pp1 - pp6));
  p6  = IMUL(cos5_16 , (pp2 - pp5));
  p7  = IMUL(cos7_16 , (pp3 - pp4));
  p8  = pp8 + pp15; p9 = pp9 + pp14; p10 = pp10 + pp13; p11 = pp11 + pp12;
  p12 = IMUL(cos1_16 , (pp8  - pp15));
  p13 = IMUL(cos3_16 , (pp9  - pp14));
  p14 = IMUL(cos5_16 , (pp10 - pp13));
  p15 = IMUL(cos7_16 , (pp11 - pp12));

  pp0  = p0 + p3; pp1 = p1 + p2;
  pp2  = IMUL(cos1_8 , (p0 - p3));
  pp3  = IMUL(cos3_8 , (p1 - p2));
  pp4  = p4 + p7; pp5 = p5 + p6;
  pp6  = IMUL(cos1_8 , (p4 - p7));
  pp7  = IMUL(cos3_8 , (p5 - p6));
  pp8  = p8 + p11; pp9 = p9 + p10;
  pp10 = IMUL(cos1_8 , (p8 - p11));
  pp11 = IMUL(cos3_8 , (p9 - p10));
  pp12 = p12 + p15; pp13 = p13 + p14;
  pp14 = IMUL(cos1_8 , (p12 - p15));
  pp15 = IMUL(cos3_8 , (p13 - p14));

  p0 = pp0 + pp1;
  p1 = IMUL(cos1_4 , (pp0 - pp1));
  p2 = pp2 + pp3;
  p3 = IMUL(cos1_4 , (pp2 - pp3));
  p4 = pp4 + pp5;
  p5 = IMUL(cos1_4 , (pp4 - pp5));
  p6 = pp6 + pp7;
  p7 = IMUL(cos1_4 , (pp6 - pp7));
  p8  = pp8 + pp9;
  p9  = IMUL(cos1_4 , (pp8 - pp9));
  p10 = pp10 + pp11;
  p11 = IMUL(cos1_4 , (pp10 - pp11));
  p12 = pp12 + pp13;
  p13 = IMUL(cos1_4 , (pp12 - pp13));
  p14 = pp14 + pp15;
  p15 = IMUL(cos1_4 , (pp14 - pp15));

  tmp          = p13 + p15;
  new_v[1]     = p1 + p9 + tmp;
  new_v[5]     = p5 + p7 + p11 + tmp;
  tmp         += p9;
  new_v[33-17] = -(p1 + p14 + tmp);
  tmp         += p5 + p7;
  new_v[3]     = tmp;
  new_v[35-17] = -(p6 + p14 + tmp);
  tmp          = p10 + p11 + p12 + p13 + p14 + p15;
  new_v[39-17] = -(p2 + p3 + tmp - p12);
  new_v[43-17] = -(p4 + p6 + p7 + tmp - p13);
  new_v[37-17] = -(p5 + p6 + p7 + tmp - p12);
  new_v[41-17] = -(p2 + p3 + tmp - p13);
  tmp          = p8 + p12 + p14 + p15;
  new_v[47-17] = -(p0 + tmp);
  new_v[45-17] = -(p4 + p6 + p7 + tmp);
  tmp          = p11 + p15;
  new_v[11]    = p7  + tmp;
  tmp         += p3;
  new_v[9]     = tmp;
  new_v[7]     = p13 + tmp;
  new_v[13]    = p7 + p15;
  new_v[15]    = p15;

	//TestSamples(new_v);
	
#ifdef INTSYNTH
	//================== DO THE INTEGER VERSION ==================

#ifdef INTDCT
	short *new_vInt = new_v;
#else
  short new_vInt[32];
for (short i = 0; i < 32; i++)
//	new_vInt[i] = (short)(new_v[i] * (32768.0));
	new_vInt[i] = (short)(new_v[i] * (16384.0));
#endif

  short *xx1 = new_vInt;
  short *xx2 = fActualV + fWritePos;
  
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1;   xx2 += 16;
  // V[16] is always 0.0:
  *xx2 = 0; xx2 += 16;
  // insert V[17-31] (== -new_v[15-1]) into actual v:
  *xx2 = -*xx1;   xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16;
  *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16;
  *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16;
  *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1; xx2 += 16; *xx2 = -*--xx1;

  // insert V[32] (== -new_v[0]) into other v:
  xx2 = (fActualV == fV1 ? fV2 : fV1) + fWritePos;
  *xx2 = -*--xx1; xx2 += 16;
  // insert V[33-48] (== new_v[16-31]) into other v:

  xx1 = new_vInt + 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16;
  *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1++; xx2 += 16; *xx2 = *xx1;   xx2 += 16;
  // insert V[49-63] (== new_v[30-16]) into other v:
  *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16;
  *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16;
  *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16;
  *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1; xx2 += 16; *xx2 = *--xx1;

#else

	//================== DO THE FLOATING POINT VERSION ==================
	
	// insert V[0-15] (== new_v[0-15]) into actual v:
  
  x1 = new_v;
  x2 = fActualV + fWritePos;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1;   x2 += 16;
  // V[16] is always 0.0:
  *x2 = 0; x2 += 16;
  // insert V[17-31] (== -new_v[15-1]) into actual v:
  *x2 = -*x1;   x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16;
  *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16;
  *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16;
  *x2 = -*--x1; x2 += 16; *x2 = -*--x1; x2 += 16; *x2 = -*--x1;

  // insert V[32] (== -new_v[0]) into other v:
  x2 = (fActualV == fV1 ? fV2 : fV1) + fWritePos;
  *x2 = -*--x1; x2 += 16;
  // insert V[33-48] (== new_v[16-31]) into other v:

  x1 = new_v + 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16;
  *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1++; x2 += 16; *x2 = *x1;   x2 += 16;
  // insert V[49-63] (== new_v[30-16]) into other v:
  *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16;
  *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16;
  *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16;
  *x2 = *--x1; x2 += 16; *x2 = *--x1; x2 += 16; *x2 = *--x1;
#endif
}


typedef long int32;
typedef short int16;

#ifdef INTSYNTH

#define OutSample(_x) _x >>= 15;\
					if (_x > 32767) _x = 32767;\
					if (_x < -32768) _x = -32768;\
					*buffer++ = _x;
#else

#define OutSample(_x) 	pcm_sample = (int)(floatreg * kScaleFactor);\
	if (pcm_sample > 32767)\
	{\
	  ++range_violations;\
	  if (floatreg > max_violation)\
	    max_violation = floatreg;\
	  pcm_sample = 32767;\
	}\
	else if (pcm_sample < -32768)\
	{\
	  ++range_violations;\
	  if (-floatreg > max_violation)\
	    max_violation = -floatreg;\
	  pcm_sample = -32768;\
	}\
	*buffer++ = (int16)pcm_sample;
#endif





#ifdef NOUNROLL

//	Unrolling is worth it

void SynthesisFilter::ComputeSamples(short *buffer) {


#ifdef INTSYNTH
	register long sample;
	short *vp;
	register const long *dp;
#else
	register real sample, *vp;
	register const real *dp;
#endif
	short i,j;

#if 0
	dp = d;
	vp = fActualV + fWritePos;
	for (i = 32; i--;) {
		sample = 0;
		for (j = fWritePos+1; j--;)
			sample += *vp-- * *dp++;
		vp += 16;
		for (j = 15-fWritePos; j--;)
			sample += *vp-- * *dp++;
		vp += 16;
		OutSample(sample);
	}
#endif
	dp = d;
	vp = fActualV + fWritePos;
	for (i = 0; i < 32; i += 2) {
		sample = 0;
		for (j = fWritePos+1; j--;)
			sample += *vp-- * *dp++;
		vp += 16;
		for (j = 15-fWritePos; j--;)
			sample += *vp-- * *dp++;
		vp += 16;
		OutSample(sample);
		OutSample(sample);
		vp += 16;
		dp += 16;
	}

}

#else

void SynthesisFilter::ComputeSamples(ushort *buffer)
{
  	ulong i;

#ifdef INTSYNTH
	register long sample;
	short *vp;
	register const long *dp;
#else
	register real sample, *vp;
	register const real *dp;
	int32 pcm_sample;
#endif

	dp = fD;
	vp = fActualV + fWritePos;
	
	switch (fWritePos) {
		case 0:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 1:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 2:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 3:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 4:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 5:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 6:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 7:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 8:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 9:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 10:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 11:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 12:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 13:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;	 sample += *--vp * *dp++; 
				vp += 15;
				OutSample(sample);
			}
			break;
		case 14:
			for (i= 32; i--;) {
				sample = *vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				vp += 15;
				sample += *vp * *dp++;
				vp += 15;
				OutSample(sample);
			}
			break;
		case 15:
			for (i= 32; i--;) {
				sample = *vp * *dp++;	 sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++;
				sample += *--vp * *dp++; sample += *--vp * *dp++; 
				vp += 31;
				OutSample(sample);
			}
			break;
	}
}

#endif


void SynthesisFilter::CalculatePCMSamples (long *samples,ushort *buffer)
{
	ComputeNewV(samples);
	ComputeSamples(buffer);
	
	fWrittenSamples += 32;
	if (fWritePos < 15)
		++fWritePos;
	else
		fWritePos = 0;
	fActualV = (fActualV == fV1 ? fV2 : fV1);
}














#if 0


void MCos(char *cosName, double cosValue)
{
	long x;
	if (cosValue >= 0)
		x = (int)((cosValue * (1 << kCOSBits)) + 0.5);
	else
		x = (int)((cosValue * (1 << kCOSBits)) - 0.5);
	printf("#define %s %d // %10f to %10f\n",cosName,x,cosValue,(double)x/(1 << kCOSBits));
}

#define ManipCos(_x) MCos(#_x,_x)

void ManipulateCos()
{
	ManipCos(cos1_64);
	ManipCos(cos3_64);
	ManipCos(cos5_64);
	ManipCos(cos7_64);
	ManipCos(cos9_64);
	ManipCos(cos11_64);
	ManipCos(cos13_64);
	ManipCos(cos15_64);
	ManipCos(cos17_64);
	ManipCos(cos19_64);
	ManipCos(cos21_64);
	ManipCos(cos23_64);
	ManipCos(cos25_64);
	ManipCos(cos27_64);
	ManipCos(cos29_64);
	ManipCos(cos31_64);
	
	ManipCos(cos1_32);
	ManipCos(cos3_32);
	ManipCos(cos5_32);
	ManipCos(cos7_32);
	ManipCos(cos9_32);
	ManipCos(cos11_32);
	ManipCos(cos13_32);
	ManipCos(cos15_32);
	
	ManipCos(cos1_16);
	ManipCos(cos3_16);
	ManipCos(cos5_16);
	ManipCos(cos7_16);
	
	ManipCos(cos1_8);
	ManipCos(cos3_8);
	
	ManipCos(cos1_4);
}

#endif


