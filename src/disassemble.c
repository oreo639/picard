#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "disassemble.h"
#include "types.h"

#define PARSE_MAJOR(_x) \
	((_x)&0xFF)

#define PARSE_MINOR(_x) \
	(((_x)>>8)&0xFF)

#define DECL_INSTR(name, fun) \
	{#name, fun}

#define DECL_INSTR_NULL \
	{NULL, 0}

#define DECL_FORMAT(x) \
	static int (x)(uint32_t instr, struct dmp_pica_prog *prog, struct dmp_pica_exe *exe)

DECL_FORMAT(format0);
DECL_FORMAT(format1);
DECL_FORMAT(format1u);
DECL_FORMAT(format1i);
DECL_FORMAT(format1c);
DECL_FORMAT(format2);
DECL_FORMAT(format3);
DECL_FORMAT(format4);
DECL_FORMAT(format5);
DECL_FORMAT(format5i);

struct {
	const char *name;
	int (* func) (uint32_t, struct dmp_pica_prog*, struct dmp_pica_exe*);
} pica_opcode_info[] = {
	DECL_INSTR(add, format1),
	DECL_INSTR(dp3, format1),
	DECL_INSTR(dp4, format1),
	DECL_INSTR(dph, format1),
	DECL_INSTR(dst, format1),
	DECL_INSTR(ex2, format1u),
	DECL_INSTR(lg2, format1u),
	DECL_INSTR(litp, format1u),
	DECL_INSTR(mul, format1),
	DECL_INSTR(sge, format1),
	DECL_INSTR(slt, format1),
	DECL_INSTR(flr, format1u),
	DECL_INSTR(max, format1),
	DECL_INSTR(min, format1),
	DECL_INSTR(rcp, format1u),
	DECL_INSTR(rsq, format1u),

	DECL_INSTR_NULL, // unk10
	DECL_INSTR_NULL, // unk11
	DECL_INSTR(mova, format1u),
	DECL_INSTR(mov, format1u),
	DECL_INSTR_NULL, // unk14
	DECL_INSTR_NULL, // unk15
	DECL_INSTR_NULL, // unk16
	DECL_INSTR_NULL, // unk17
	DECL_INSTR(dphi, format1i),
	DECL_INSTR(dsti, format1i),
	DECL_INSTR(sgei, format1i),
	DECL_INSTR(slti, format1i),
	DECL_INSTR_NULL, // unk1C
	DECL_INSTR_NULL, // unk1D
	DECL_INSTR_NULL, // unk1E
	DECL_INSTR_NULL, // unk1F

	DECL_INSTR(break, format0),
	DECL_INSTR(nop, format0),
	DECL_INSTR(end, format0),
	DECL_INSTR(breakc, format2),
	DECL_INSTR(call, format2),
	DECL_INSTR(callc, format2),
	DECL_INSTR(callu, format3),
	DECL_INSTR(ifu, format3),
	DECL_INSTR(ifc, format2),
	DECL_INSTR(for, format3),
	DECL_INSTR(emit, format0),
	DECL_INSTR(setemit, format4),
	DECL_INSTR(jmpc, format2),
	DECL_INSTR(jmpu, format3),
	DECL_INSTR(cmp, format1c),
	DECL_INSTR(cmp, format1c),

	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(madi, format5i),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
	DECL_INSTR(mad, format5),
};

enum {
	OPDESC_SWI_DST = 0,
	OPDESC_SWI_SRC1,
	OPDESC_NEG_SRC1,
	OPDESC_SWI_SRC2,
	OPDESC_NEG_SRC2,
	OPDESC_SWI_SRC3,
	OPDESC_NEG_SRC3,
};

union stropdesc {char str[5];};

static union stropdesc getStrOpdesc(unsigned mask, int type) {
	union stropdesc out = {0};
	if (type == OPDESC_SWI_DST) {
		mask = mask&0xF; //dst swizle mask
		for (int i = 0; i < 4; ++i) {
			if (!(mask & (0x8 >> i)))
				out.str[i] = '_';
			else
				out.str[i] = "xyzw"[i];
		}
	} else if (type == OPDESC_SWI_SRC1) {
		mask = mask>>5&0xFF; //src1 swizle selector
		unsigned swisel[4] = {
			[0] = mask>>6&0x3,
			[1] = mask>>4&0x3,
			[2] = mask>>2&0x3,
			[3] = mask&0x3,
		};
		for (int i = 0; i < 4; i++)
			out.str[i] = "xyzw"[swisel[i]];
	} else if (type == OPDESC_SWI_SRC2) {
		mask = mask>>14&0xFF; //src2 swizle selector
		unsigned swisel[4] = {
			[0] = mask>>6&0x3,
			[1] = mask>>4&0x3,
			[2] = mask>>2&0x3,
			[3] = mask&0x3,
		};
		for (int i = 0; i < 4; i++)
			out.str[i] = "xyzw"[swisel[i]];
	} else if (type == OPDESC_SWI_SRC3) {
		mask = mask>>23&0xFF; //src3 swizle selector
		unsigned swisel[4] = {
			[0] = mask>>6&0x3,
			[1] = mask>>4&0x3,
			[2] = mask>>2&0x3,
			[3] = mask&0x3,
		};
		for (int i = 0; i < 4; i++)
			out.str[i] = "xyzw"[swisel[i]];
	} else if (type == OPDESC_NEG_SRC1) {
		mask = mask>>4&0x1; //src1 negation bit
		if (mask)
			out.str[0] = '-';
	} else if (type == OPDESC_NEG_SRC2) {
		mask = mask>>13&0x1; //src2 negation bit
		if (mask)
			out.str[0] = '-';
	} else if (type == OPDESC_NEG_SRC3) {
		mask = mask>>22&0x1; //src3 negation bit
		if (mask)
			out.str[0] = '-';
	}
	return out;
}

union strreg {char str[7];};

static union strreg getStrDst(unsigned dst) {
	union strreg out = {0};
	if (dst <= 0x6)
		snprintf(out.str, sizeof(out.str), "o%d", dst);
	else if (dst >= 0x10 && dst <= 0x1F)
		snprintf(out.str, sizeof(out.str), "r%d", dst-0x10);
	else
		snprintf(out.str, sizeof(out.str), "err%d", dst);
	return out;
}

static union strreg getStrSrc(unsigned src) {
	union strreg out = {0};
	if (src <= 0xF)
		snprintf(out.str, sizeof(out.str), "v%d", src);
	else if (src >= 0x10 && src <= 0x1F)
		snprintf(out.str, sizeof(out.str), "r%d", src-0x10);
	else if (src >= 0x20 && src <= 0x7F)
		snprintf(out.str, sizeof(out.str), "c%d", src-0x20);
	else
		snprintf(out.str, sizeof(out.str), "err%d", src);
	return out;
}

static union strreg getStrUniformTableReg(unsigned src) {
	union strreg out = {0};
	if (src <= 0xF)
		snprintf(out.str, sizeof(out.str), "v%d", src);
	else if (src >= 0x10 && src <= 0x6F)
		snprintf(out.str, sizeof(out.str), "c%d", src-0x10);
	else if (src >= 0x70 && src <= 0x73)
		snprintf(out.str, sizeof(out.str), "i%d", src-0x70);
	else if (src >= 0x78 && src <= 0x87)
		snprintf(out.str, sizeof(out.str), "b%d", src-0x78);
	else
		snprintf(out.str, sizeof(out.str), "err%d", src);
	return out;
}

static const char *getLabel(struct dmp_pica_exe *exe, uint32_t prog_offset) {
	if (exe) {
		for (size_t j = 0; j < exe->labelTableSize; j++)
			if (prog_offset == exe->labelTableData[j].progOffset*4)
				return &exe->symbTableData[exe->labelTableData[j].symbolOffset];
	}

	return NULL;
}

const char *outTypeName[] = {
	"position",
	"normalquat",
	"color",
	"texcoord0",
	"texcoord0w",
	"texcoord1",
	"texcoord2",
	NULL,
	"view",
	"dummy",
};

static inline const char *getStrOutType(uint16_t type) {
	if (type < ARRAY_SIZE(outTypeName))
		return outTypeName[type];

	return NULL;
}

static union stropdesc getStrOutComp(unsigned mask) {
	union stropdesc out = {0};
	mask = mask&0xF;
	for (int i = 0; i < 4; ++i) {
		if (!(mask & (1 << i)))
			out.str[i] = '_';
		else
			out.str[i] = "xyzw"[i];
	}
	return out;
}

const char *cmpOpStr[] = {
	"eq",
	"ne",
	"lt",
	"le",
	"gt",
	"ge",
};

static const char *getStrCmpOp(unsigned op) {
	if (op < ARRAY_SIZE(cmpOpStr))
		return cmpOpStr[op];

	return NULL;
}

const char *addrIdxStr[] = {
	"",
	"[a0.x]",
	"[a0.y]",
	"[aL]",
};

static const char *getStrAddrIdx(unsigned idx) {
	if (idx < ARRAY_SIZE(addrIdxStr))
		return addrIdxStr[idx];

	return "[err]";
}

static float f24tof32(uint32_t f24) {
	uint32_t mantissa = f24 & 0xFFFF;
	uint32_t exponent = (f24 >> 16) & 0x7F;
	uint32_t sign = f24 >> 23;

	return pow(2.0f, (float)exponent-63.0f) * (1.0f + mantissa * pow(2.0f, -16.f)) * (sign?-1:1);
}

DECL_FORMAT(format0) {
	uint8_t opcode = instr>>26;
	printf("%02x                    ", opcode);
	printf("%s", pica_opcode_info[opcode].name);
	return 0;
}

DECL_FORMAT(format1) {
	uint8_t opcode = instr>>26;
	uint8_t dst = (instr>>21)&0x1F;
	uint8_t idx_src1 = (instr>>19)&0x3;
	uint8_t src1 = (instr>>12)&0x7F;
	uint8_t src2 = (instr>>7)&0x1F;
	uint8_t opdesc = (instr)&0x7F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x %02x     ",
		opcode,
		dst,
		idx_src1,
		src1,
		src2,
		opdesc);

	printf("%-7s %s.%s, %s%s%s.%s, %s%s.%s", pica_opcode_info[opcode].name,
		getStrDst(dst).str, getStrOpdesc(opdescData, OPDESC_SWI_DST).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrAddrIdx(idx_src1), getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrSrc(src2).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str);

	return 0;
}

