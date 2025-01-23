#include "fidoFont.h"

const unsigned long propBold12Bits[] = {
// � (1)
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// M (2)
0x69963369, 0x96000000, 
0x9FEC33BD, 0xF9000000, 
0x9FBF33EA, 0xF9000000, 
0x9F9E74E9, 0xF9000000, 
0x9F9AA7B9, 0xF9000000, 
0x9F97EB89, 0xF9000000, 
0x9F93FE59, 0xF9000000, 
0x9F93DF39, 0xF9000000, 
0x9F93AC39, 0xF9000000, 
0x69634636, 0x96000000, 
// � (3)
0x38995000, 
0x59980000, 
// M (4)
0x69963369, 0x96000000, 
0x9FEC33BD, 0xF9000000, 
0x9FBF33EA, 0xF9000000, 
0x9F9E74E9, 0xF9000000, 
0x9F9AA7B9, 0xF9000000, 
0x9F97EB89, 0xF9000000, 
0x9F93FE59, 0xF9000000, 
0x9F93DF39, 0xF9000000, 
0x9F93AC39, 0xF9000000, 
0x69634636, 0x96000000, 
// � (5)
0x3BBB0000, 
0x57375000, 
// ! (33)
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
0x69600000, 
0x9F900000, 
0x69600000, 
// # (35)
0x03645600, 
0x03C6A700, 
0x03E3D400, 
0x369F9F96, 
0x37EBCC94, 
0x33E5B700, 
0x49F9EB70, 
0x6CCBE960, 
0x3B77B000, 
0x38476000, 
// % (37)
0x34884337, 0x50000000, 
0x4FDDF43E, 0x00000000, 
0x9F99F230, 0x00000000, 
0x4FDDF5E0, 0x00000000, 
0x34884990, 0x00000000, 
0x05E58840, 0x00000000, 
0x049BFDDF, 0x40000000, 
0x04EAF99F, 0x90000000, 
0x03994FDD, 0xF4000000, 
0x03833488, 0x40000000, 
// & (38)
0x33696000, 0x00000000, 
0x36F9E600, 0x00000000, 
0x39E3B900, 0x00000000, 
0x34FDF600, 0x00000000, 
0x35EFB366, 0x00000000, 
0x4FEAF6E8, 0x00000000, 
0x9FA3CFF6, 0x00000000, 
0x7FA35FF0, 0x00000000, 
0x3DFADCFB, 0x00000000, 
0x33796379, 0x50000000, 
// ' (39)
0x66000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x66000000, 
// ( (40)
0x03950000, 
0x33DB0000, 
0x3BF00000, 
0x4FB00000, 
0x9F900000, 
0x9F900000, 
0x5FB00000, 
0x3BF00000, 
0x33DB0000, 
0x03940000, 
// ) (41)
0x49500000, 
0x3BF00000, 
0x33FC0000, 
0x33BF5000, 
0x339F9000, 
0x339F9000, 
0x33BF5000, 
0x34FC0000, 
0x3BF00000, 
0x59400000, 
// * (42)
0x33660000, 
0x6BCCB600, 
0x37FF7000, 
0x3A65A000, 
// + (43)
0x33660000, 
0x33990000, 
0x33990000, 
0x69CC9600, 
0x69CC9600, 
0x33990000, 
0x33990000, 
0x33660000, 
// , (44)
0x69600000, 
0x9F900000, 
0x6C900000, 
0x6E400000, 
0x64000000, 
// - (45)
0x69960000, 
0x69960000, 
// . (46)
0x69600000, 
0x9F900000, 
0x69600000, 
// / (47)
0x33490000, 
0x338D0000, 
0x33D80000, 
0x34F50000, 
0x38E00000, 
0x3CA00000, 
0x3F500000, 
0x7E000000, 
0xBB000000, 
0x95000000, 
// 0 (48)
0x33797000, 
0x3CF9FC00, 
0x5FB3BF50, 
0x8F939F80, 
0x9F939F90, 
0x9F939F90, 
0x8F939F80, 
0x5FB3BF50, 
0x3CF9FC00, 
0x33797000, 
// 1 (49)
0x33796000, 
0x6BFF9000, 
0x69CF9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x33696000, 
// 2 (50)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF70, 
0x69639F90, 
0x04EF6000, 
0x03DFA000, 
0x33DF7000, 
0x3DF70000, 
0x7FF23600, 
0x62560000, 
// 3 (51)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF80, 
0x6963AF90, 
0x336BFF40, 
0x336BFF40, 
0x6963AF90, 
0x7FA3AF80, 
0x3DF9FD00, 
0x33797000, 
// 4 (52)
0x03796000, 
0x334FF900, 
0x33BEF900, 
0x34E9F900, 
0x3B79F900, 
0x4E39F900, 
0x9C9CFC60, 
0x699CFC60, 
0x039F9000, 
0x03696000, 
// 5 (53)
0x33246000, 
0x36E23600, 
0x36C00000, 
0x38C98000, 
0x39F9FE00, 
0x3673BF80, 
0x69639F90, 
0x7FB3BF70, 
0x3DFAFC00, 
0x33797000, 
// 6 (54)
0x33798000, 
0x3BFAFF40, 
0x5FC36960, 
0x7FA88000, 
0x9FFAFE00, 
0x9FB3BF70, 
0x9F939F90, 
0x6FB3BF70, 
0x3DFAFD00, 
0x33797000, 
// 7 (55)
0x62560000, 
0x623BF700, 
0x04FC0000, 
0x03BF4000, 
0x334FB000, 
0x33BF7000, 
0x33FF0000, 
0x36FC0000, 
0x38FA0000, 
0x36960000, 
// 8 (56)
0x33797000, 
0x3DF9FD00, 
0x8FA3AF80, 
0x8FA3AF80, 
0x3CF9FC00, 
0x3EF9FD00, 
0x8FA3AF80, 
0x8FA3AF80, 
0x3FF9FD00, 
0x33897000, 
// 9 (57)
0x33797000, 
0x3DF9FB00, 
0x7FB3BF40, 
0x9F939F80, 
0x8FA3BF90, 
0x3FFAFF90, 
0x3388AF70, 
0x6963BF40, 
0x4FFAF900, 
0x33896000, 
// : (58)
0x69600000, 
0x9F900000, 
0x69600000, 
0x00000000, 
0x69600000, 
0x9F900000, 
0x69600000, 
// < (60)
0x057B9000, 
0x337BFB70, 
0x7FB70000, 
0x7FB70000, 
0x337BFB70, 
0x057B9000, 
// = (61)
0x62560000, 
0x62560000, 
0x62560000, 
0x62560000, 
// > (62)
0x8B700000, 
0x37BFB700, 
0x047BF700, 
0x047BF700, 
0x37BFB700, 
0x9B700000, 
// ? (63)
0x33797000, 
0x3DF9FE00, 
0x7FB3AF80, 
0x6963AF80, 
0x039FE000, 
0x337FD000, 
0x33696000, 
0x33696000, 
0x339F9000, 
0x33696000, 
// @ (64)
0x03589700, 0x00000000, 
0x33BEA9BF, 0x70000000, 
0x3BB48868, 0xE4000000, 
0x5E3CBBF5, 0xA8000000, 
0x9A7D37E3, 0x99000000, 
0x243CA3D6, 0x00000000, 
0x7D7DBFDB, 0xB0000000, 
0x3D67839B, 0x70000000, 
0x33DB99CE, 0x70000000, 
0x03799700, 0x00000000, 
// A (65)
0x33699600, 
0x33CFFC00, 
0x33140000, 
0x37FBBF70, 
0x3BF77FB0, 
0x3FF44FF0, 
0x7FF99FF6, 
0xBFB99BFA, 
0xFF4334FE, 
0x97048900, 
// B (66)
0x62484000, 
0x9FC99FF4, 
0x9F933AF9, 
0x9F933AF8, 
0x9FC99FC0, 
0x9FC99FE0, 
0x9F933AF8, 
0x9F933AF8, 
0x9FC99FD0, 
0x62470000, 
// C (67)
0x03798400, 
0x35FE9CF8, 
0x3EF03EF5, 
0x7FB03696, 
0x9F900000, 
0x9F900000, 
0x7FB03696, 
0x3FF03EF4, 
0x35FE9CF7, 
0x03798400, 
// D (68)
0x62450000, 
0x9FC9BF80, 
0x9F933DF4, 
0x9F933AF7, 
0x9F9339F9, 
0x9F9339F9, 
0x9F933BF7, 
0x9F933EF0, 
0x9FC9CF70, 
0x62440000, 
// E (69)
0x62660000, 
0x9FC24600, 
0x9F900000, 
0x9FC24000, 
0x9FC24000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9FC24600, 
0x62660000, 
// F (70)
0x62560000, 
0x9FC23600, 
0x9F900000, 
0x9F900000, 
0x9FC99600, 
0x9FC99600, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// G (71)
0x03798400, 0x00000000, 
0x35EE9CF9, 0x00000000, 
0x3EF433DF, 0x50000000, 
0x7FB03696, 0x00000000, 
0x9F936236, 0x00000000, 
0x9F9369BF, 0x90000000, 
0x7FB038F9, 0x00000000, 
0x3FF03EF9, 0x00000000, 
0x35FE9CEE, 0x90000000, 
0x03897366, 0x00000000, 
// H (72)
0x69633696, 
0x9F9339F9, 
0x9F9339F9, 
0x9FC99CF9, 
0x9FC99CF9, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x69633696, 
// I (73)
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// J (74)
0x04696000, 
0x049F9000, 
0x049F9000, 
0x049F9000, 
0x049F9000, 
0x69639F90, 
0x9F939F90, 
0x6FB39F80, 
0x3DF9FF00, 
0x33798000, 
// K (75)
0x69633797, 
0x9F937FD0, 
0x9F97FF50, 
0x9FBFF500, 
0x913B0000, 
0x9FFEF700, 
0x9FA5FF00, 
0x9F939FB0, 
0x9F933EF7, 
0x69633598, 
// L (76)
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9FC23600, 
0x62560000, 
// M (77)
0x69963369, 0x96000000, 
0x9FEC33BD, 0xF9000000, 
0x9FBF33EA, 0xF9000000, 
0x9F9E74E9, 0xF9000000, 
0x9F9AA7B9, 0xF9000000, 
0x9F97EB89, 0xF9000000, 
0x9F93FE59, 0xF9000000, 
0x9F93DF39, 0xF9000000, 
0x9F93AC39, 0xF9000000, 
0x69634636, 0x96000000, 
// N (78)
0x69733696, 
0x9FF439F9, 
0x9FF939F9, 
0x9FAE39F9, 
0x9F9B79F9, 
0x9F96B9F9, 
0x9F93DBF9, 
0x9F937FF9, 
0x9F933FF9, 
0x69633796, 
// O (79)
0x03898000, 
0x35FE9DF5, 
0x3FF03FF0, 
0x7FB03BF7, 
0x9F9039F9, 
0x9F9039F9, 
0x7FB03BF7, 
0x3FF03FF0, 
0x35FE9DF5, 
0x03898000, 
// P (80)
0x62470000, 
0x9FC9AFF4, 
0x9F933AF9, 
0x9F933AF9, 
0x9FC9AFF4, 
0x9FC99700, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// Q (81)
0x03797000, 
0x35FD9DF5, 
0x3FF03FE0, 
0x7FB03BF7, 
0x9F9039F9, 
0x9F9039F9, 
0x7FB03BF7, 
0x3FF34EFF, 
0x35FE9DFB, 
0x038986B0, 
// R (82)
0x62484000, 
0x9FC99FF0, 
0x9F933AF8, 
0x9F933AF8, 
0x9FC99FC0, 
0x9FC99FE4, 
0x9F933AF9, 
0x9F9339F9, 
0x9F9339F9, 
0x69633696, 
// S (83)
0x33799700, 
0x3DFABFD0, 
0x8FA33BF7, 
0x9FB33696, 
0x4EFEB800, 
0x337BDFF4, 
0x69633BF9, 
0x7FB33AF8, 
0x3BFBAFD0, 
0x33599700, 
// T (84)
0x62760000, 0x00000000, 
0x699CFC99, 0x60000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x039F9000, 0x00000000, 
0x03696000, 0x00000000, 
// U (85)
0x69633696, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x9F9339F9, 
0x7FB33BF7, 
0x3BFAAFB0, 
0x33799700, 
// V (86)
0x99049900, 
0xCF7337FD, 
0x8FB33AF9, 
0x5FE33EF5, 
0x3EF55FE0, 
0x3AF88FB0, 
0x37FCCF70, 
0x33140000, 
0x33BFFB00, 
0x33699600, 
// W (87)
0x49733993, 0x37900000, 
0x4FF33FF3, 0x3EF00000, 
0x3FF46FF6, 0x3FF00000, 
0x3CF77DC7, 0x6FC00000, 
0x39F9ABA9, 0x9FA00000, 
0x37FCC98C, 0xBF700000, 
0x34FEF66F, 0xEF500000, 
0x33134413, 0x00000000, 
0x33CFF33F, 0xFC000000, 
0x33797337, 0x97000000, 
// X (88)
0x89533598, 
0x7FC33CF7, 
0x3BF66FB0, 
0x34FCCF00, 
0x338FF700, 
0x339FF900, 
0x34FCBF50, 
0x3CF64FC0, 
0x7FC33BF7, 
0x89533598, 
// Y (89)
0x59703795, 
0x3FF434FF, 
0x39FB3BF9, 
0x34FF4FF4, 
0x33BFEFB0, 
0x33413400, 
0x03BFB000, 
0x039F9000, 
0x039F9000, 
0x03696000, 
// Z (90)
0x62660000, 
0x624EF800, 
0x047FC000, 
0x04FF4000, 
0x03BF7000, 
0x338FB000, 
0x34FF0000, 
0x3DF70000, 
0x8FE24600, 
0x62660000, 
// [ (91)
0x69960000, 
0x9FC60000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9FC60000, 
0x69960000, 
// \ (92)
0x76000000, 
0x7C000000, 
0x3E400000, 
0x39A00000, 
0x33F00000, 
0x33B70000, 
0x336D0000, 
0x03E50000, 
0x038B0000, 
0x04800000, 
// ] (93)
0x69960000, 
0x6CF90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x6CF90000, 
0x69960000, 
// ^ (94)
0x33880000, 
0x34FF4000, 
0x3BCCB000, 
0x4F44F400, 
0x9B33B900, 
0x80480000, 
// _ (95)
0x62660000, 
0x62660000, 
// ` (96)
0x59600000, 
0x3BE00000, 
0x33850000, 
// a (97)
0x34899700, 
0x4FF9DF70, 
0x6963BF90, 
0x39E13900, 
0x6FE79F90, 
0x9F93BF90, 
0x6FDAFF90, 
0x36975960, 
// b (98)
0x69600000, 
0x9F900000, 
0x9FA88000, 
0x9FF9FD00, 
0x9FB3BF70, 
0x9F939F90, 
0x9F939F90, 
0x9FB3BF70, 
0x9FFAFD00, 
0x69788000, 
// c (99)
0x33798400, 
0x3DF9FF40, 
0x7FB36960, 
0x9F900000, 
0x9F936960, 
0x7FC3BF60, 
0x3DFAFB00, 
0x33797000, 
// d (100)
0x04696000, 
0x049F9000, 
0x3388AF90, 
0x3DFBFF90, 
0x7FB3BF90, 
0x9F939F90, 
0x9F939F90, 
0x7FB3BF90, 
0x3DFAFF90, 
0x33887960, 
// e (101)
0x33797000, 
0x3BFAFC00, 
0x7FB3BF70, 
0x9FC9CF90, 
0x9FC23600, 
0x7FB37960, 
0x3BFAFE00, 
0x33797000, 
// f (102)
0x33796000, 
0x38FD6000, 
0x6CFC6000, 
0x6CFC6000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x36960000, 
// g (103)
0x33887960, 
0x3DFAFF90, 
0x7FB3BF90, 
0x9F939F90, 
0x9F939F90, 
0x7FB3BF90, 
0x3DFBFF90, 
0x3388AF90, 
0x6963AF70, 
0x4FF9FD00, 
0x34897000, 
// h (104)
0x69600000, 
0x9F900000, 
0x9F979400, 
0x9FFAFF40, 
0x9FB39F90, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x69636960, 
// i (105)
0x69600000, 
0x69600000, 
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// j (106)
0x36960000, 
0x36960000, 
0x36960000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x6DF60000, 
0x69700000, 
// k (107)
0x69600000, 
0x9F900000, 
0x9F938400, 
0x9F96D000, 
0x9F9E6000, 
0x9FEF0000, 
0x91370000, 
0x9F9CD000, 
0x9F97F500, 
0x69639600, 
// l (108)
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// m (109)
0x69679738, 0x96000000, 
0x9FF9139F, 0xF6000000, 
0x9F939FA3, 0x9F900000, 
0x9F939F93, 0x9F900000, 
0x9F939F93, 0x9F900000, 
0x9F939F93, 0x9F900000, 
0x9F939F93, 0x9F900000, 
0x69636963, 0x69600000, 
// n (110)
0x69679600, 
0x9FEAFF60, 
0x9FB39F90, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x69636960, 
// o (111)
0x33599500, 
0x3BFBBFB0, 
0x5FC33CF5, 
0x9F9339F9, 
0x9F9339F9, 
0x5FC33CF5, 
0x3BFBBFB0, 
0x33599500, 
// p (112)
0x69788000, 
0x9FFBFD00, 
0x9FB3BF70, 
0x9F939F90, 
0x9F939F90, 
0x9FB3BF70, 
0x9FFAFD00, 
0x9FA88000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// q (113)
0x33887960, 
0x3DFBFF90, 
0x7FB3BF90, 
0x9F939F90, 
0x9F939F90, 
0x7FB3BF90, 
0x3DFAFF90, 
0x3388AF90, 
0x049F9000, 
0x049F9000, 
0x04696000, 
// r (114)
0x69760000, 
0x9FE90000, 
0x9FF60000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// s (115)
0x33798400, 
0x4FD9FF40, 
0x9FB36960, 
0x713B7000, 
0x36B13700, 
0x6963BF90, 
0x5FF9DF40, 
0x34898400, 
// t (116)
0x36960000, 
0x39F90000, 
0x6CFC6000, 
0x6CFC6000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x39F90000, 
0x38FC6000, 
0x33796000, 
// u (117)
0x69636960, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x9F939F90, 
0x9F93AF90, 
0x5FFAFF90, 
0x36976960, 
// v (118)
0x49733794, 
0x3FF33FF4, 
0x3DF65FE0, 
0x3AF98FB0, 
0x37FCBF70, 
0x34144000, 
0x33EFFE00, 
0x33799700, 
// w (119)
0x99337733, 0x99000000, 
0xEF43EE34, 0xFE000000, 
0xAF75FF57, 0xFA000000, 
0x7FA8BB8A, 0xF7000000, 
0x3FCB88BC, 0xF0000000, 
0x3CFF44FF, 0xB0000000, 
0x39FE33EF, 0x80000000, 
0x34973379, 0x40000000, 
// x (120)
0x89559800, 
0x8FCCF800, 
0x3DFFD000, 
0x36FF6000, 
0x36FF6000, 
0x3EFFE000, 
0x8FBBF800, 
0x89449800, 
// y (121)
0x49733794, 
0x4FF33FF0, 
0x3EF44FD0, 
0x3BF77FB0, 
0x39F99F70, 
0x36FCCF40, 
0x3313E000, 
0x33AFFB00, 
0x03EF8000, 
0x36BFB000, 
0x36970000, 
// z (122)
0x62560000, 
0x623DF800, 
0x037FB000, 
0x335FD000, 
0x33DF5000, 
0x3BF70000, 
0x8FD23600, 
0x62560000, 
// { (123)
0x33496000, 
0x36FF6000, 
0x39F90000, 
0x39F90000, 
0x3AF80000, 
0x7FB00000, 
0x7FC00000, 
0x3AF80000, 
0x39F90000, 
0x39F90000, 
0x36FF6000, 
0x33486000, 
// | (124)
0x66000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x66000000, 
// } (125)
0x69400000, 
0x6FF60000, 
0x39F90000, 
0x39F90000, 
0x38FA0000, 
0x33BF7000, 
0x33CF7000, 
0x39FA0000, 
0x39F90000, 
0x39F90000, 
0x7FF60000, 
0x68400000, 
// ~ (126)
0x34903660, 
0x3FBD53B7, 
0x7B35DAE0, 
0x66039500, 
// � (161)
0x69600000, 
0x9F900000, 
0x69600000, 
0x69600000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x9F900000, 
0x69600000, 
// � (163)
0x33797000, 
0x3FFABD00, 
0x8FA33B70, 
0x8F933660, 
0x7FC99600, 
0x6ED99600, 
0x39F60000, 
0x3CB94340, 
0x6D9FFD70, 
0x40386000, 
// � (164)
0x55885500, 
0x5FBBF500, 
0x8B33B800, 
0x8B33B800, 
0x5FBBF500, 
0x55885500, 
// | (166)
0x66000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x99000000, 
0x66000000, 
// � (167)
0x33898400, 
0x4FE9FF40, 
0x9FA36960, 
0x6FFE8000, 
0x8FCFFB00, 
0x9D339F70, 
0x7FC63C90, 
0x39FFEF50, 
0x334DFF70, 
0x6965BF90, 
0x4FF9FF40, 
0x33797000, 
// � (168)
0x66660000, 
0x66660000, 
// � (169)
0x03599500, 0x00000000, 
0x33CE99EC, 0x00000000, 
0x3CB3894B, 0xC0000000, 
0x5E3FAAE4, 0xE5000000, 
0x998A3376, 0x99000000, 
0x998A3376, 0x99000000, 
0x5E3EBAE4, 0xE5000000, 
0x3CB3894B, 0xC0000000, 
0x33CE99EC, 0x00000000, 
0x03599500, 0x00000000, 
// � (170)
0x37970000, 
0x5ACE8000, 
0x6D9B9000, 
0x8D9F9000, 
0x38876000, 
// � (172)
0x62460000, 
0x623C9000, 
0x04990000, 
0x04990000, 
0x04660000, 
// � (173)
0x62660000, 
0x62660000, 
// � (174)
0x03599500, 0x00000000, 
0x33CE99EC, 0x00000000, 
0x3CE9973B, 0xC0000000, 
0x5E9C9E83, 0xE5000000, 
0x23C9D739, 0x90000000, 
0x23C9F639, 0x90000000, 
0x5E993993, 0xE5000000, 
0x3CE6366B, 0xC0000000, 
0x33CE99EC, 0x00000000, 
0x03599500, 0x00000000, 
// � (175)
0x62360000, 
0x62360000, 
// x (176)
0x89559800, 
0x8FCCF800, 
0x3DFFD000, 
0x36FF6000, 
0x36FF6000, 
0x3EFFE000, 
0x8FBBF800, 
0x89449800, 
// 2 (178)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF70, 
0x69639F90, 
0x04EF6000, 
0x03DFA000, 
0x33DF7000, 
0x3DF70000, 
0x7FF23600, 
0x62560000, 
// 3 (179)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF80, 
0x6963AF90, 
0x336BFF40, 
0x336BFF40, 
0x6963AF90, 
0x7FA3AF80, 
0x3DF9FD00, 
0x33797000, 
// � (180)
0x36950000, 
0x3EB00000, 
0x58000000, 
// � (181)
0x36963990, 
0x3CF75FE0, 
0x3DF56FC0, 
0x3FF39FA0, 
0x4FE3AF90, 
0x6FD3DF60, 
0x8FEBFF50, 
0x9FA79900, 
0xCF600000, 
0xDF500000, 
0x99000000, 
// 1 (182)
0x33796000, 
0x6BFF9000, 
0x69CF9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x33696000, 
// 2 (183)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF70, 
0x69639F90, 
0x04EF6000, 
0x03DFA000, 
0x33DF7000, 
0x3DF70000, 
0x7FF23600, 
0x62560000, 
// 3 (184)
0x33797000, 
0x3DF9FD00, 
0x7FA3AF80, 
0x6963AF90, 
0x336BFF40, 
0x336BFF40, 
0x6963AF90, 
0x7FA3AF80, 
0x3DF9FD00, 
0x33797000, 
// 1 (185)
0x33796000, 
0x6BFF9000, 
0x69CF9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x33696000, 
// � (191)
0x33696000, 
0x339F9000, 
0x33696000, 
0x33696000, 
0x33BF8000, 
0x37FF4000, 
0x6FF50000, 
0x9F936960, 
0x4FF9FF50, 
0x33798400, 
// � (198)
0x33427600, 0x00000000, 
0x33A13C23, 0x60000000, 
0x33EFCF90, 0x00000000, 
0x34FF9F90, 0x00000000, 
0x38FB9FC2, 0x36000000, 
0x3DF79FC2, 0x36000000, 
0x3FFACF90, 0x00000000, 
0x7FD9CF90, 0x00000000, 
0xBF739FC2, 0x36000000, 
0x99336256, 0x00000000, 
// x (215)
0x89559800, 
0x8FCCF800, 
0x3DFFD000, 
0x36FF6000, 
0x36FF6000, 
0x3EFFE000, 
0x8FBBF800, 
0x89449800, 
// � (222)
0x03499600, 
0x334FF960, 
0x338F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x69FF4000, 
0x69940000, 
// � (223)
0x03499600, 
0x334FF960, 
0x338F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x339F9000, 
0x69FF4000, 
0x69940000, 
// � (240)
0x34895000, 
0x34FAC700, 
0x33433C40, 
0x33797970, 
0x3BBAFF90, 
0x7D33BF90, 
0x99339F60, 
0x7B33CD00, 
0x3EABF500, 
0x33880000, 
// � (248)
0x33699775, 
0x3BFBBFF0, 
0x5FB37FF7, 
0x9F97C9F9, 
0x9FBC39F9, 
0x7FE33CF4, 
0x6FFABF70, 
0x73799400, 
// ? (254)
0x33797000, 
0x3DF9FE00, 
0x7FB3AF80, 
0x6963AF80, 
0x039FE000, 
0x337FD000, 
0x33696000, 
0x33696000, 
0x339F9000, 
0x33696000, 
0 };

