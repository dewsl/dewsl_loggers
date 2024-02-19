#include "Due_config.h"

uint16_t lib_LOGGER_COUNT = 78;

// READ FIRST:

// 1. INCREMENT(+n) lib_LOGGER_COUNT if adding new entries/item to the config containter by the number of entries/items added.
// When editing an entry/item in the config containter, the  value of the lib_LOGGER_COUNT is retained

// CONFIG paramer arrangement (see example config "SMPLE" below)
// 1. Mastername
// 2. column IDs
// 3. number of nodes
// 4. sensor version
// 5. datalogger version
// 6. piezo
// 7. turn on delay
// 8. broadcast timeout
// 9. max sampling retry
// 10. b64

struct lib_config config_container[] = {
  {
    "EXPTA",                          // Mastername (5 char string)
    "5363,5324,5355,5344,5352,5180",  // column IDs
    6,                                // number of nodes
    5,                                // sensor version
    2,                                // datalogger version
    0,                                // piezo
    1000,                              // turn on delay
    3000,                             // broadcast timeout
    3,                                // max sampling retry
    1                                 // b64
  },

  {
    "EXPSA",                          // Mastername (5 char string)
    "2272,2094",  // column IDs
    2,                                // number of nodes
    3,                                // sensor version
    2,                                // datalogger version
    0,                                // piezo
    1000,                              // turn on delay
    3000,                             // broadcast timeout
    3,                                // max sampling retry
    1                                 // b64
  },

