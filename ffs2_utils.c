#include <ffs2_utils.h>

int main(int argc, char* argv[]){
	if(argc < 2){
		printf("Error: Usage ./ffs2_utils.o <mkfs,copyfile> <disk file>\n");
		exit(1);
	}
	if(strcmp(argv[1], "mkfs") == 0){
		if(argc != 4){
			printf("Error: Usage ./ffs2_utils.o mkfs <disk file> <file length in KiB>\n");
			exit(1);
		}
		uint64_t length = strtol(argv[3], 0x0, 10)*1024;
		FILE *fp;
		fp = fopen(argv[2],"wb+");
		ffs2_header_t* header = (ffs2_header_t*) malloc(sizeof(ffs2_header_t));
		memset(header,0,sizeof(ffs2_header_t));
		strcpy(header->sig, "FFS2");
		header->flags = 1; // Root filesystem flag enabled
		header->num_sectors = length/512;
		header->chain_block_start = 4; // Start at sector #4, right after header
		uint64_t num_chain_blocks = (header->num_sectors / 32) + (header->num_sectors % 32 > 0 ? 1 : 0);
		header->first_data_sector = header->chain_block_start + num_chain_blocks;
		
		ffs2_node_t* root_dir = (ffs2_node_t*) malloc(sizeof(ffs2_node_t));
		memset(root_dir,0,sizeof(ffs2_node_t));
		strcpy(root_dir->name, "/root/");
		root_dir->start_sector = header->first_data_sector;
		
		header->root_directory = *root_dir;
		free(root_dir);
		
		ffs2_chain_block_t* chain_blocks = (ffs2_chain_block_t*) malloc(sizeof(ffs2_chain_block_t)*num_chain_blocks);
		memset(chain_blocks,0,sizeof(ffs2_chain_block_t)*num_chain_blocks);
		for(uint64_t i = 0; i < header->chain_block_start+num_chain_blocks; i++){
			uint64_t index = i / 32;
			int pos = i % 32;
			chain_blocks[index].entries[pos] = 0xFFFFFFFFFFFFFFFE;
		}
		uint64_t index = header->first_data_sector / 32;
		int pos = header->first_data_sector % 32;
		chain_blocks[index].entries[pos] = 0xFFFFFFFFFFFFFFFF;
		
		char* zero = (char*) malloc(512);
		memset(zero,0,512);
		for(int i = 0; i < 3; i++){
			fwrite(zero, 1, 512, fp);
		}
		fwrite(header, sizeof(ffs2_header_t),1,fp);
		for(uint64_t i = 0; i < num_chain_blocks; i++){
			fwrite(&chain_blocks[i],sizeof(ffs2_chain_block_t),1,fp);
		}
		uint64_t zero_sectors = header->num_sectors - (header->chain_block_start+num_chain_blocks);
		for(uint64_t i = 0; i < zero_sectors; i++){
			fwrite(zero, 512, 1, fp);
		}
		fclose(fp);
	}else if(strcmp(argv[1], "copyfile") == 0){
		
	}else{
		printf("Error: Usage ./ffs2_utils.o <mkfs,copyfile> <disk file>\n");
		exit(1);
	}
}
