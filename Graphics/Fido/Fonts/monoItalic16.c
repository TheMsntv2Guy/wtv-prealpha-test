#include "fidoFont.h"

const unsigned long monoItalic16Bits[] = {
// � (1)
0x36995000, 
0x379F4000, 
0x334D0000, 
0x33880000, 
0x33D40000, 
0x34D00000, 
0x38800000, 
0x3D400000, 
0x4F970000, 
0x59960000, 
// M (2)
0x04490467, 0x00000000, 
0x04AF04F8, 0x00000000, 
0x04EF03AF, 0x40000000, 
0x035EF336, 0xED000000, 
0x03AAD33C, 0x98000000, 
0x03E8C399, 0xD4000000, 
0x335E6C4D, 0x4D000000, 
0x33AA6CC7, 0x88000000, 
0x33E56FC3, 0xD4000000, 
0x35E38F64, 0xD0000000, 
0x3AA39B38, 0x80000000, 
0x3E53643D, 0x40000000, 
0x5E044D00, 0x00000000, 
0x67045600, 0x00000000, 
// � (3)
0x37833700, 
0xBCEFBF00, 
0x93378000, 
// M (4)
0x04490467, 0x00000000, 
0x04AF04F8, 0x00000000, 
0x04EF03AF, 0x40000000, 
0x035EF336, 0xED000000, 
0x03AAD33C, 0x98000000, 
0x03E8C399, 0xD4000000, 
0x335E6C4D, 0x4D000000, 
0x33AA6CC7, 0x88000000, 
0x33E56FC3, 0xD4000000, 
0x35E38F64, 0xD0000000, 
0x3AA39B38, 0x80000000, 
0x3E53643D, 0x40000000, 
0x5E044D00, 0x00000000, 
0x67045600, 0x00000000, 
// � (5)
0x03600000, 
0x33BB0000, 
0x3B7F0000, 
0x57374000, 
// ! (33)
0x04470000, 
0x04A80000, 
0x04E40000, 
0x034D0000, 
0x03880000, 
0x03D40000, 
0x334D0000, 
0x33880000, 
0x33D40000, 
0x34D00000, 
0x35600000, 
0x00000000, 
0x37000000, 
0x45000000, 
// # (35)
0x07803800, 0x00000000, 
0x066C336C, 0x00000000, 
0x06C633C6, 0x00000000, 
0x33499BE9, 0x9BE70000, 
0x3369AF99, 0xAF960000, 
0x04993399, 0x00000000, 
0x04E03E00, 0x00000000, 
0x03B733B7, 0x00000000, 
0x335D335D, 0x00000000, 
0x49DB99DB, 0x97000000, 
0x6CC99CC9, 0x96000000, 
0x3E433E40, 0x00000000, 
0x8A338A00, 0x00000000, 
0x80380000, 0x00000000, 
// % (37)
0x04599800, 0x00000000, 
0x037EAAF7, 0x00000000, 
0x03E533F4, 0x33C00000, 
0x335D335D, 0x33C50000, 
0x33AA3398, 0x3C500000, 
0x33BF9BD5, 0xC5000000, 
0x039986B0, 0x00000000, 
0x055C7996, 0x00000000, 
0x045CBD9C, 0xF0000000, 
0x035C6E33, 0x5D000000, 
0x337A3993, 0x38A00000, 
0x37B33D53, 0x3E500000, 
0x7B03FC9D, 0xB0000000, 
0x80469970, 0x00000000, 
// & (38)
0x05599800, 0x00000000, 
0x047EAAF7, 0x00000000, 
0x04D633F5, 0x00000000, 
0x034E336C, 0x00000000, 
0x03AA35F6, 0x00000000, 
0x03D97F70, 0x00000000, 
0x03AFE500, 0x00000000, 
0x337FD933, 0x65000000, 
0x37E63E43, 0xE4000000, 
0x4E037B6C, 0x00000000, 
0x8904CF60, 0x00000000, 
0xD504CF00, 0x00000000, 
0xFC99BFAB, 0xD0000000, 
0x62374339, 0x70000000, 
// ' (39)
0x49970000, 
0x6FF70000, 
0x9FC00000, 
0xBF600000, 
0xCC000000, 
0x85000000, 
// ( (40)
0x09770000, 
0x064BFB60, 
0x057D7000, 
0x04BB0000, 
0x038B0000, 
0x336D0000, 
0x33E40000, 
0x37B00000, 
0x3D500000, 
0x3F000000, 
0x6C000000, 
0x6C000000, 
0x5F000000, 
0x3E800000, 
0x37F70000, 
0x339FD700, 
0x04760000, 
// ) (41)
0x05485000, 
0x056BFC00, 
0x074DC000, 
0x084F5000, 
0x09B90000, 
0x09990000, 
0x09990000, 
0x09A80000, 
0x09E40000, 
0x086C0000, 
0x08C60000, 
0x078A0000, 
0x067D0000, 
0x058D0000, 
0x036CB000, 
0x4ADD7000, 
0x67400000, 
// * (42)
0x04470000, 
0x04770000, 
0x37439347, 
0x39FDBED7, 
0x337FF700, 
0x7EECDFA0, 
0x74393470, 
0x33770000, 
0x33740000, 
// + (43)
0x05660000, 
0x05D50000, 
0x044E0000, 
0x048A0000, 
0x823EB230, 
0x23BE2380, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33660000, 
// , (44)
0x03860000, 
0x33CFF000, 
0x33FFD000, 
0x336D7000, 
0x335B0000, 
0x4BB00000, 
0x65000000, 
// - (45)
0x82600000, 
0x26800000, 
// . (46)
0x38700000, 
0xCFF00000, 
0xFFC00000, 
0x78000000, 
// / (47)
0x0BB00000, 
0x0AB70000, 
0x09990000, 
0x087B0000, 
0x077B0000, 
0x067B0000, 
0x057B0000, 
0x045C0000, 
0x035C0000, 
0x335C0000, 
0x35C00000, 
0x3C500000, 
0x75000000, 
// 0 (48)
0x05599600, 0x00000000, 
0x047EA99B, 0x50000000, 
0x03BD5038, 0xF0000000, 
0x335E05DE, 0x00000000, 
0x339A04CE, 0xA0000000, 
0x33D403C5, 0xF5000000, 
0x34D03C55, 0xE0000000, 
0x38933C53, 0x8A000000, 
0x3D53C533, 0xD5000000, 
0x4F3B034E, 0x00000000, 
0x8FB049A0, 0x00000000, 
0xBF503BD4, 0x00000000, 
0x3DB99DB0, 0x00000000, 
0x34799700, 0x00000000, 
// 1 (49)
0x07670000, 
0x054BE800, 
0x04BE6E40, 
0x034935D0, 
0x06A80000, 
0x06E40000, 
0x055D0000, 
0x05A80000, 
0x05E40000, 
0x045D0000, 
0x04A80000, 
0x04E40000, 
0x499AF997, 
0x62660000, 
// 2 (50)
0x06682380, 0x00000000, 
0x048FDB99, 0xAF000000, 
0x04606F00, 0x00000000, 
0x0A6D0000, 0x00000000, 
0x095D5000, 0x00000000, 
0x087D5000, 0x00000000, 
0x07BB0000, 0x00000000, 
0x057E7000, 0x00000000, 
0x04CC4000, 0x00000000, 
0x335EA000, 0x00000000, 
0x35E70000, 0x00000000, 
0x3C700000, 0x00000000, 
0x4F267000, 0x00000000, 
0x52760000, 0x00000000, 
// 3 (51)
0x05799500, 0x00000000, 
0x037FB9AF, 0x90000000, 
0x037047C0, 0x00000000, 
0x086C0000, 0x00000000, 
0x08B70000, 0x00000000, 
0x07BD0000, 0x00000000, 
0x33499BEA, 0x00000000, 
0x33623CF6, 0x00000000, 
0x077D0000, 0x00000000, 
0x076C0000, 0x00000000, 
0x07B70000, 0x00000000, 
0x6059B000, 0x00000000, 
0xFEB9AD90, 0x00000000, 
0x48996000, 0x00000000, 
// 4 (52)
0x09570000, 0x00000000, 
0x085F8000, 0x00000000, 
0x075CF400, 0x00000000, 
0x065C6D00, 0x00000000, 
0x055C3A80, 0x00000000, 
0x045C33E4, 0x00000000, 
0x035C335D, 0x00000000, 
0x335C03A8, 0x00000000, 
0x35C04E40, 0x00000000, 
0x4FA23AF9, 0x97000000, 
0x625DC996, 0x00000000, 
0x06E50000, 0x00000000, 
0x055E0000, 0x00000000, 
0x05670000, 0x00000000, 
// 5 (53)
0x04426000, 
0x04AC2500, 
0x04E50000, 
0x035E0000, 
0x03AA0000, 
0x03EB9740, 
0x0323BEC5, 
0x087D0000, 
0x09F00000, 
0x085D0000, 
0x08B70000, 
0x3605BB00, 
0x4FDA9BD7, 
0x33799600, 
// 6 (54)
0x06589900, 0x00000000, 
0x046EB99C, 0x00000000, 
0x038B0000, 0x00000000, 
0x337B0000, 0x00000000, 
0x35E00000, 0x00000000, 
0x3C635995, 0x00000000, 
0x4E5CD99E, 0xB0000000, 
0x9EF704F0, 0x00000000, 
0xCF705F00, 0x00000000, 
0xFE055D00, 0x00000000, 
0xFC05C700, 0x00000000, 
0xDE04B800, 0x00000000, 
0x7FE9BE70, 0x00000000, 
0x34997000, 0x00000000, 
// 7 (55)
0x33427000, 
0x33626C00, 
0x085A0000, 
0x075B0000, 
0x06590000, 
0x05970000, 
0x04B70000, 
0x037A0000, 
0x335C0000, 
0x33D40000, 
0x37B00000, 
0x3D500000, 
0x5E000000, 
0x67000000, 
// 8 (56)
0x06799700, 0x00000000, 
0x049DA9AF, 0xA0000000, 
0x03B8045F, 0x00000000, 
0x335E056C, 0x00000000, 
0x336F05D5, 0x00000000, 
0x03D8336A, 0x40000000, 
0x034FBCB0, 0x00000000, 
0x337E937F, 0x40000000, 
0x37D403AB, 0x00000000, 
0x4E056C00, 0x00000000, 
0x8A05A800, 0x00000000, 
0x9B046C00, 0x00000000, 
0x6FD99DC0, 0x00000000, 
0x34898500, 0x00000000, 
// 9 (57)
0x06599700, 0x00000000, 
0x045CB9BF, 0xB0000000, 
0x035C5037, 0xF4000000, 
0x03E405F6, 0x00000000, 
0x339A057F, 0x60000000, 
0x33C605CF, 0x00000000, 
0x33C804BD, 0xD0000000, 
0x337FB9BE, 0x79800000, 
0x03489733, 0xE0000000, 
0x08990000, 0x00000000, 
0x077D0000, 0x00000000, 
0x068D0000, 0x00000000, 
0x5FB9AE90, 0x00000000, 
0x47997000, 0x00000000, 
// : (58)
0x03870000, 
0x33CFF000, 
0x33FFC000, 
0x33780000, 
0x00000000, 
0x00000000, 
0x38700000, 
0xCFF00000, 
0xFFC00000, 
0x78000000, 
// < (60)
0x09400000, 
0x076BD000, 
0x057DB600, 
0x3349E940, 
0x6BD70000, 
0x7EB40000, 
0x339F9000, 
0x034CE600, 
0x057EA000, 
0x07500000, 
// = (61)
0x36286000, 
0x37284000, 
0x00000000, 
0x62860000, 
0x72840000, 
// > (62)
0x03500000, 
0x03AE7000, 
0x046EC400, 
0x069F9000, 
0x074BE700, 
0x077CB600, 
0x059D9400, 
0x335BD700, 
0x4CB60000, 
0x44000000, 
// ? (63)
0x04579970, 0x00000000, 
0x038CB99A, 0xC0000000, 
0x09F00000, 0x00000000, 
0x086C0000, 0x00000000, 
0x077E5000, 0x00000000, 
0x06AC0000, 0x00000000, 
0x045C8000, 0x00000000, 
0x037C4000, 0x00000000, 
0x339B0000, 0x00000000, 
0x34E00000, 0x00000000, 
0x35700000, 0x00000000, 
0x00000000, 0x00000000, 
0x37000000, 0x00000000, 
0x45000000, 0x00000000, 
// @ (64)
0x06523800, 0x00000000, 
0x057EA9AF, 0x70000000, 
0x04BD503B, 0xF0000000, 
0x334DB624, 0x8F000000, 
0x33897EA9, 0x9D8A0000, 
0x33D4D603, 0xE4D00000, 
0x34D4E035, 0xD4E00000, 
0x38A8A03A, 0xA9A00000, 
0x3E5BF998, 0xDFD40000, 
0x5F332365, 0x80000000, 
0x7F700000, 0x00000000, 
0x3BF03500, 0x00000000, 
0x33FC9BC0, 0x00000000, 
0x33699700, 0x00000000, 
// A (65)
0x08570000, 
0x08DC0000, 
0x078DC000, 
0x064D7C00, 
0x06C66C00, 
0x057B36C0, 
0x05E336C0, 
0x04B7336C, 
0x037B036B, 
0x03EB23C9, 
0x33AB24C9, 
0x36C05990, 
0x3D505990, 
0x57066600, 
// B (66)
0x04426800, 0x00000000, 
0x04AB24AF, 0x00000000, 
0x04E405F0, 0x00000000, 
0x035D055E, 0x00000000, 
0x03A805AA, 0x00000000, 
0x03E405E5, 0x00000000, 
0x335F25D9, 0x00000000, 
0x33AB24AF, 0x70000000, 
0x33E405F5, 0x00000000, 
0x35D055D0, 0x00000000, 
0x3A805A80, 0x00000000, 
0x3E405E50, 0x00000000, 
0x5F25DB00, 0x00000000, 
0x62670000, 0x00000000, 
// C (67)
0x05524600, 0x00000000, 
0x047EA99A, 0x90000000, 
0x03BD5000, 0x00000000, 
0x335E0000, 0x00000000, 
0x339A0000, 0x00000000, 
0x33D50000, 0x00000000, 
0x34E00000, 0x00000000, 
0x38A00000, 0x00000000, 
0x3D500000, 0x00000000, 
0x4E000000, 0x00000000, 
0x8A000000, 0x00000000, 
0xBF000000, 0x00000000, 
0x3FC23B00, 0x00000000, 
0x36245000, 0x00000000, 
// D (68)
0x04425800, 0x00000000, 
0x04AB23AF, 0x70000000, 
0x04E404BF, 0x00000000, 
0x035D055D, 0x00000000, 
0x03A80588, 0x00000000, 
0x03E405D4, 0x00000000, 
0x335D054D, 0x00000000, 
0x33A8058A, 0x00000000, 
0x33E405D5, 0x00000000, 
0x35D054E0, 0x00000000, 
0x3A8059A0, 0x00000000, 
0x3E404BD4, 0x00000000, 
0x5F24DB00, 0x00000000, 
0x62570000, 0x00000000, 
// E (69)
0x04426000, 
0x04AB2500, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335F2570, 
0x33AB2560, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5F257000, 
0x62660000, 
// F (70)
0x04426000, 
0x04AB2500, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335F2570, 
0x33AC2560, 
0x33E50000, 
0x35E00000, 
0x3AA00000, 
0x3E500000, 
0x5E000000, 
0x67000000, 
// G (71)
0x05524700, 
0x047EA23C, 
0x03BD5000, 
0x335E0000, 
0x339A0000, 
0x33D40000, 
0x34D00000, 
0x38804696, 
0x3D4047F5, 
0x4E054E00, 
0x8A059A00, 
0xBF05E500, 
0x3FC23AE0, 
0x36247500, 
// H (72)
0x04470540, 
0x04A805A0, 
0x04E405E0, 
0x035D055D, 
0x03A805A8, 
0x03E405E4, 
0x335F25AD, 
0x33AC25D8, 
0x33E505E4, 
0x35E055D0, 
0x3AA05A80, 
0x3E505E40, 
0x5E055D00, 
0x67056600, 
// I (73)
0x04426700, 0x00000000, 
0x04699DB9, 0x96000000, 
0x07E40000, 0x00000000, 
0x065D0000, 0x00000000, 
0x06A80000, 0x00000000, 
0x06E40000, 0x00000000, 
0x055D0000, 0x00000000, 
0x05A80000, 0x00000000, 
0x05E40000, 0x00000000, 
0x045D0000, 0x00000000, 
0x04A80000, 0x00000000, 
0x04E40000, 0x00000000, 
0x499AF997, 0x00000000, 
0x62660000, 0x00000000, 
// J (74)
0x06424000, 
0x06623D00, 
0x0AE00000, 
0x095D0000, 
0x09A80000, 
0x09E40000, 
0x085D0000, 
0x08A80000, 
0x08E40000, 
0x075E0000, 
0x07AA0000, 
0x3605E500, 
0x5FEA99DB, 
0x36823700, 
// K (75)
0x04470580, 
0x04A804B5, 
0x04E403B5, 
0x035D335C, 
0x03A836B0, 
0x03E47B00, 
0x335D9800, 
0x33AAE000, 
0x33E57B00, 
0x35E33B70, 
0x3AA03E00, 
0x3E5037B0, 
0x5E05B700, 
0x67068000, 
// L (76)
0x04470000, 
0x04A80000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5F257000, 
0x62660000, 
// M (77)
0x04490467, 0x00000000, 
0x04AF04F8, 0x00000000, 
0x04EF03AF, 0x40000000, 
0x035EF336, 0xED000000, 
0x03AAD33C, 0x98000000, 
0x03E8C399, 0xD4000000, 
0x335E6C4D, 0x4D000000, 
0x33AA6CC7, 0x88000000, 
0x33E56FC3, 0xD4000000, 
0x35E38F64, 0xD0000000, 
0x3AA39B38, 0x80000000, 
0x3E53643D, 0x40000000, 
0x5E044D00, 0x00000000, 
0x67045600, 0x00000000, 
// N (78)
0x04480540, 0x00000000, 
0x04AF05A0, 0x00000000, 
0x04EF504E, 0x00000000, 
0x035DB703, 0x5D000000, 
0x03AA9903, 0x98000000, 
0x03E56B03, 0xD4000000, 
0x335E33C3, 0x34D00000, 
0x33AA33D3, 0x38800000, 
0x33E533B4, 0x3D400000, 
0x35E037B4, 0xD0000000, 
0x3AA035FE, 0x80000000, 
0x3E504FF4, 0x00000000, 
0x5E05CD00, 0x00000000, 
0x67056600, 0x00000000, 
// O (79)
0x05523800, 0x00000000, 
0x047EA9AF, 0x70000000, 
0x03BD503B, 0xF0000000, 
0x335E055D, 0x00000000, 
0x339A0589, 0x00000000, 
0x33D505D5, 0x00000000, 
0x34E054E0, 0x00000000, 
0x38A058A0, 0x00000000, 
0x3D505D50, 0x00000000, 
0x4E054E00, 0x00000000, 
0x8A059A00, 0x00000000, 
0xBF04BD40, 0x00000000, 
0x3FC99DB0, 0x00000000, 
0x36237000, 0x00000000, 
// P (80)
0x04426800, 0x00000000, 
0x04AC24AF, 0x00000000, 
0x04E505F0, 0x00000000, 
0x035E055D, 0x00000000, 
0x03AA05A8, 0x00000000, 
0x03E505E5, 0x00000000, 
0x335E045D, 0xB0000000, 
0x33AD23AE, 0x70000000, 
0x33EA2450, 0x00000000, 
0x35E00000, 0x00000000, 
0x3AA00000, 0x00000000, 
0x3E500000, 0x00000000, 
0x5E000000, 0x00000000, 
0x67000000, 0x00000000, 
// Q (81)
0x05523800, 0x00000000, 
0x047EA9AF, 0x70000000, 
0x03BD503B, 0xF0000000, 
0x335E055D, 0x00000000, 
0x339A0588, 0x00000000, 
0x33D505D4, 0x00000000, 
0x34E054D0, 0x00000000, 
0x38A058A0, 0x00000000, 
0x3D505D50, 0x00000000, 
0x4E054E00, 0x00000000, 
0x8A059A00, 0x00000000, 
0xBF04BD40, 0x00000000, 
0x3FC99DB0, 0x00000000, 
0x369EC700, 0x00000000, 
0x03F70000, 0x00000000, 
0x334FD997, 0x00000000, 
0x03523600, 0x00000000, 
// R (82)
0x04425800, 0x00000000, 
0x04AB23AF, 0x70000000, 
0x04E404F5, 0x00000000, 
0x035D045E, 0x00000000, 
0x03A804AA, 0x00000000, 
0x03E403BD, 0x00000000, 
0x335F23D9, 0x00000000, 
0x33AC99B7, 0x00000000, 
0x33E5334A, 0x00000000, 
0x35E04C00, 0x00000000, 
0x3AA04B40, 0x00000000, 
0x3E504880, 0x00000000, 
0x5E056B00, 0x00000000, 
0x67069000, 0x00000000, 
// S (83)
0x07723700, 
0x057DA236, 
0x047B0000, 
0x034E0000, 
0x039B0000, 
0x039E0000, 
0x034ED600, 
0x057EB000, 
0x079B0000, 
0x08AB0000, 
0x08A80000, 
0x076C0000, 
0x425DB000, 
0x62474000, 
// T (84)
0x42800000, 
0x623DC230, 
0x04E50000, 
0x035E0000, 
0x03AA0000, 
0x03E50000, 
0x335E0000, 
0x33AA0000, 
0x33E50000, 
0x35E00000, 
0x3AA00000, 
0x3E500000, 
0x5E000000, 
0x67000000, 
// U (85)
0x03470540, 
0x03A805A0, 
0x03E405E0, 
0x335D055D, 
0x33A90599, 
0x33E505D5, 
0x35E054E0, 
0x3AA058A0, 
0x3D505D50, 
0x4E054E00, 
0x8905AA00, 
0xD505E500, 
0xFC24DB00, 
0x62570000, 
// V (86)
0x47000000, 
0x6C06B000, 
0x6C057900, 
0x6C05D000, 
0x6C04B600, 
0x6C037B00, 
0x6C03E000, 
0x6C33C700, 
0x8C38B000, 
0x9C4E4000, 
0x9BB80000, 
0x9DC00000, 
0x9F400000, 
0x67000000, 
// W (87)
0x33470363, 0x34000000, 
0x337B3379, 0x33B00000, 
0x339933E6, 0x34E00000, 
0x33C738F4, 0x3B700000, 
0x33F53DF3, 0x4E000000, 
0x34F39BC3, 0xB8000000, 
0x36D3D7B3, 0xE0000000, 
0x39B97240, 0x00000000, 
0x3BAE3C7E, 0x00000000, 
0x3EE93DA9, 0x00000000, 
0x3FF33FF0, 0x00000000, 
0x6F935FB0, 0x00000000, 
0x8F337F40, 0x00000000, 
0x67336700, 0x00000000, 
// X (88)
0x05740480, 
0x05A703C5, 
0x059933B7, 
0x056B3A80, 
0x055C7B00, 
0x06FC0000, 
0x054F0000, 
0x05CE6000, 
0x04C5B700, 
0x03B73990, 
0x338A337A, 
0x37B036C0, 
0x5C044E00, 
0x80690000, 
// Y (89)
0x75000000, 
0x7B000000, 
0x3F05B000, 
0x3B603B70, 
0x38A33B70, 
0x34E3B700, 
0x33DC7000, 
0x33A90000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5D000000, 
0x66000000, 
// Z (90)
0x04427000, 
0x04626D00, 
0x0A7B0000, 
0x097B0000, 
0x087B0000, 
0x077B0000, 
0x067B0000, 
0x057B0000, 
0x045B0000, 
0x035A0000, 
0x335C0000, 
0x35C00000, 
0x4FA25700, 
0x62760000, 
// [ (91)
0x05425700, 
0x05AB2460, 
0x05E40000, 
0x045D0000, 
0x04A80000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5F247000, 
0x62560000, 
// \ (92)
0x75000000, 
0x8A000000, 
0x5C000000, 
0x3F000000, 
0x3C600000, 
0x39800000, 
0x37B00000, 
0x33F00000, 
0x33E40000, 
0x33B70000, 
0x337A0000, 
0x335D0000, 
0x03F00000, 
0x03740000, 
// ] (93)
0x05425700, 
0x05624D80, 
0x0AE40000, 
0x095D0000, 
0x09A80000, 
0x09E40000, 
0x085D0000, 
0x08A80000, 
0x08E40000, 
0x075D0000, 
0x07A80000, 
0x07E40000, 
0x065D0000, 
0x06A80000, 
0x06E40000, 
0x424AD000, 
0x62560000, 
// ^ (94)
0x06400000, 
0x06F00000, 
0x05CE5000, 
0x04B7B700, 
0x03993990, 
0x337B337B, 
0x35C035D0, 
0x3E05F000, 
0x75058000, 
// _ (95)
0x62860000, 
0x72840000, 
// ` (96)
0x68000000, 
0x3F400000, 
0x37800000, 
0x33700000, 
// a (97)
0x04724600, 0x00000000, 
0x03BD99AF, 0x50000000, 
0x34DB034E, 0x00000000, 
0x3A9048A0, 0x00000000, 
0x3E504D50, 0x00000000, 
0x5E044E00, 0x00000000, 
0xA9049A00, 0x00000000, 
0xD503BF50, 0x00000000, 
0xFC99DCE0, 0x00000000, 
0x62375700, 0x00000000, 
// b (98)
0x04470000, 0x00000000, 
0x04A80000, 0x00000000, 
0x04E40000, 0x00000000, 
0x035D0000, 0x00000000, 
0x03A87236, 0x00000000, 
0x03ECD99C, 0xF0000000, 
0x335FB035, 0xD0000000, 
0x33A9049A, 0x00000000, 
0x33E404E5, 0x00000000, 
0x35D045E0, 0x00000000, 
0x3A8049A0, 0x00000000, 
0x3E403BD4, 0x00000000, 
0x5FA99DB0, 0x00000000, 
0x62470000, 0x00000000, 
// c (99)
0x04799800, 
0x03BD99BE, 
0x34DB0000, 
0x3A900000, 
0x3E500000, 
0x5E000000, 
0xAA000000, 
0xBF000000, 
0x3FC99BD0, 
0x36237500, 
// d (100)
0x09400000, 0x00000000, 
0x09A00000, 0x00000000, 
0x09E00000, 0x00000000, 
0x085D0000, 0x00000000, 
0x04723C80, 0x00000000, 
0x03BD99AF, 0x40000000, 
0x34DB034D, 0x00000000, 
0x3A904880, 0x00000000, 
0x3E504D40, 0x00000000, 
0x5E044D00, 0x00000000, 
0xA9049800, 0x00000000, 
0xD503BF40, 0x00000000, 
0xFC99DDD0, 0x00000000, 
0x62376600, 0x00000000, 
// e (101)
0x04723600, 
0x03AE99CF, 
0x34DB035D, 
0x38B04880, 
0x3DB24E40, 
0x4E258000, 
0x8A000000, 
0xBF045000, 
0x3FC9ADC0, 
0x36998500, 
// f (102)
0x07700000, 
0x057DA000, 
0x047B0000, 
0x04E00000, 
0x038A0000, 
0x899EB230, 
0x99BE2380, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5D000000, 
0x66000000, 
// g (103)
0x06724000, 0x00000000, 
0x05BD99AF, 0x00000000, 
0x034DB034, 0xE0000000, 
0x03A9048A, 0x00000000, 
0x03E504D5, 0x00000000, 
0x335E044E, 0x00000000, 
0x33A9049A, 0x00000000, 
0x33D503BF, 0x50000000, 
0x33FC99DC, 0xE0000000, 
0x33623798, 0x00000000, 
0x3604BD00, 0x00000000, 
0x5FD99DA0, 0x00000000, 
0x34899700, 0x00000000, 
// h (104)
0x04470000, 0x00000000, 
0x04A80000, 0x00000000, 
0x04E40000, 0x00000000, 
0x035D0000, 0x00000000, 
0x03A87236, 0x00000000, 
0x03ECD99C, 0xF0000000, 
0x335FB035, 0xD0000000, 
0x33AA0488, 0x00000000, 
0x33E504E4, 0x00000000, 
0x35E045D0, 0x00000000, 
0x3AA04A80, 0x00000000, 
0x3E504E40, 0x00000000, 
0x5E045D00, 0x00000000, 
0x67046600, 0x00000000, 
// i (105)
0x05700000, 
0x04450000, 
0x00000000, 
0x00000000, 
0x36995000, 
0x379F4000, 
0x334D0000, 
0x33880000, 
0x33D40000, 
0x34D00000, 
0x38800000, 
0x3D400000, 
0x4F970000, 
0x59960000, 
// j (106)
0x0B700000, 
0x0A450000, 
0x00000000, 
0x00000000, 
0x05624500, 
0x05723F40, 
0x084D0000, 
0x08880000, 
0x08D40000, 
0x074D0000, 
0x07880000, 
0x07D40000, 
0x064D0000, 
0x06980000, 
0x05BD4000, 
0x5C99DB00, 
0x42370000, 
// k (107)
0x04470000, 
0x04A80000, 
0x04E40000, 
0x035D0000, 
0x03A90357, 
0x03E5339A, 
0x335E34B6, 
0x33AA8B00, 
0x33EC8000, 
0x35E6C000, 
0x3AA3B600, 
0x3E534E00, 
0x5E039800, 
0x67048000, 
// l (108)
0x33499700, 
0x3369D800, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5F970000, 
0x69960000, 
// m (109)
0x03763803, 0x80000000, 
0x03E8BBD7, 0xBAB00000, 
0x335F736F, 0x735C0000, 
0x33AA33AB, 0x33AA0000, 
0x33E533E5, 0x33E50000, 
0x35E335E3, 0x35E00000, 
0x3AA33AA3, 0x3AA00000, 
0x3E533E53, 0x3E500000, 
0x5E335E33, 0x5E000000, 
0x67336733, 0x67000000, 
// n (110)
0x03757236, 0x00000000, 
0x03ECD99C, 0xF0000000, 
0x335FB035, 0xD0000000, 
0x33AA0488, 0x00000000, 
0x33E504E4, 0x00000000, 
0x35E045D0, 0x00000000, 
0x3AA04A80, 0x00000000, 
0x3E504E40, 0x00000000, 
0x5E045D00, 0x00000000, 
0x67046600, 0x00000000, 
// o (111)
0x04799600, 
0x03BD9CF0, 
0x34DB03FB, 
0x3A904AA0, 
0x3E404E50, 
0x5E044E00, 
0xAA049A00, 
0xBF03BD40, 
0x3FC9DB00, 
0x36997000, 
// p (112)
0x04767236, 0x00000000, 
0x04EDD99C, 0xF0000000, 
0x035FB035, 0xD0000000, 
0x03A9049A, 0x00000000, 
0x03E404E5, 0x00000000, 
0x335D045E, 0x00000000, 
0x33A8049A, 0x00000000, 
0x33E403BD, 0x40000000, 
0x35FA99DB, 0x00000000, 
0x3AC23700, 0x00000000, 
0x3E500000, 0x00000000, 
0x5E000000, 0x00000000, 
0x67000000, 0x00000000, 
// q (113)
0x04724600, 0x00000000, 
0x03BD99AF, 0x50000000, 
0x34DB034E, 0x00000000, 
0x3A9048A0, 0x00000000, 
0x3E504D50, 0x00000000, 
0x5E044E00, 0x00000000, 
0xA9049A00, 0x00000000, 
0xD503BF50, 0x00000000, 
0xFC99DDE0, 0x00000000, 
0x6237AA00, 0x00000000, 
0x05E50000, 0x00000000, 
0x045E0000, 0x00000000, 
0x04670000, 0x00000000, 
// r (114)
0x03767246, 0x00000000, 
0x03EDD23F, 0x50000000, 
0x335FB035, 0xE0000000, 
0x33A90467, 0x00000000, 
0x33E40000, 0x00000000, 
0x35D00000, 0x00000000, 
0x3A800000, 0x00000000, 
0x3E400000, 0x00000000, 
0x5D000000, 0x00000000, 
0x66000000, 0x00000000, 
// s (115)
0x03799840, 
0x33BD99BF, 
0x34E04600, 
0x36F40000, 
0x34AEA700, 
0x0349F700, 
0x054E0000, 
0x505E5000, 
0xFEB99DB0, 
0x37237000, 
// t (116)
0x03800000, 
0x334E0000, 
0x338A0000, 
0x89EB2300, 
0x9BE23800, 
0x3A800000, 
0x3D500000, 
0x4E000000, 
0x8A000000, 
0xD5000000, 
0xFC9BC000, 
0x69970000, 
// u (117)
0x33660476, 
0x33D504E5, 
0x34E045E0, 
0x38A04AA0, 
0x3D504E50, 
0x4E045E00, 
0x8804AA00, 
0xD503BF50, 
0xFC99DCE0, 
0x62375700, 
// v (118)
0x94065000, 
0xD606D000, 
0xB905C700, 
0x9B04B900, 
0x7C037B00, 
0x5F336D00, 
0x3F44E000, 
0x3E7C5000, 
0x3CF70000, 
0x36700000, 
// w (119)
0x39433763, 0x36700000, 
0x3F334F83, 0x3E500000, 
0x3F33BF63, 0x9A000000, 
0x3F36CD64, 0xF0000000, 
0x6F3C6F3C, 0x80000000, 
0x6F7B3F7D, 0x00000000, 
0x6EE46EE6, 0x00000000, 
0x7FB36FB0, 0x00000000, 
0x9F338F40, 0x00000000, 
0x67336700, 0x00000000, 
// x (120)
0x03750480, 
0x036D03B7, 
0x04C73B70, 
0x046EB700, 
0x05E90000, 
0x04CAB000, 
0x03C53F00, 
0x33C533B7, 
0x3C5037B0, 
0x74058000, 
// y (121)
0x04906800, 
0x04D604C6, 
0x04C7038A, 
0x0499335D, 
0x048B33C5, 
0x046C3B70, 
0x044F7B00, 
0x05FD0000, 
0x05E50000, 
0x04980000, 
0x038B0000, 
0x99DA0000, 
0x98400000, 
// z (122)
0x03627600, 
0x03725AF4, 
0x086B0000, 
0x07990000, 
0x06B60000, 
0x046B0000, 
0x03990000, 
0x33B60000, 
0x4FA25700, 
0x62760000, 
// { (123)
0x05489600, 
0x045C9950, 
0x04B00000, 
0x03590000, 
0x03A70000, 
0x03E50000, 
0x336C0000, 
0x6AC00000, 
0x7DB00000, 
0x36C00000, 
0x3A800000, 
0x3E400000, 
0x4B000000, 
0x84000000, 
0xA0000000, 
0xCA960000, 
0x49950000, 
// | (124)
0x04470000, 
0x04A80000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5D000000, 
0x66000000, 
// } (125)
0x06996000, 
0x05499E50, 
0x08940000, 
0x08C00000, 
0x077A0000, 
0x07D50000, 
0x07F00000, 
0x07FA6000, 
0x067D9400, 
0x054E0000, 
0x05890000, 
0x05C50000, 
0x05E00000, 
0x04590000, 
0x04940000, 
0x399B7000, 
0x49950000, 
// ~ (126)
0x33596037, 0x50000000, 
0x37FEF933, 0xF4000000, 
0x4E33AFEE, 0x70000000, 
0x57036940, 0x00000000, 
// � (161)
0x04540000, 
0x04700000, 
0x00000000, 
0x03760000, 
0x03E50000, 
0x335E0000, 
0x33AA0000, 
0x33E50000, 
0x35E00000, 
0x3AA00000, 
0x3E500000, 
0x5E000000, 
0x67000000, 
// � (163)
0x07523800, 0x00000000, 
0x067EA9BE, 0x00000000, 
0x06D60000, 0x00000000, 
0x054E0000, 0x00000000, 
0x05AA0000, 0x00000000, 
0x05E50000, 0x00000000, 
0x3349AF99, 0x70000000, 
0x3369DC99, 0x60000000, 
0x04E40000, 0x00000000, 
0x035D0000, 0x00000000, 
0x03B70000, 0x00000000, 
0x337D0000, 0x00000000, 
0x4BFB2470, 0x00000000, 
0x62760000, 0x00000000, 
// � (164)
0x03D43896, 0x38900000, 
0x037DDAAF, 0xB8000000, 
0x039B037D, 0x00000000, 
0x334E046C, 0x00000000, 
0x338A04A8, 0x00000000, 
0x339B037D, 0x00000000, 
0x35CDB9DD, 0xB0000000, 
0x4B338943, 0xB6000000, 
// | (166)
0x04470000, 
0x04A80000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x33A80000, 
0x33E40000, 
0x35D00000, 
0x3A800000, 
0x3E400000, 
0x5D000000, 
0x66000000, 
// � (167)
0x06525700, 0x00000000, 
0x057EA246, 0x00000000, 
0x05D60000, 0x00000000, 
0x044F0000, 0x00000000, 
0x045F7000, 0x00000000, 
0x044FD800, 0x00000000, 
0x04A33CB0, 0x00000000, 
0x037A0388, 0x40000000, 
0x03D605B0, 0x00000000, 
0x03F8044D, 0x00000000, 
0x037F903B, 0x70000000, 
0x044BE66D, 0x00000000, 
0x064BF600, 0x00000000, 
0x08B70000, 0x00000000, 
0x08E50000, 0x00000000, 
0x426DB000, 0x00000000, 
0x62670000, 0x00000000, 
// � (168)
0x80480000, 
0x80480000, 
// � (169)
0x06799400, 0x00000000, 
0x048EB9BF, 0x70000000, 
0x03BB4798, 0x8E000000, 
0x338B5DA9, 0xB3F00000, 
0x35E5D404, 0xF0000000, 
0x3C7C605F, 0x00000000, 
0x4E4E055D, 0x00000000, 
0x8A8B05A8, 0x00000000, 
0xC78C05E0, 0x00000000, 
0xC63FB9A3, 0xA9000000, 
0xC7349967, 0xD0000000, 
0xAC049D00, 0x00000000, 
0x4FE9AEB0, 0x00000000, 
0x33798400, 0x00000000, 
// � (170)
0x33525700, 
0x37EA23C8, 
0x3D504D50, 
0x5D045E00, 
0xA9037FA0, 
0xBF99BDF5, 
0x32383800, 
// � (172)
0x82800000, 0x00000000, 
0xCCCCCCCD, 0xE0000000, 
0x07A90000, 0x00000000, 
0x07E40000, 0x00000000, 
0x07800000, 0x00000000, 
// � (173)
0x82800000, 
0x28800000, 
// � (174)
0x06799400, 0x00000000, 
0x048EB9BF, 0x70000000, 
0x03BB9983, 0x8E000000, 
0x338B4E9E, 0x93F00000, 
0x35E3883A, 0x83F00000, 
0x3C73D43E, 0x33F00000, 
0x4E34F9D5, 0x35D00000, 
0x8A38CC83, 0x3A800000, 
0xC73D55D3, 0x3E000000, 
0xC64E33E4, 0xA9000000, 
0xC757337A, 0xD0000000, 
0xAC049D00, 0x00000000, 
0x4FE9AEB0, 0x00000000, 
0x33798400, 0x00000000, 
// � (175)
0x82400000, 
0x24800000, 
// x (176)
0x03750480, 
0x036D03B7, 
0x04C73B70, 
0x046EB700, 
0x05E90000, 
0x04CAB000, 
0x03C53F00, 
0x33C533B7, 
0x3C5037B0, 
0x74058000, 
// 2 (178)
0x06682380, 0x00000000, 
0x048FDB99, 0xAF000000, 
0x04606F00, 0x00000000, 
0x0A6D0000, 0x00000000, 
0x095D5000, 0x00000000, 
0x087D5000, 0x00000000, 
0x07BB0000, 0x00000000, 
0x057E7000, 0x00000000, 
0x04CC4000, 0x00000000, 
0x335EA000, 0x00000000, 
0x35E70000, 0x00000000, 
0x3C700000, 0x00000000, 
0x4F267000, 0x00000000, 
0x52760000, 0x00000000, 
// 3 (179)
0x05799500, 0x00000000, 
0x037FB9AF, 0x90000000, 
0x037047C0, 0x00000000, 
0x086C0000, 0x00000000, 
0x08B70000, 0x00000000, 
0x07BD0000, 0x00000000, 
0x33499BEA, 0x00000000, 
0x33623CF6, 0x00000000, 
0x077D0000, 0x00000000, 
0x076C0000, 0x00000000, 
0x07B70000, 0x00000000, 
0x6059B000, 0x00000000, 
0xFEB9AD90, 0x00000000, 
0x48996000, 0x00000000, 
// � (180)
0x33580000, 
0x35C50000, 
0x5A000000, 
0x60000000, 
// � (181)
0x04760576, 0x00000000, 
0x04E505E5, 0x00000000, 
0x035E055E, 0x00000000, 
0x03AA05AA, 0x00000000, 
0x03E505E5, 0x00000000, 
0x335E055E, 0x00000000, 
0x33AA059A, 0x00000000, 
0x33EF04BF, 0x50000000, 
0x35EFC99D, 0xCE000000, 
0x3AA62375, 0x70000000, 
0x3E500000, 0x00000000, 
0x5E000000, 0x00000000, 
0x67000000, 0x00000000, 
// 1 (182)
0x07670000, 
0x054BE800, 
0x04BE6E40, 
0x034935D0, 
0x06A80000, 
0x06E40000, 
0x055D0000, 
0x05A80000, 
0x05E40000, 
0x045D0000, 
0x04A80000, 
0x04E40000, 
0x499AF997, 
0x62660000, 
// 2 (183)
0x06682380, 0x00000000, 
0x048FDB99, 0xAF000000, 
0x04606F00, 0x00000000, 
0x0A6D0000, 0x00000000, 
0x095D5000, 0x00000000, 
0x087D5000, 0x00000000, 
0x07BB0000, 0x00000000, 
0x057E7000, 0x00000000, 
0x04CC4000, 0x00000000, 
0x335EA000, 0x00000000, 
0x35E70000, 0x00000000, 
0x3C700000, 0x00000000, 
0x4F267000, 0x00000000, 
0x52760000, 0x00000000, 
// 3 (184)
0x05799500, 0x00000000, 
0x037FB9AF, 0x90000000, 
0x037047C0, 0x00000000, 
0x086C0000, 0x00000000, 
0x08B70000, 0x00000000, 
0x07BD0000, 0x00000000, 
0x33499BEA, 0x00000000, 
0x33623CF6, 0x00000000, 
0x077D0000, 0x00000000, 
0x076C0000, 0x00000000, 
0x07B70000, 0x00000000, 
0x6059B000, 0x00000000, 
0xFEB9AD90, 0x00000000, 
0x48996000, 0x00000000, 
// 1 (185)
0x07670000, 
0x054BE800, 
0x04BE6E40, 
0x034935D0, 
0x06A80000, 
0x06E40000, 
0x055D0000, 
0x05A80000, 
0x05E40000, 
0x045D0000, 
0x04A80000, 
0x04E40000, 
0x499AF997, 
0x62660000, 
// � (191)
0x08540000, 
0x08700000, 
0x00000000, 
0x07750000, 
0x07E40000, 
0x06C70000, 
0x047C5000, 
0x335B8000, 
0x38D40000, 
0x7B000000, 
0xC7000000, 
0x9E23CD00, 
0x35998600, 
// � (198)
0x0A790000, 0x00000000, 
0x097B9000, 0x00000000, 
0x085F4000, 0x00000000, 
0x08ED0000, 0x00000000, 
0x07CB8000, 0x00000000, 
0x06A8D400, 0x00000000, 
0x057B4F99, 0x70000000, 
0x045C3AB9, 0x96000000, 
0x04D33E40, 0x00000000, 
0x03CB9AD0, 0x00000000, 
0x33BA99D8, 0x00000000, 
0x37B03E40, 0x00000000, 
0x5D035F99, 0x70000000, 
0x80462360, 0x00000000, 
// x (215)
0x03750480, 
0x036D03B7, 
0x04C73B70, 
0x046EB700, 
0x05E90000, 
0x04CAB000, 
0x03C53F00, 
0x33C533B7, 
0x3C5037B0, 
0x74058000, 
// � (222)
0x07599500, 
0x067ED900, 
0x06D67000, 
0x054E0000, 
0x05890000, 
0x05D40000, 
0x044D0000, 
0x04880000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x549A0000, 
0xAFD40000, 
0x38000000, 
// � (223)
0x07599500, 
0x067ED900, 
0x06D67000, 
0x054E0000, 
0x05890000, 
0x05D40000, 
0x044D0000, 
0x04880000, 
0x04E40000, 
0x035D0000, 
0x03A80000, 
0x03E40000, 
0x335D0000, 
0x549A0000, 
0xAFD40000, 
0x38000000, 
// � (240)
0x05423400, 0x00000000, 
0x05699ED0, 0x00000000, 
0x086FC000, 0x00000000, 
0x09AFA000, 0x00000000, 
0x057238F8, 0x00000000, 
0x04BD99BF, 0xF4000000, 
0x335DB04A, 0xD0000000, 
0x37E705A8, 0x00000000, 
0x3D506E40, 0x00000000, 
0x4D066D00, 0x00000000, 
0x88057E70, 0x00000000, 
0xD504BD50, 0x00000000, 
0xFC23DB00, 0x00000000, 
0x62470000, 0x00000000, 
// � (248)
0x04799838, 0x00000000, 
0x03BD99DF, 0x70000000, 
0x34DB03AF, 0x60000000, 
0x3A903C5C, 0x60000000, 
0x3E433C53, 0xE4000000, 
0x4E35C334, 0xE0000000, 
0x6C5C039A, 0x00000000, 
0x6EA03BD4, 0x00000000, 
0x7FD99DB0, 0x00000000, 
0x73899700, 0x00000000, 
// ? (254)
0x04579970, 0x00000000, 
0x038CB99A, 0xC0000000, 
0x09F00000, 0x00000000, 
0x086C0000, 0x00000000, 
0x077E5000, 0x00000000, 
0x06AC0000, 0x00000000, 
0x045C8000, 0x00000000, 
0x037C4000, 0x00000000, 
0x339B0000, 0x00000000, 
0x34E00000, 0x00000000, 
0x35700000, 0x00000000, 
0x00000000, 0x00000000, 
0x37000000, 0x00000000, 
0x45000000, 0x00000000, 
0 };

