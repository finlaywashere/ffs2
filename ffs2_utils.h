#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ffs2_node{
	char name[20];
	uint16_t type;
	uint16_t permissions;
	uint64_t inode;
	uint64_t creation_date;
	uint64_t modification_date;
	uint64_t start_sector;
	uint64_t length;
}__attribute__((packed));
typedef struct ffs2_node ffs2_node_t;

struct ffs2_header{
	char sig[4];
	uint32_t flags;
	uint64_t num_sectors;
	uint64_t chain_block_start;
	uint64_t first_data_sector;
	char reserved[416];
	ffs2_node_t root_directory;
}__attribute__((packed));
typedef struct ffs2_header ffs2_header_t;

struct ffs2_chain_block{
	uint64_t entries[32];
}__attribute__((packed));
typedef struct ffs2_chain_block ffs2_chain_block_t;