  { 
    "AGBSB",
    "2647,2674,2683,2695,2719,2738,2768,2779,2793,2794,2833,2857,2865,2800,2904,2946,2961,2903,2964,2965",
    20,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "AGBTA",
    "850,799,1134,967,637,580,560,492,432,429,383,245",
    12,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAKTA",
    "641,648,682,683,976,1016,1022,1070,1147,1199,1229,1414,1459,1550,1561,1604,1621,1623,1634,2710",
    20,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAKTB",
    "989,1032,1054,1085,1097,1120,1134,1136,1149,1168,1183,1209,1222,1230,1239,1308,1311,1326,1327,1344,1465,1467,1479,1564,1568,1575,2724",
    27,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAKTC",
    "638,659,793,872,887,906,921,928,932,934,939,947,1020,1052,1055,1059,1061,1096,1152,2749",
    20,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BANTA",
    "879,1014,1194,1195,1219,1236,1240,1303,1385,1494,1496,1520,1554,2756",
    14,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BANTB",
    "794,796,933,1341,1382,1389,1403,1469,1474,1484,1527,1577,1583,2700",
    14,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BARSC",
    "2951,2958,2978,2982,3060,3066,3070,3075,3299,3300,3311,3358,3389,3442,3517",
    15,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BARTB",
    "545,68,614,722,833,930,956,1165,1208,1309,1373,1415,1438,1471,2711",
    15,
    3,
    2,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAYSB",
    "2648,3147,3089,3074,3029,3057,3012,3011,2926,2888,2848,2058,2944,2677,2929,2821,2813,2744,2801,2708",
    20,
    2,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAYTA",
    "N/A",
    0,
    0,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BAYTC",
    "401,406,420,478,528,564,572,669,993,765,1009,1029,1488,1516,207,770,561,862,780,934,1355,1419,1426,1767,1781,2045,2101,229",
    28,
    2,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { 
    "BCNTA",
    "N/A",
    0,
    0,
    4,
    0,
    1000,
    3000,
    3,
    1 
  },

  { "BCNTB",
    "N/A",
    0,
    0,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "BLCSB",
    "3084,3100,3122,2910,2926,2930,2938,2941,2943,2952,2962,2976,2986,3005,3085,3099,3112,3119,3136,2718",
    20,
    2,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "BLCTA",
    "1579,546,579,600,613,692,699,834,836,903,924,968,1007,1103,1123,1176,1235,1266,1319,1347,1226,1402,1508,1509,1513,1552,1557,1560,1587,2717", 30, 3, 4, 0, 1000, 3000, 3, 1 },
  { "BOLTA", "1736,1739,1770,1776,1790,1796,1800,1843,1867,1883,1884,1887,1890,1891,1894,1976,2084,2146,2172,2221,2305,2312,2357,2368,2489,2522,2536,2554,2582,2781",
    30,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  {
    "BTOTA",
    "576,607,627,801,814,823,1033,1071,1256,1272,1367,1596,1603,1629,2732",
    15,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1  //old 576,607,627,801,814,823,1033,1071,1256,1272,1367,1596,1603,1629,2732
  },

  {
    "BTOTB",
    "735,839,893,1078,1163,1180,1451,1476,1477,1486,1526,1576,1607,2733",
    14,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1  // old 735,839,893,1078,1163,1180,1451,1476,1477,1486,1526,1576,1607,2733
  },

  { "CARTC",
    "4046, 4047, 4104, 4165, 4167, 4226, 4281, 4295, 4327",
    9,
    4,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "CARTD",
    "4038,4048,4065,4073,4139,4161,4220,4228,4229,4321",
    10,
    4,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "CUDTB",
    "1110,442,588,604,762,878,879,919,993,1025,1122,1170,1182,1717,1793,1823,242",
    17,
    2,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "DADTA",
    "524,655,1328,1406,1469,1644,1679,1695,1784,1826,1866,1933,2085,2090,2030,2198,2220,2240,2364,253",
    20,
    3,
    4,
    0,
    1000,
    3000,
    3, 1 },

  { "GAASA",
    "2947,2980,2981,2975,2983,2997,3000,3027,3031,3087,3125,3150,3166,3171,3539",
    15,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "GAATC",
    "1690,1693,1744,1749,1772,1818,1838,1847,1862,1888,1889,1917,1926,1962,1972,1974,1981,2018,2046,2071,2072,2080,2126,2131,2143,2148,2166,2188,2248,2265,2280,2319,2325,2331,2438,2475,2477,2545,2556,2798",
    40,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "HINSA",
    "3008,3025,3045,3050,3082,3096,3162,3202,3362,3400,3435,3439,3520",
    13,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "HINSB",
    "2922,2933,3058,3143,3154,3172,3187,3243,3245,3427,3313,3506,3285",
    13,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "HUMB",
    "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26",
    26,
    1,
    4,
    1,
    1000,
    3000,
    3,
    1 },

  { "IMESB",
    "3177,3186,3203,3209,3211,3269,3280,3289,3306,3314,3317,3383,3386,3394,3395,3410,3417,3430,3431,3531",
    20,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "IMETA",
    "551,567,636,665,711,757,768,1046,1259,1310,1569,2721",
    12,
    3,
    2,
    0,
    1000,
    3000,
    3,
    1 },

  { "IMUTA",
    "525,543,564,609,663,667,1306,1294,1320,1325,1329,1333,1337,1384,1417,1437,1441,1450,1455,1475,1512,1605,1616,1855,2715",
    25,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "IMUTD",
    "N/A",
    0,
    0,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "IMUTE",
    "4029,4037,4045,4053,4056,4066,4082,4084,4143,4118,4183,4184,4123,4124,4129,4202,4205,4244,4246,4252,4267,4268,4306,4328",
    24,
    4,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "INATA",
    "2752,1581,1516,1077,1026,1015,1013,1011,994,972,964,963,935,913,900,884,874,846",
    18,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "INATC",
    "2748,1105,1098,1073,1069,971,942,925,920,888,873,827,807,671",
    14,
    3,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "JORTA",
    "1636,1638,1659,1773,1817,1834,1837,1875,2039,2048,2075,2082,2112,2116,2128,2133,2135,2144,2163,2168,2205,2574,2587,2799",
    24,
    2,
    2,
    0,
    1000,
    3000,
    3,
    1 },

  { "KNRTA",
    "5102,5114,5139,5140,5141,5149,5220,5229,5290,5295,5309,5319,5331,5340,5346,5350,5357,5393",
    18,
    5,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "KNRTB",
    "N/A",
    0,
    0,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "LABB",
    "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25",
    25,
    1,
    4,
    0,
    1000,
    3000,
    3,
    1 },

  { "LABT", "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39", 39, 1, 4, 0, 1000, 3000, 3, 1 },
  { "LAYSA", "2992,3003,3059,3078,3092,3093,3097,3305,3357,3502", 10, 3, 4, 0, 1000, 3000, 3, 1 },
  { "LAYSB", "2914,2990,3037,3088,3148,3183,3188,3266,3320,3503", 10, 3, 4, 0, 1000, 3000, 3, 1 },
  { "LOOTA", "580,628,681,747,803,822,844,943,948,975,991,1008,1289,1324,1345,1379,1404,1425,1478,1535,1553,1631,2722", 23, 4, 4, 0, 1000, 3000, 3, 1 },
  { "LOOTB", "1653,1657,1673,1678,1692,1741,1715,1761,1763,1771,1783,1802,1826,1878,2074,2115,2120,2176,2189,2211,2234,2236,2316,2335,2347,2388,2453,2501,2769", 29, 3, 4, 1, 1000, 3000, 3, 1 },
  { "LPASA", "2940,2949,2955,3036,3002,3046,3076,3106,3124,3134,3137,3169,3282,3307,3507", 15, 3, 2, 0, 1000, 3000, 3, 1 },
  { "LPASB", "2912,2936,2954,3016,3048,3049,3116,3121,3146,3160,3170,3198,3342,3347,3408,2706", 16, 3, 2, 0, 1000, 3000, 3, 1 },
  { "LTESA", "2989,3052,3102,3115,3131,3140,3151,3159,3227,3229,3359,3384,3385,3523,2934", 15, 2, 4, 1, 1000, 3000, 3, 1 },
  { "LTETB", "523,560,589,597,602,658,677,718,820,1321,1391,1448,1457,2729", 14, 3, 4, 0, 1000, 3000, 3, 1 },
  { "LUNTC", "882,920,1048,1095,1215,1381,1560,1778,1828,1896,1937,1959,2003,2007,2063,2165,2284,2292,2381,219", 20, 2, 4, 0, 1000, 3000, 3, 1 },
  { "LUNTD", "1727,1775,1860,1885,1902,1929,1828,2011,2037,2058,2070,2090,2102,2114,2124,2140,2149,2162,2212,2241,2313,2328,2390,2392,2411,2482,2484,2506,2766", 29, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MAGTE", "4042,4075,4076,4091,4096,4101,4107,4134,4135,4137,4150,4162,4163,4164,4175,4178,4181,4186,4189,4203,4208,42214224,4225,4274,4277,4285,4294,4296,4323", 30, 4, 4, 0, 1000, 3000, 3, 1 },
  { "MAMTA", "649,651,694,709,811,840,868,875,896,960,1110,1140,1043,,1170,1181,1187,1191,1202,1265,1338,1366,1406,1433,1466,1497,1558,1611,1612,2736", 29, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MAMTB", "1418,962,1009,1023,1024,1031,1048,1198,1245,1247,1314,1343,1409,1458,1487,1523,1532,1545,1546,1555,1594,1595,1597,1600,1614,2245,2713", 77, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MARTA", "532,623,686,690,693,724,754,755,774,787,788,815,825,848,861,949,952,978,1003,2758", 20, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MARTB", "515,605,748,759,804,851,865,869,911,950,967,1040,1050,1093,1154,1157,1172,1182,1189,2703", 20, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MCATA", "2701,2744,2749,2778,2811,2849,2884,2932,2933,2937,2971,2983,3059,3060,3069,3100,3114,3118,3128,2635", 20, 2, 4, 0, 1000, 3000, 3, 1 },
  { "MNGSA", "2918,2931,2993,2998,3011,3065,3069,3071,3086,3109,3130,3132,3139,3235,3262,3319,3372,3512", 18, 3, 4, 0, 1000, 3000, 3, 1 },
  { "MNGTB", "974,1035,1064,1179,1212,1244,1282,1375,1376,1387,1423,1491,1551,1556,1566,1625,2759", 17, 3, 4, 0, 1000, 3000, 3, 1 },
  { "NURTA", "542,681,836,900,1018,1277,1339,1369,1745,1786,1870,1844,1891,1899,1909,1999,2072,2081,2092,2105,2135,2156,2206,2210,2238,2251,2253,2267,2297,244", 30, 2, 4, 0, 1000, 3000, 3, 1 },
  { "NURTB", "831, 963, 1185, 1346, 1352, 1367, 1370, 1416, 1421, 1454, 1483, 1505, 1507, 1625, 1632, 1666, 1819, 1872, 1888, 2056, 2167, 2214, 2230, 251", 24, 2, 4, 0, 1000, 3000, 3, 1 },
  { "PARTA", "1539,1639,1640,1641,1650,1664,1682,1689,1788,1873,1991,1935,1946,2108,2123,2156,2165,2161,2244,2253,2254,2262,2284,2299,2322,2326,2330,2433,2563,2762", 30, 3, 4, 0, 1000, 3000, 3, 1 },
  { "PARTB", "1697,1680,1865,1882,2029,2092,2093,2109,2134,2159,2296,2297,2333,2451,2533,2559,2560,2565,2572,2795", 20, 3, 4, 0, 1000, 3000, 3, 1 },
  { "PEPSB", "3428,3226,3233,3236,3242,3260,3273,3283,3295,3296,3297,3301,3302,3304,3330,3333,3334,3335,3336,3337,3339,3345,3346,3349,3351,3379,3382,3401,3441,3538", 29, 3, 4, 1, 1000, 3000, 3, 1 },
  { "PEPTC", "1966,1965,1880,1864,1842,1234,1196,1132,1052,897,634,611,214", 13, 2, 4, 0, 1000, 3000, 3, 1 },
  { "PNGTA", "662,684,704,767,772,776,810,832,877,885,905,914,946,1107,1155,1201,1214,1356,1361,1405,1444,1584,2741", 23, 3, 2, 0, 1000, 3000, 3, 1 },
  { "PNGTB", "1027,1047,1094,1108,1193,1233,1250,1251,1268,1277,1290,1291,1301,1353,1408,1435,1464,1468,1472,1498,1510,1515,1519,1544,1602,1610,1633,2275,2794", 29, 3, 2, 0, 1000, 3000, 3, 1 },
  { "SAGTA", "1147,1251,1327,1330,1340,1349,1362,1387,1393,1417,1433,1450,1510,1566,1578,1622,1637,1684,1798,206", 20, 2, 2, 0, 1000, 3000, 3, 1 },
  { "SAGTB", "311,412,425,664,700,728,746,856,1012,1186,1242,1317,1092,1120,1434,1459,1616,1636,1646,212", 20, 2, 2, 0, 1000, 3000, 3, 1 },
  { "SIBTA", "820,871,959,1225,1453,1461,1713,2021,2806,205", 10, 2, 4, 0, 1000, 3000, 3, 1 },
  { "SINSA", "2923,2939,2956,2966,2995,3074,3145,3152,3153,3155,3164,3168,3197,3232,3237,3238,3254,3259,3267,3281,3290,3315,3322,3328,3341,3369,3396,3412,3421,3514", 30, 3, 5, 0, 1000, 3000, 3, 1 },
  { "SINTB", "668,701,778,781,857,878,883,892,908,909,937,958,984,1001,1034,1037,1056,1083,1086,1142,1161,1188,1227,1249,1270,1276,2750", 27, 3, 5, 0, 1000, 3000, 3, 1 },
  { "SUMTA", "294,370,376,435,508,530,785,887,910,917,922,1072,1114,1271,254", 15, 3, 2, 0, 1000, 3000, 3, 1 },
  { "SUMTC", "1645,1648,1685,1719,1750,1823,1836,1937,1934,2036,2169,2247,2258,2264,2266,2283,2290,2293,2295,2327,2337,2351,2361,2381,2393,2396,2414,2415,2436,2499,2511,2523,2524,2527,2529,2531,2579,2589,2739", 39, 3, 4, 0, 1000, 3000, 3, 1 },
  { "TGATA", "504,507,511,530,537,556,568,598,601,606,644,661,672,679,800,847,870,880,882,917,951,1044,1062,1144,1225,1232,1246,1410,1431,2702", 30, 3, 2, 0, 1000, 3000, 3, 1 },
  { "TGATB", "617,621,631,680,717,762,784,785,821,881,957,979,996,1089,1106,1125,1221,1253,1254,1257,1260,1273,1275,1283,1286,1295,1400,1420,1495,2778", 30, 3, 2, 0, 1000, 3000, 3, 1 },
  { "TUETA", "779,780,813,849,864,895,953,954,965,999,1029,1057,1074,1075,1111,1113,1130,1151,1158,1206,1215,1217,1223,1224,1243,1271,1281,1313,1316,1317,1342,1416,1445,1446,2783", 35, 3, 4, 0, 1000, 3000, 3, 1 },
  { "TUETB", "471,490,522,550,627,667,699,748,782,809,827,837,839,859,867,907,912,918,1017,1038,1093,1181,1289,1295,1604,1680,1709,1727,1741,1783,2099,208", 32, 2, 4, 0, 1000, 3000, 3, 1 }
};