DECL_FORMAT(format1i) {
	uint8_t opcode = instr>>26;
	uint8_t dst = (instr>>21)&0x1F;
	uint8_t idx_src2 = (instr>>19)&0x3;
	uint8_t src1 = (instr>>14)&0x1F;
	uint8_t src2 = (instr>>7)&0x7F;
	uint8_t opdesc = (instr)&0x7F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x %02x     ",
		opcode,
		dst,
		idx_src2,
		src1,
		src2,
		opdesc);

	printf("%-7s %s.%s, %s%s.%s, %s%s%s.%s", pica_opcode_info[opcode].name,
		getStrDst(dst).str, getStrOpdesc(opdescData, OPDESC_SWI_DST).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrSrc(src2).str, getStrAddrIdx(idx_src2), getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str);

	return 0;
}

DECL_FORMAT(format1u) {
	uint8_t opcode = instr>>26;
	uint8_t dst = (instr>>21)&0x1F;
	uint8_t idx_src1 = (instr>>19)&0x3;
	uint8_t src1 = (instr>>12)&0x7F;
	uint8_t opdesc = (instr)&0x7F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x        ",
		opcode,
		dst,
		idx_src1,
		src1,
		opdesc);

	printf("%-7s ", pica_opcode_info[opcode].name);
	if (opcode != 0x12/*MOVA*/)
		printf("%s.%s, ", getStrDst(dst).str, getStrOpdesc(opdescData, OPDESC_SWI_DST).str);
	else
		printf("a0.%s, ", getStrOpdesc(opdescData, OPDESC_SWI_DST).str);

	printf("%s%s%s.%s",
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrAddrIdx(idx_src1), getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str);

	return 0;
}