const monospaced_font monoItalic16 = {
fontTypes::monospaced, 10, /* advance width */ {
	0x0001, /* version */
	fontTypes::monospaced, /* family */
	fontTypes::italic, /* style */
	fontTypes::size2, /* size (16) */
	16, /* max ascent */
	4, /* max descent */
	125, /* glyph count */
	iso8859_1_glyph[0], /* glyphs */
{ // offsets
	0x0000, 0x000A, 0x0026, 0x0029, 0x0045, 0x0049, 0x0057, 0x0073, 
	0x008F, 0x00AB, 0x00B1, 0x00C2, 0x00D3, 0x00DC, 0x00E6, 0x00ED, 
	0x00EF, 0x00F3, 0x0100, 0x011C, 0x012A, 0x0146, 0x0162, 0x017E, 
	0x018C, 0x01A8, 0x01B6, 0x01D2, 0x01EE, 0x01F8, 0x0202, 0x0207, 
	0x0211, 0x022D, 0x0249, 0x0257, 0x0273, 0x028F, 0x02AB, 0x02B9, 
	0x02C7, 0x02D5, 0x02E3, 0x02FF, 0x030D, 0x031B, 0x0329, 0x0345, 
	0x0361, 0x037D, 0x0399, 0x03BB, 0x03D7, 0x03E5, 0x03F3, 0x0401, 
	0x040F, 0x042B, 0x0439, 0x0447, 0x0455, 0x0466, 0x0474, 0x0485, 
	0x048E, 0x0490, 0x0494, 0x04A8, 0x04C4, 0x04CE, 0x04EA, 0x04F4, 
	0x0502, 0x051C, 0x0538, 0x0546, 0x0557, 0x0565, 0x0573, 0x0587, 
	0x059B, 0x05A5, 0x05BF, 0x05D9, 0x05ED, 0x05F7, 0x0603, 0x060D, 
	0x0617, 0x062B, 0x0635, 0x0642, 0x064C, 0x065D, 0x066B, 0x067C, 
	0x0684, 0x0691, 0x06AD, 0x06BD, 0x06CB, 0x06ED, 0x06EF, 0x070B, 
	0x0712, 0x071C, 0x071E, 0x073A, 0x073C, 0x0746, 0x0762, 0x077E, 
	0x0782, 0x079C, 0x07AA, 0x07C6, 0x07E2, 0x07F0, 0x07FD, 0x0819, 
	0x0823, 0x0833, 0x0843, 0x085F, 0x0873, 0x088F
}, { // kern
	-3, -1, -7, -1, -6, -4, -1, -1, -1, -8, -2, 0, -4, -4, -2, -4, 
	-5, -1, -2, -1, -1, -3, -1, -2, -3, -3, -2, -1, -5, -3, -1, -1, 
	-3, -1, 0, -1, -2, -1, -2, -2, -2, -1, -1, -2, -1, -2, -1, -1, 
	-2, -1, -2, -1, -1, -4, -2, -4, -2, -1, -5, -1, -1, -6, 1, -3, 
	0, -8, -2, -1, -2, -3, -2, -5, -2, -1, -4, -1, -2, -3, 0, -1, 
	-2, 0, -3, -2, -3, -6, -3, -4, -2, -1, -1, -1, -4, -4, -1, -2, 
	-4, -1, -1, -4, 0, -6, -2, -4, -3, -3, -2, -7, -1, -1, -3, -8, 
	0, -1, -1, -3, -1, -1, -1, -1, -2, -2, -1, -3, -3  
}, { // ascent
	9, 13, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 9, 3, 5, 
	3, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 9, 9, 6, 9, 
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
	0, 13, 9, 13, 9, 13, 9, 13, 9, 13, 13, 13, 13, 13, 9, 9, 
	9, 9, 9, 9, 9, 11, 9, 9, 9, 9, 9, 9, 13, 13, 13, 6, 
	9, 13, 11, 13, 13, 11, 13, 13, 5, 5, 13, 11, 9, 13, 13, 13, 
	9, 13, 13, 13, 13, 9, 13, 9, 13, 13, 13, 9, 13  
}, { // lines
	10, 14, 3, 14, 4, 14, 14, 14, 14, 6, 17, 17, 9, 10, 7, 2, 
	4, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 10, 10, 5, 10, 
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
	14, 14, 17, 14, 14, 14, 14, 14, 14, 14, 14, 14, 17, 14, 17, 9, 
	2, 4, 10, 14, 10, 14, 10, 14, 13, 14, 14, 17, 14, 14, 10, 10, 
	10, 13, 13, 10, 10, 12, 10, 10, 10, 10, 13, 10, 17, 14, 17, 4, 
	13, 14, 8, 14, 17, 2, 14, 7, 5, 2, 14, 2, 10, 14, 14, 4, 
	13, 14, 14, 14, 14, 13, 14, 10, 16, 16, 14, 10, 14  
},
monoItalic16Bits
}
};
