/*
 * boardid.h
 *
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __BOARDID_H__
#define __BOARDID_H__

#include <boardid_base.h>

#define BOARD_ID_N              3
#define BOARD_ID_REV_N          5
#define BOARD_ID_REV_LEN        BOARD_ID_REV_N - BOARD_ID_N

// BOARD_ID_INDEX will be available in boarid_base.h
#define BOARD_IS_(id, b, n) (strncmp((id+BOARD_ID_INDEX), (b), (n)) == 0)
#define BOARD_REV_GRT_(id, b) (strncmp((id+BOARD_ID_INDEX+BOARD_ID_N), (b+BOARD_ID_INDEX+BOARD_ID_N), BOARD_ID_REV_LEN) > 0)
#define BOARD_REV_GRT_EQ_(id, b) (strncmp((id+BOARD_ID_INDEX+BOARD_ID_N), (b+BOARD_ID_INDEX+BOARD_ID_N), BOARD_ID_REV_LEN) >= 0)

#define BOARD_REV_GREATER(id, b) (BOARD_IS_((id), (b), BOARD_ID_N) && BOARD_REV_GRT_((id), (b)))
#define BOARD_REV_GREATER_EQ(id, b) (BOARD_IS_((id), (b), BOARD_ID_N) && BOARD_REV_GRT_EQ_((id), (b)))
#define BOARD_REV_EQ(id, b) (strncmp((id), b, BOARD_ID_REV_N) == 0)

#define BOARD_ID_abc123                "000"
#define BOARD_ID_abc123_3              "00003"
#define BOARD_ID_abc123_5              "00005"
#define BOARD_ID_abc123               "001"
#define BOARD_ID_abc123                 "002"
#define BOARD_ID_abc123              "003"
#define BOARD_ID_abc123_EVT1         "00301"
#define BOARD_ID_abc123               "004"
#define BOARD_ID_abc123_EVT1          "00400"
#define BOARD_ID_abc123              "005"
#define BOARD_ID_abc123_PROTO        "00501"
#define BOARD_ID_abc123_EVT1         "00512"
#define BOARD_ID_abc123_WFO          "006"
#define BOARD_ID_abc123_WFO_PROTO    "00600"
#define BOARD_ID_abc123_WFO_EVT1     "00601"
#define BOARD_ID_abc123_EVT2         "00309"
#define BOARD_ID_abc123_EVT2         "00516"
#define BOARD_ID_abc123_EVT3         "00520"
#define BOARD_ID_abc123_WFO_EVT2     "00605"
#define BOARD_ID_abc123_WFO_EVT3     "00606"
#define BOARD_ID_abc123ME              "007"
#define BOARD_ID_abc123ME_1            "00701"
#define BOARD_ID_abc123ME_3            "00703"
#define BOARD_ID_abc123_512          "009"
#define BOARD_ID_abc123_EVT1_1       "00903"
#define BOARD_ID_abc123_EVT1_2       "00906"
#define BOARD_ID_abc123_EVT2         "00908"
#define BOARD_ID_abc123_EVT2_3       "00909"
#define BOARD_ID_abc123_EVT2_4       "00910"
#define BOARD_ID_abc123_512_EVT3     "00910"
#define BOARD_ID_abc123_WFO_256      "00A"
#define BOARD_ID_abc123_WFO_EVT1_1   "00A03"
#define BOARD_ID_abc123_WFO_EVT1_2   "00A06"
#define BOARD_ID_abc123_WFO_EVT2     "00A08"
#define BOARD_ID_abc123_WFO_EVT2_3   "00A09"
#define BOARD_ID_abc123_WFO_EVT2_4   "00A10"
#define BOARD_ID_abc123_WFO_256_EVT3 "00A10"
#define BOARD_ID_abc123_256          "015"
#define BOARD_ID_abc123_256_EVT3     "01510"
#define BOARD_ID_abc123_WFO_512      "016"
#define BOARD_ID_abc123_WFO_512_EVT3 "01610"
/* #define BOARD_ID_abc123              "00D" - OBSOLETE */
/* #define BOARD_ID_abc123_PROTO        "00D01" - OBSOLETE */
#define BOARD_ID_abc123                    "01A"
#define BOARD_ID_abc123_WFO  	            "01B"
#define BOARD_ID_abc123	                    "019"
#define BOARD_ID_abc123_1                    "01901"
#define BOARD_ID_abc123_2                    "01902"
#define BOARD_ID_abc123_2_1                  "01903"
#define BOARD_ID_abc123_3                    "01904"
#define BOARD_ID_abc123_3_512M               "01905"
#define BOARD_ID_abc123_3_256M_4P35BAT       "01906"
#define BOARD_ID_abc123_3_512M_4P35BAT       "01907"
#define BOARD_ID_abc123_4_256M_CFG_C         "01908"
#define BOARD_ID_abc123_4_512M_CFG_B         "01909"
#define BOARD_ID_abc123_4_1G_CFG_A           "01910"
#define BOARD_ID_abc123_4_512M_CFG_D         "01911"
#define BOARD_ID_abc123_5                    "01911"
#define BOARD_ID_abc123_abc123              "025"
#define BOARD_ID_abc123_abc123_P5           "02508"
#define BOARD_ID_abc123_abc123_EVT1_2       "02510"
#define BOARD_ID_abc123_abc123_EVT1_3       "02511"
#define BOARD_ID_abc123_abc123_EVT2_0       "02512"
#define BOARD_ID_abc123_abc123_EVT3         "02514"
#define BOARD_ID_abc123_WFO_abc123          "026"
#define BOARD_ID_abc123_WFO_abc123_P5       "02608"
#define BOARD_ID_abc123_WFO_abc123_EVT1_2   "02610"
#define BOARD_ID_abc123_WFO_abc123_EVT1_3   "02611"
#define BOARD_ID_abc123_WFO_abc123_EVT2_0   "02612"
#define BOARD_ID_abc123_WFO_abc123_EVT3     "02614"
#define BOARD_ID_abc123_abc123_512          "047"
#define BOARD_ID_abc123_WFO_abc123_512      "048"
#define BOARD_ID_abc123_abc123_512_EVT3     "04702"
#define BOARD_ID_abc123_WFO_abc123_512_EVT3 "04802"
#define BOARD_ID_abc123_abc123_512_EVT4     "04703"
#define BOARD_ID_abc123_WFO_abc123_512_EVT4 "04803"
#define BOARD_ID_abc123_PRQ_NO_LSW         "04750"
#define BOARD_ID_abc123_WFO_PRQ_NO_LSW     "04850"
#define BOARD_ID_abc123_WFO                  "027"
#define BOARD_ID_abc123_WFO_EVT1             "02709"
#define BOARD_ID_abc123_WFO_EVT1_2           "02716"
#define BOARD_ID_abc123                      "02A"
#define BOARD_ID_abc123_EVT1_2               "02A16"
#define BOARD_ID_abc123_WFO_2GB              "02E"
#define BOARD_ID_abc123_WFO_2GB_EVT1         "02E09"
#define BOARD_ID_abc123_WFO_2GB_EVT1_2       "02E16"
#define BOARD_ID_abc123_2GB                  "02F"
#define BOARD_ID_abc123_2GB_EVT1_2           "02F16"
#define BOARD_ID_abc123_WFO                "051"
/* abc123 PreBuild EVT2 has new TT - abc123-170*/
#define BOARD_ID_abc123_WFO_PREEVT2        "062"
#define BOARD_ID_abc123_WFO_EVT1           "05102"