DECL_FORMAT(format1c) {
	uint8_t opcode = instr>>26;
	uint8_t cmpop_x = (instr>>24)&0x7;
	uint8_t cmpop_y = (instr>>21)&0x7;
	uint8_t idx_src1 = (instr>>19)&0x3;
	uint8_t src1 = (instr>>12)&0x7F;
	uint8_t src2 = (instr>>7)&0x1F;
	uint8_t opdesc = (instr)&0x7F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x %02x %02x  ",
		opcode,
		cmpop_x,
		cmpop_y,
		idx_src1,
		src1,
		src2,
		opdesc);

	printf("%-7s %s%s%s.%s, %s, %s, %s%s.%s", pica_opcode_info[opcode].name,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrAddrIdx(idx_src1), getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
		getStrCmpOp(cmpop_x),
		getStrCmpOp(cmpop_y),
		getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrSrc(src2).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str);

	return 0;
}

static inline void printCondOP(unsigned condop, bool refx, bool refy) {
	switch (condop) {
		case 0x0:
			printf("%scmp.x || %scmp.y", refx?"":"!", refy?"":"!");
			break;
		case 0x1:
			printf("%scmp.x && %scmp.y", refx?"":"!", refy?"":"!");
			break;
		case 0x2:
			printf("%scmp.x", refx?"":"!");
			break;
		case 0x3:
			printf("%scmp.y", refy?"":"!");
			break;
	}
}