const proportional_font propBold12 = {
fontTypes::proportional,
{ // widths
0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
7, 4, 2, 7, 9, 11, 9, 2, 5, 5, 5, 6, 4, 4, 4, 4, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 4, 7, 7, 7, 7, 
11, 9, 9, 10, 9, 9, 7, 10, 9, 4, 7, 9, 7, 11, 9, 10, 
9, 10, 9, 9, 8, 9, 9, 11, 9, 8, 8, 4, 4, 4, 6, 7, 
4, 7, 7, 7, 7, 7, 4, 7, 7, 3, 3, 6, 3, 11, 7, 8, 
7, 7, 4, 7, 5, 7, 7, 11, 7, 7, 6, 5, 3, 5, 7, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
7, 0, 7, 7, 7, 8, 0, 0, 9, 9, 0, 7, 3, 0, 12, 0, 
0, 6, 0, 0, 0, 7, 6, 0, 0, 0, 0, 7, 0, 0, 0, 8, 
9, 9, 9, 9, 9, 9, 0, 10, 9, 9, 9, 9, 4, 4, 4, 4, 
9, 9, 10, 10, 10, 10, 10, 0, 10, 9, 9, 9, 9, 8, 0, 0, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 
0, 7, 8, 8, 8, 8, 8, 4, 4, 7, 7, 7, 7, 7, 0, 7  
}, {
	0x0001, /* version */
	fontTypes::proportional, /* family */
	fontTypes::bold, /* style */
	fontTypes::size0, /* size (12) */
	9, /* max ascent */
	3, /* max descent */
	125, /* glyph count */
	iso8859_1_glyph[0], /* glyphs */
{ // offsets
	0x0000, 0x0008, 0x001C, 0x001E, 0x0032, 0x0034, 0x003E, 0x0048, 
	0x005C, 0x0070, 0x0075, 0x007F, 0x0089, 0x008D, 0x0095, 0x009A, 
	0x009C, 0x009F, 0x00A9, 0x00B3, 0x00BD, 0x00C7, 0x00D1, 0x00DB, 
	0x00E5, 0x00EF, 0x00F9, 0x0103, 0x010D, 0x0114, 0x011A, 0x011E, 
	0x0124, 0x012E, 0x0142, 0x014C, 0x0156, 0x0160, 0x016A, 0x0174, 
	0x017E, 0x0192, 0x019C, 0x01A6, 0x01B0, 0x01BA, 0x01C4, 0x01D8, 
	0x01E2, 0x01EC, 0x01F6, 0x0200, 0x020A, 0x0214, 0x0228, 0x0232, 
	0x023C, 0x0250, 0x025A, 0x0264, 0x026E, 0x0278, 0x0282, 0x028C, 
	0x0292, 0x0294, 0x0297, 0x029F, 0x02A9, 0x02B1, 0x02BB, 0x02C3, 
	0x02CD, 0x02D8, 0x02E2, 0x02EC, 0x02F8, 0x0302, 0x030C, 0x031C, 
	0x0324, 0x032C, 0x0337, 0x0342, 0x034A, 0x0352, 0x035C, 0x0364, 
	0x036C, 0x037C, 0x0384, 0x038F, 0x0397, 0x03A3, 0x03AD, 0x03B9, 
	0x03BD, 0x03C7, 0x03D1, 0x03D7, 0x03E1, 0x03ED, 0x03EF, 0x0403, 
	0x0408, 0x040D, 0x040F, 0x0423, 0x0425, 0x042D, 0x0437, 0x0441, 
	0x0444, 0x044F, 0x0459, 0x0463, 0x046D, 0x0477, 0x0481, 0x0495, 
	0x049D, 0x04A8, 0x04B3, 0x04BD, 0x04C5, 0x04CF
}, { // kern
	0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 
	-1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 
	-1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, -1, -2, -1, -1, 0, 0, -1, -1, 0, 
	-1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 
	0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1  
}, { // ascent
	7, 9, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 2, 4, 
	2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 6, 7, 4, 7, 
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 
	-1, 9, 7, 9, 7, 9, 7, 9, 7, 9, 9, 8, 9, 9, 7, 7, 
	7, 7, 7, 7, 7, 9, 7, 7, 7, 7, 7, 7, 9, 9, 9, 5, 
	7, 9, 7, 9, 9, 9, 9, 9, 5, 4, 9, 9, 7, 9, 9, 9, 
	7, 9, 9, 9, 9, 7, 9, 7, 8, 8, 9, 8, 9  
}, { // lines
	8, 10, 2, 10, 2, 10, 10, 10, 10, 5, 10, 10, 4, 8, 5, 2, 
	3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 7, 6, 4, 6, 
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 6, 
	2, 3, 8, 10, 8, 10, 8, 10, 11, 10, 10, 12, 10, 10, 8, 8, 
	8, 11, 11, 8, 8, 10, 8, 8, 8, 8, 11, 8, 12, 10, 12, 4, 
	10, 10, 6, 10, 12, 2, 10, 5, 5, 2, 10, 2, 8, 10, 10, 3, 
	11, 10, 10, 10, 10, 10, 10, 8, 11, 11, 10, 8, 10  
},
propBold12Bits
}
};