#define BOARD_ID_abc123_WFO                 "067"
#define BOARD_ID_abc123_WAN                 "068"
#define BOARD_ID_abc123_WFO                 "079"
#define BOARD_ID_abc123_WFO_HVT1            "07902"
#define BOARD_ID_abc123_WAN                 "078"
#define BOARD_ID_abc123_WAN_HVT1            "07802"
#define BOARD_ID_abc123_WAN_EVT1            "07803"
#define BOARD_ID_abc123_WFO_EVT1            "07903"
#define BOARD_ID_abc123_WAN_EVT1_1          "07804"
#define BOARD_ID_abc123_WFO_EVT1_1          "07904"
#define BOARD_ID_abc123_WAN_DVT1            "07805"
#define BOARD_ID_abc123_WFO_DVT1            "07905"
#define BOARD_ID_abc123_WAN_DVT1_1          "07806"
#define BOARD_ID_abc123_WFO_DVT1_1          "07906"
#define BOARD_ID_abc123_WAN_DVT1_1_REV_C    "07807"     /* PMIC - REV-C */
#define BOARD_ID_abc123_WFO_DVT1_1_REV_C    "07907"     /* PMIC - REV-C */

#define BOARD_ID_abc123                      "07F"
#define BOARD_ID_abc123_2                    "07F02"