DECL_FORMAT(format2) {
	uint8_t  opcode   = instr>>26;
	bool     refx     = (instr>>25)&0x1;
	bool     refy     = (instr>>24)&0x1;
	uint8_t  condop   = (instr>>22)&0x3;
	uint16_t dst      = (instr>>8)&0xFFF;
	uint8_t  instrnum = (instr)&0xFF;
	printf("%02x %02x %02x %02x %03x %02x    ",
		opcode,
		refx,
		refy,
		condop,
		dst,
		instrnum);

	printf("%-7s ", pica_opcode_info[opcode].name);

	switch (opcode) {
		case 0x2C: /*JMPC*/ {
			printCondOP(condop, refx, refy);
			printf(", ");

			const char *label = getLabel(exe, dst);
			if (label)
				printf("<%s>", label);
			else
				printf("0x%04x", dst);
			break;
		}
		case 0x28: /*IFC*/
		case 0x25: /*CALLC*/
			printCondOP(condop, refx, refy);
			printf(", ");
		case 0x24: /*CALL*/ {
			const char *label = getLabel(exe, dst);
			if (label)
				printf("<%s>, %d", label, instrnum);
			else
				printf("0x%04x, %d", dst, instrnum);
			break;
		}
	}
	return 0;
}

DECL_FORMAT(format3) {
	uint8_t opcode = instr>>26;
	uint8_t uniformid = (instr>>22)&0xF;
	uint16_t dst = (instr>>8)&0xFFF;
	uint8_t instrnum = (instr)&0xFF;
	printf("%02x %02x %03x %02x          ",
		opcode,
		uniformid,
		dst,
		instrnum);

	printf("%-7s ", pica_opcode_info[opcode].name);
	switch (opcode) {
		case 0x29: /*FOR*/ {
			const char *label = getLabel(exe, dst);
			if (label)
				printf("i%d, <%s>", uniformid, label);
			else
				printf("i%d, 0x%04x", uniformid, dst);
			break;
		}
		case 0x27: /*IFU*/
		case 0x26: /*CALLU*/ {
			const char *label = getLabel(exe, dst);
			if (label)
				printf("b%d, <%s>, %d", uniformid, label, instrnum);
			else
				printf("b%d, 0x%04x, %d", uniformid, dst, instrnum);
			break;
		}
		case 0x2D: /*JMPU*/ {
			const char *label = getLabel(exe, dst);
			if (label)
				printf("%sb%d, <%s>", instrnum?"":"!", uniformid, label);
			else
				printf("%sb%d, 0x%04x", instrnum?"":"!", uniformid, dst);
			break;
		}
	}
	return 0;
}

DECL_FORMAT(format4) {
	uint8_t opcode = instr>>26;
	uint8_t vtxid = (instr>>24)&0x3;
	bool primemit = (instr>>23)&0x1;
	bool winding = (instr>>22)&0x1;
	printf("%02x %02x %02x %02x           ",
		opcode,
		vtxid,
		primemit,
		winding);

	printf("%-7s ", pica_opcode_info[opcode].name);
	printf("%d", vtxid);
	if (primemit || winding)
		printf(",");
	printf("%s", primemit ? " prim" : "");
	printf("%s", winding ? " inv" : "");

	return 0;
}

DECL_FORMAT(format5) {
	uint8_t opcode = instr>>26;
	uint8_t dst = (instr>>24)&0x1F;
	uint8_t idx_src2 = (instr>>22)&0x3;
	uint8_t src1 = (instr>>11)&0x1F;
	uint8_t src2 = (instr>>10)&0x7F;
	uint8_t src3 = (instr>>5)&0x1F;
	uint8_t opdesc = (instr)&0x1F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x %02x %02x  ",
		opcode,
		dst,
		idx_src2,
		src1,
		src2,
		src3,
		opdesc);

	printf("%-7s %s.%s, %s%s.%s, %s%s%s.%s, %s%s.%s", pica_opcode_info[opcode].name,
		getStrDst(dst).str, getStrOpdesc(opdescData, OPDESC_SWI_DST).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrSrc(src2).str, getStrAddrIdx(idx_src2), getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC3).str, getStrSrc(src3).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC3).str);

	return 0;
}

