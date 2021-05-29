#pragma once

#include <stdint.h>
#include <assert.h>

enum dmp_shader_type {
	DMP_SHADER_VERTEX   = 0,
	DMP_SHADER_GEOMETRY = 1,
};

enum dmp_gsh_mode {
	DMP_GSH_POINT         = 0,
	DMP_GSH_VARIABLE_PRIM = 1,
	DMP_GSH_FIXED_PRIM    = 2,
};

#pragma pack(push, 1)

struct constant_table_ent {
	uint16_t type;
	uint16_t id;
	uint32_t data[4];
};

static_assert(sizeof(struct constant_table_ent)==0x14, "Invalid constant table entry size.");

struct label_table_ent {
	uint16_t id;
	uint16_t pad;
	uint32_t progOffset;
	uint32_t unk;
	uint32_t symbolOffset;
};

static_assert(sizeof(struct label_table_ent)==0x10, "Invalid label table entry size.");

struct output_table_ent {
	uint16_t type;
	uint16_t oid;
	uint8_t mask;
	uint8_t unk[3];
};

static_assert(sizeof(struct output_table_ent)==0x8, "Invalid output table entry size.");

struct uniform_table_ent {
	uint32_t symbolOffset;
	uint16_t startReg;
	uint16_t endReg;
};

static_assert(sizeof(struct uniform_table_ent)==0x8, "Invalid uniform table entry size.");

#pragma pack(pop)

struct dmp_pica_exe {
	uint16_t version;
	enum dmp_shader_type type;
	bool mergeOutmaps;
	uint32_t mainOffset;
	uint32_t endmainOffset;
	uint16_t inMap;
	uint16_t outMap;
	enum dmp_gsh_mode gshMode;
	uint8_t gshFixedVtxStart;
	uint8_t gshVariableVtxNum;
	uint8_t gshFixedVtxNum;

	uint32_t constTableSize;
	const struct constant_table_ent *constTableData;
	uint32_t labelTableSize;
	const struct label_table_ent *labelTableData;
	uint32_t outTableSize;
	const struct output_table_ent *outTableData;
	uint32_t uniformTableSize;
	const struct uniform_table_ent *uniformTableData;
	uint32_t symbTableSize;
	const char *symbTableData;
};

typedef struct dmp_pica_info {
	int numDVLE;
	struct dmp_pica_prog {
		uint32_t version;
		uint32_t codeSize;
		const uint32_t *codeData;
		uint32_t opdescSize;
		uint32_t *opdescData;
		uint32_t unkSize;
		const uint32_t *unkData;
		uint32_t fnsymbSize;
		const char *fnsymbData;
	} prog;
	struct dmp_pica_exe *exe;
	bool sucess;
} dmp_pica_info;

dmp_pica_info picaParseHeader(const char *bin);
int picaDisass(dmp_pica_info *pinfo, int dvleIndex);
void picaFinish(dmp_pica_info *pinfo);