#define BOARD_ID_abc123_WFO_TOUCH_LS        "06704"
#define BOARD_ID_abc123_WAN_TOUCH_LS        "06804"

#define BOARD_ID_HEISENBERG                 "118"
#define BOARD_ID_EANAB_WFO                  "118"
#define BOARD_ID_EANAB_WFO_PROTO            "11804"

#define BOARD_IS_abc123(id)             BOARD_IS_((id), BOARD_ID_abc123,         BOARD_ID_N)
#define BOARD_IS_abc123_5(id)           BOARD_IS_((id), BOARD_ID_abc123_5,       BOARD_ID_REV_N)
#define BOARD_IS_abc123(id)           BOARD_IS_((id), BOARD_ID_abc123,       BOARD_ID_N)
#define BOARD_IS_abc123_WFO(id)       BOARD_IS_((id), BOARD_ID_abc123_WFO,   BOARD_ID_N)
#define BOARD_IS_abc123_PROTO(id)     BOARD_IS_((id), BOARD_ID_abc123_PROTO, BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_PROTO(id) BOARD_IS_((id), BOARD_ID_abc123_WFO_PROTO, BOARD_ID_REV_N)
#define BOARD_IS_abc123(id)              BOARD_IS_((id), BOARD_ID_abc123,          BOARD_ID_N)
#define BOARD_IS_abc123(id)           BOARD_IS_((id), BOARD_ID_abc123,       BOARD_ID_N)
#define BOARD_IS_abc123(id)            BOARD_IS_((id), BOARD_ID_abc123,        BOARD_ID_N)
#define BOARD_IS_abc123_EVT1(id)       BOARD_IS_((id), BOARD_ID_abc123_EVT1,   BOARD_ID_REV_N)
#define BOARD_IS_abc123_EVT2(id)      BOARD_IS_((id), BOARD_ID_abc123_EVT2,  BOARD_ID_REV_N)
#define BOARD_IS_abc123_EVT2(id)      BOARD_IS_((id), BOARD_ID_abc123_EVT2,  BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_EVT2(id)  BOARD_IS_((id), BOARD_ID_abc123_WFO_EVT2,  BOARD_ID_REV_N)

#define BOARD_IS_abc123ME(id)           BOARD_IS_((id), BOARD_ID_abc123ME,         BOARD_ID_N)
#define BOARD_IS_abc123ME_1(id)         BOARD_IS_((id), BOARD_ID_abc123ME_1,       BOARD_ID_REV_N)
#define BOARD_IS_abc123ME_3(id)         BOARD_IS_((id), BOARD_ID_abc123ME_3,       BOARD_ID_REV_N)

#define BOARD_IS_abc123(id)           ((BOARD_IS_((id), BOARD_ID_abc123_512,         BOARD_ID_N)) \
					|| (BOARD_IS_((id), BOARD_ID_abc123_256,         BOARD_ID_N)))