DECL_FORMAT(format5i) {
	uint8_t opcode = instr>>26;
	uint8_t dst = (instr>>24)&0x1F;
	uint8_t idx_src3 = (instr>>22)&0x3;
	uint8_t src1 = (instr>>11)&0x1F;
	uint8_t src2 = (instr>>12)&0x1F;
	uint8_t src3 = (instr>>5)&0x7F;
	uint8_t opdesc = (instr)&0x1F;
	uint32_t opdescData = prog->opdescData[opdesc];
	printf("%02x %02x %02x %02x %02x %02x %02x  ",
		opcode,
		dst,
		idx_src3,
		src1,
		src2,
		src3,
		opdesc);

	printf("%-7s %s.%s, %s%s.%s, %s%s.%s, %s%s%s.%s", pica_opcode_info[opcode].name,
		getStrDst(dst).str, getStrOpdesc(opdescData, OPDESC_SWI_DST).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrSrc(src1).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrSrc(src2).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str,
		getStrOpdesc(opdescData, OPDESC_NEG_SRC3).str, getStrSrc(src3).str, getStrAddrIdx(idx_src3), getStrOpdesc(opdescData, OPDESC_SWI_SRC3).str);

	return 0;
}

static inline uint32_t getUint32(const uint32_t *data, int offset) {
	return le_u32(data[offset]);
}

dmp_pica_info picaParseHeader(const char *bin) {
	dmp_pica_info pinfo = {0};

	if (!bin)
		return pinfo;

	if (memcmp("DVLB", bin, 4)!=0) {
		error("Something went wrong\n");
	}

	const uint32_t *shbinData = (uint32_t*)bin;
	pinfo.numDVLE = getUint32(shbinData, 1);
	pinfo.exe     = malloc(sizeof(struct dmp_pica_exe)*pinfo.numDVLE);
	printf("DVLEs found: %d\n", pinfo.numDVLE);

	const uint32_t *dvlpData=&shbinData[2+pinfo.numDVLE];
	pinfo.prog.version  = getUint32(dvlpData, 1);
	pinfo.prog.codeSize = getUint32(dvlpData, 3);
	pinfo.prog.codeData = &dvlpData[getUint32(dvlpData, 2)/4];
	// Swizzle table
	pinfo.prog.opdescSize = getUint32(dvlpData, 5);
	pinfo.prog.opdescData = (uint32_t*)malloc(sizeof(uint32_t)*pinfo.prog.opdescSize);
	for(size_t i = 0; i < pinfo.prog.opdescSize; i++)
		pinfo.prog.opdescData[i] = getUint32(dvlpData, getUint32(dvlpData, 4)/4+i*2);

	pinfo.prog.unkSize = getUint32(dvlpData, 7);
	pinfo.prog.unkData = &dvlpData[getUint32(dvlpData, 6)/4];

	pinfo.prog.fnsymbSize = getUint32(dvlpData, 9);
	pinfo.prog.fnsymbData = (const char*)&dvlpData[getUint32(dvlpData, 8)/4];

	for (int i = 0; i < pinfo.numDVLE; i++) {
		struct dmp_pica_exe *exe = &pinfo.exe[i];
		const uint32_t* dvleData=&shbinData[getUint32(shbinData, 2+i)/4];

		if (memcmp("DVLE", dvleData, 4)!=0) {
			error("Something went wrong when parsing dvle\n");
			goto err0;
		}

		exe->version       = getUint32(dvleData, 1)&0xFFFF;
		exe->type          = (getUint32(dvleData, 1)>>16)&0xFF;
		exe->mergeOutmaps  = (getUint32(dvleData, 1)>>24)&1;
		exe->mainOffset    = getUint32(dvleData, 2);
		exe->endmainOffset = getUint32(dvleData, 3);
		exe->inMap         = getUint32(dvleData, 4)&0xFFFF;
		exe->outMap        = (getUint32(dvleData, 4)>>16)&0xFFFF;

		if(exe->type==DMP_SHADER_GEOMETRY)
		{
			exe->gshMode           = getUint32(dvleData, 5)&0xFF;
			exe->gshFixedVtxStart  = (getUint32(dvleData, 5)>>8)&0xFF;
			exe->gshVariableVtxNum = (getUint32(dvleData, 5)>>16)&0xFF;
			exe->gshFixedVtxNum    = (getUint32(dvleData, 5)>>24)&0xFF;
		}

		exe->constTableSize = getUint32(dvleData, 7);
		exe->constTableData = (struct constant_table_ent*)&dvleData[getUint32(dvleData, 6)/4];

		exe->labelTableSize = getUint32(dvleData, 9);
		exe->labelTableData = (struct label_table_ent*)&dvleData[getUint32(dvleData, 8)/4];

		exe->outTableSize = getUint32(dvleData, 11);
		exe->outTableData = (struct output_table_ent*)&dvleData[getUint32(dvleData, 10)/4];

		exe->uniformTableSize = getUint32(dvleData, 13);
		exe->uniformTableData = (struct uniform_table_ent*)&dvleData[getUint32(dvleData, 12)/4];

		exe->symbTableSize = getUint32(dvleData, 15);
		exe->symbTableData = (char*)&dvleData[getUint32(dvleData, 14)/4];
	}

	pinfo.sucess = true;
	goto exit;

err0:
	free(pinfo.exe);
	free(pinfo.prog.opdescData);
exit:
	return pinfo;
}

