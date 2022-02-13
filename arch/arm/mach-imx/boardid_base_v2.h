/*
 * boardid_base.h
 *
 * Copyright 2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/*
	PSN/DSN format: VPPCCCUUYWWD####
		
	V  Version Code, Base 32 digit used to designate the version of the SN format.  
		0-F ? indicates version #1 (legacy) DSN format (see Appendix 1)
		or version #1 (legacy) PSN format (see Appendix 2)
		G ? indicates version #2 DSN format
		H,J-N ? reserved
		P ? indicates version #2 PSN format
		Q-X ? reserved
		Z ? indicates version #1 DSN format (see Appendix 1). Z was reserved for mass-generated test DSNs to designate systems working with test accounts vs.
				 real customer devices.  Z is no longer a valid version code since it is not a valid Base 32 character.
	PP  Plant Code, two-digit Base 32 code to designate the Manufacture site.
		05  This value used to designate a proto house or unqualified manufacturer
		XX  This value is reserved for test or unofficial DSNs/PSNs
		*** - Plant codes for individual plants are defined in 571-1000-00-AD 	
	CCC  Unique Code
		For DSN  Device Code, three-digit Base 32 code to uniquely identify a product device model number.  
			Every device configuration as defined by a 52-xxx part number in Agile will be assigned a unique Device Code in a strict one-to-one relationship. 
		For PSN  Tattoo Code, three-digit Base 32 code to uniquely identify a PCBA design.  
			Every PCBA as defined by a 31-xxx part number in Agile (with Part Type PCBAand Function LOGIC) will be assigned a unique Tattoo Code in a strict one-to-one relationship.
	UU  Unit Assembly Revision. 
		Use the two-digit revision of the Device or PCBA part number (52-xxxxxx Rev UU for DSN; 31-xxxxxxx Rev UU for PSN).

*/


#ifndef __BOARDID_BASE_V2_H__
#define __BOARDID_BASE_V2_H__

/**
 * For device eariler Heisenberg platform, we used first 3 digits to identify device type.
 * From Heisenberg Platform we will use digits 4,5 and 6 to identify device type.
 */

#define BOARD_ID_INDEX			    3 //CCC start index

#endif