#define BOARD_IS_abc123_WFO(id)       ((BOARD_IS_((id), BOARD_ID_abc123_WFO_256,     BOARD_ID_N)) \
					|| (BOARD_IS_((id), BOARD_ID_abc123_WFO_512,     BOARD_ID_N)))

#define BOARD_IS_abc123_EVT1_1(id)    BOARD_IS_((id), BOARD_ID_abc123_EVT1_1,  BOARD_ID_REV_N)
#define BOARD_IS_abc123_EVT1_2(id)    BOARD_IS_((id), BOARD_ID_abc123_EVT1_2,  BOARD_ID_REV_N)
#define BOARD_IS_abc123_EVT2(id)      BOARD_IS_((id), BOARD_ID_abc123_EVT2,    BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_EVT1_1(id) BOARD_IS_((id), BOARD_ID_abc123_WFO_EVT1_1, BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_EVT1_2(id) BOARD_IS_((id), BOARD_ID_abc123_WFO_EVT1_2, BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_EVT2(id)   BOARD_IS_((id), BOARD_ID_abc123_WFO_EVT2,   BOARD_ID_REV_N)
#define BOARD_IS_abc123_512(id)       BOARD_IS_((id), BOARD_ID_abc123_512, BOARD_ID_N)
#define BOARD_IS_abc123_256(id)       BOARD_IS_((id), BOARD_ID_abc123_256, BOARD_ID_N)
#define BOARD_IS_abc123_WFO_512(id)   BOARD_IS_((id), BOARD_ID_abc123_WFO_512, BOARD_ID_N)
#define BOARD_IS_abc123_WFO_256(id)   BOARD_IS_((id), BOARD_ID_abc123_WFO_256, BOARD_ID_N)
#define BOARD_IS_abc123(id)           BOARD_IS_((id), BOARD_ID_abc123,         BOARD_ID_N)
#define BOARD_IS_abc123_WFO(id)       BOARD_IS_((id), BOARD_ID_abc123_WFO,     BOARD_ID_N)

#define BOARD_IS_abc123_WFO(id)        BOARD_IS_((id), BOARD_ID_abc123_WFO,     BOARD_ID_N)
#define BOARD_IS_abc123_WAN(id)        BOARD_IS_((id), BOARD_ID_abc123_WAN,         BOARD_ID_N)

#define BOARD_IS_abc123_WFO(id)        BOARD_IS_((id), BOARD_ID_abc123_WFO,     BOARD_ID_N)
#define BOARD_IS_abc123_WAN(id)        BOARD_IS_((id), BOARD_ID_abc123_WAN,     BOARD_ID_N)

#define BOARD_IS_abc123(id)		       BOARD_IS_((id), BOARD_ID_abc123,       BOARD_ID_N)

#define BOARD_IS_abc123(id)		          BOARD_IS_((id), BOARD_ID_abc123,         BOARD_ID_N)
#define BOARD_IS_abc123_4_1G_CFG_A(id)	  BOARD_IS_((id), BOARD_ID_abc123_4_1G_CFG_A, BOARD_ID_REV_N)
#define BOARD_IS_abc123_4_512M_CFG_B(id)	  BOARD_IS_((id), BOARD_ID_abc123_4_512M_CFG_B, BOARD_ID_REV_N)
#define BOARD_IS_abc123_4_256M_CFG_C(id)   BOARD_IS_((id), BOARD_ID_abc123_4_256M_CFG_C, BOARD_ID_REV_N)
#define BOARD_IS_abc123_5(id)              BOARD_IS_((id), BOARD_ID_abc123_5,         BOARD_ID_REV_N)

#define BOARD_IS_abc123_abc123(id)	 ( BOARD_IS_((id), BOARD_ID_abc123_abc123, BOARD_ID_N) \
					|| BOARD_IS_((id), BOARD_ID_abc123_abc123_512, BOARD_ID_N))