int picaDisass(dmp_pica_info *pinfo, int dvleIndex) {
	int err = 0;

	if (!pinfo->sucess)
		return -1;

	verbose("Linked against %d.%d\n", PARSE_MAJOR(pinfo->prog.version), PARSE_MINOR(pinfo->prog.version));

	for (int i = 0; i < pinfo->numDVLE; i++) {
		verbose("DVLE %d: %s\n", i, pinfo->exe[i].type == DMP_SHADER_VERTEX ? "vsh" : "gsh");
	}

	int fileNum = 0;
	for (size_t i = 0; i < pinfo->prog.fnsymbSize; i++) {
		if (i == 0 || pinfo->prog.fnsymbData[i-1] == '\0') {
			verbose("File %d: %s\n", fileNum++, &pinfo->prog.fnsymbData[i]);
		}
	}

	struct dmp_pica_exe *exe = NULL;
	if (dvleIndex < 0) {
		printf("No dvle specified!\n");
	} else if (dvleIndex >= pinfo->numDVLE) {
		printf("Invalid dvle index specified\n");
		dvleIndex = -1;
	} else {
		exe = &pinfo->exe[dvleIndex];

		verbose("; DVLE version %d.%d\n", PARSE_MAJOR(exe->version), PARSE_MINOR(exe->version));
		for (size_t j = 0; j < exe->labelTableSize; j++) {
			uint32_t progSize = exe->labelTableData[j].progSize == (uint32_t)-1 ?
				exe->labelTableData[j].progSize : exe->labelTableData[j].progSize*4;

			verbose("; Label <%s>, offset: 0x%x, size: 0x%x, id %d (unk %d)\n",
				&exe->symbTableData[exe->labelTableData[j].symbolOffset], exe->labelTableData[j].progOffset*4,
				progSize, exe->labelTableData[j].id, exe->labelTableData[j].pad);
		}

		verbose("; mergeOutmaps = %s\n", exe->mergeOutmaps?"true":"false");

		if(exe->type==DMP_SHADER_GEOMETRY) {
			switch (exe->gshMode) {
				case DMP_GSH_POINT:
					printf(".gsh point\n");
					break;
				case DMP_GSH_VARIABLE_PRIM:
					printf(".gsh variable %d\n", exe->gshVariableVtxNum);
					break;
				case DMP_GSH_FIXED_PRIM:
					printf(".gsh fixed c%d %d\n", exe->gshFixedVtxStart, exe->gshFixedVtxNum);
					break;
			};
		}


		for (size_t j = 0; j < exe->outTableSize; j++) {
			const char *typename = getStrOutType(exe->outTableData[j].type);
			if (typename) {
				printf(".out %-10s o%d.%s", typename, exe->outTableData[j].oid, getStrOutComp(exe->outTableData[j].mask).str);
				if (exe->outTableData[j].unk) {
					verbose(" (unk: 0x%x)", exe->outTableData[j].unk);
				}
				printf("\n");
			} else {
				error(".out err%d o%d.%s\n", exe->outTableData[j].type, exe->outTableData[j].oid, getStrOutComp(exe->outTableData[j].mask).str);
				err++;
			}
		}

		for (size_t j = 0; j < exe->uniformTableSize; j++) {
			if (exe->uniformTableData[j].startReg != exe->uniformTableData[j].endReg) {
				printf(".uniform %s %s-%s\n",
					&exe->symbTableData[exe->uniformTableData[j].symbolOffset],
					getStrUniformTableReg(exe->uniformTableData[j].startReg).str,
					getStrUniformTableReg(exe->uniformTableData[j].endReg).str);
			} else {
				printf(".uniform %s %s\n",
					&exe->symbTableData[exe->uniformTableData[j].symbolOffset],
					getStrUniformTableReg(exe->uniformTableData[j].startReg).str);
			}
		}

		for (size_t j = 0; j < exe->constTableSize; j++) {
			switch (exe->constTableData[j].type) {
				case 0x0:
					printf(".const b%d %s",
						exe->constTableData[j].id,
						exe->constTableData[j].data[0]&0x1?"true":"false");
					break;
				case 0x1:
					printf(".const i%d(%d, %d, %d, %d)",
						exe->constTableData[j].id,
						exe->constTableData[j].data[0]&0xFF,
						exe->constTableData[j].data[0]>>8&0xFF,
						exe->constTableData[j].data[0]>>16&0xFF,
						exe->constTableData[j].data[0]>>24&0xFF);
					break;
				case 0x2:
					printf(".const c%d(%f, %f, %f, %f)",
						exe->constTableData[j].id,
						f24tof32(exe->constTableData[j].data[0]),
						f24tof32(exe->constTableData[j].data[1]),
						f24tof32(exe->constTableData[j].data[2]),
						f24tof32(exe->constTableData[j].data[3]));
					break;
				default:
					error("Unknown constant type %d", exe->constTableData[j].type);
					err++;
			}
			printf("\n");
		}
	}

	printf("; %d program instructions in total\n", pinfo->prog.codeSize);
	for (unsigned i = 0; i < pinfo->prog.codeSize; ++i) {
		uint32_t instr = getUint32(pinfo->prog.codeData, i);

		for (size_t j = 0; exe && j < exe->labelTableSize; j++)
			if (i==exe->labelTableData[j].progOffset)
				printf("%08x <%s>:\n", i*4, &exe->symbTableData[exe->labelTableData[j].symbolOffset]);

		if (exe && i==exe->mainOffset) printf("[*]%08x: ", i*4);
		else if (exe && i==exe->endmainOffset-1) printf("[-]%08x: ", i*4);
		else printf("   %08x: ", i*4);
		uint8_t opcode = instr>>26;
		if (opcode < ARRAY_SIZE(pica_opcode_info) && pica_opcode_info[opcode].name) {
			pica_opcode_info[opcode].func(instr, &pinfo->prog, exe);
		} else {
			error("Unknown opcode 0x%02x", opcode);
			err++;
		}
		printf("\n");
	}
	for (size_t j = 0; exe && j < exe->labelTableSize; j++)
		if (pinfo->prog.codeSize==exe->labelTableData[j].progOffset)
			printf("%08x <%s>:\n", pinfo->prog.codeSize*4, &exe->symbTableData[exe->labelTableData[j].symbolOffset]);

	printf("Operand descriptors:\n");
	for (unsigned i = 0; i < pinfo->prog.opdescSize; ++i) {
		uint32_t opdescData = pinfo->prog.opdescData[i];
		printf("(%3x) %08x dst.%s %-1ssrc1.%s %-1ssrc2.%s %-1ssrc3.%s\n",
			i, opdescData,
			getStrOpdesc(opdescData, OPDESC_SWI_DST).str,
			getStrOpdesc(opdescData, OPDESC_NEG_SRC1).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC1).str,
			getStrOpdesc(opdescData, OPDESC_NEG_SRC2).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC2).str,
			getStrOpdesc(opdescData, OPDESC_NEG_SRC3).str, getStrOpdesc(opdescData, OPDESC_SWI_SRC3).str);
	}

	return err;
}

void picaFinish(dmp_pica_info *pinfo) {
	if (!pinfo->sucess)
		return;

	free(pinfo->prog.opdescData);
	free(pinfo->exe);
}