#define BOARD_IS_abc123_abc123_EVT1_2(id) BOARD_IS_((id), BOARD_ID_abc123_abc123_EVT1_2, BOARD_ID_REV_N)
#define BOARD_IS_abc123_WFO_abc123(id)	 ( BOARD_IS_((id), BOARD_ID_abc123_WFO_abc123, BOARD_ID_N) \
					|| BOARD_IS_((id), BOARD_ID_abc123_WFO_abc123_512, BOARD_ID_N))
#define BOARD_IS_abc123(id)		        ( BOARD_IS_((id), BOARD_ID_abc123,         BOARD_ID_N) \
                                        || BOARD_IS_((id), BOARD_ID_abc123_2GB,         BOARD_ID_N))
#define BOARD_IS_abc123_WFO(id)		    (  BOARD_IS_((id), BOARD_ID_abc123_WFO,     BOARD_ID_N) \
                                        || BOARD_IS_((id), BOARD_ID_abc123_WFO_2GB,         BOARD_ID_N))
#define BOARD_IS_abc123(id)           ( BOARD_IS_((id), BOARD_ID_abc123_WFO, BOARD_ID_N) \
					|| BOARD_IS_((id), BOARD_ID_abc123_WFO_PREEVT2, BOARD_ID_N) )
/* Check for PreEVT2's only! */
#define BOARD_IS_abc123_PREEVT2(id)   ( BOARD_IS_((id), BOARD_ID_abc123_WFO_PREEVT2, BOARD_ID_N) )
#define BOARD_IS_EANAB_WFO(id)        BOARD_IS_((id), BOARD_ID_EANAB_WFO,    BOARD_ID_N)
#define BOARD_IS_HEISENBERG(id)       BOARD_IS_((id), BOARD_ID_HEISENBERG,   BOARD_ID_N)
#define BOARD_IS_EANAB_WFO_PROTO(id)     BOARD_IS_((id), BOARD_ID_EANAB_WFO_PROTO,  BOARD_ID_REV_N)

#define PLATFORM_IS_abc123(id)          ( BOARD_IS_abc123(id) \
                                       || BOARD_IS_abc123(id) \
                                       || BOARD_IS_abc123(id) \
                                       || BOARD_IS_abc123(id) \
                                       || BOARD_IS_abc123_WFO(id) )

#define PLATFORM_IS_abc123ME(id)        ( BOARD_IS_abc123ME_1(id) \
                                       || (BOARD_IS_abc123(id) && !BOARD_REV_GREATER_EQ(id, BOARD_ID_abc123_EVT1_2)) \
                                       || (BOARD_IS_abc123_WFO(id) && !BOARD_REV_GREATER_EQ(id, BOARD_ID_abc123_WFO_EVT1_2)))

#define PLATFORM_IS_abc123ME_3(id)      ( BOARD_IS_abc123ME_3(id) \
                                       || (BOARD_REV_GREATER_EQ(id, BOARD_ID_abc123_EVT1_2)) \
                                       || (BOARD_REV_GREATER_EQ(id, BOARD_ID_abc123_WFO_EVT1_2)) \
				       || (BOARD_IS_abc123(id)) || (BOARD_IS_abc123_WFO(id)) \
				       || (BOARD_IS_abc123_256(id)) || (BOARD_IS_abc123_WFO_512(id)))

#define PLATFORM_IS_abc123(id)			( BOARD_IS_abc123(id) || BOARD_IS_abc123_abc123(id) || \
											BOARD_IS_abc123_WFO_abc123(id) || \
											BOARD_IS_abc123(id) || BOARD_IS_abc123_WFO(id) || BOARD_IS_abc123(id) || \
											BOARD_IS_abc123_WAN(id) || BOARD_IS_abc123_WFO(id) ) 

#define PLATFORM_IS_DUET(id)			( BOARD_IS_abc123(id) || BOARD_IS_abc123_WAN(id) || BOARD_IS_abc123_WFO(id) )
#define PLATFORM_IS_HEISENBERG(id)                      ( BOARD_IS_HEISENBERG(id) || BOARD_IS_EANAB_WFO(id) )
#endif
