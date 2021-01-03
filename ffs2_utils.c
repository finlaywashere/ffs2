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
		fwrite(chain_blocks, sizeof(ffs2_chain_block_t),num_chain_blocks,fp);
		uint64_t zero_sectors = header->num_sectors - (header->chain_block_start+num_chain_blocks);
		for(uint64_t i = 0; i < zero_sectors; i++){
			fwrite(zero, 512, 1, fp);
		}
		fclose(fp);
	}else if(strcmp(argv[1], "copyfile") == 0){
		if(argc != 5){
                        printf("Error: Usage ./ffs2_utils.o copyfile <disk file> <file path> <new file name>\n");
                        exit(1);
                }
                FILE *fp;
                fp = fopen(argv[2],"rb+");
		FILE *src;
		src = fopen(argv[3],"rb");
		char* name = argv[4];
		
		fseek(src, 0L, SEEK_END);
		uint64_t src_size = ftell(src);
		rewind(src);
		
		uint64_t src_sectors = src_size/512 + (src_size % 512 > 0 ? 1 : 0);
		
		fseek(fp, 3*512,SEEK_SET);
		ffs2_header_t* header = (ffs2_header_t*) malloc(sizeof(ffs2_header_t));
		fread(header,sizeof(ffs2_header_t),1,fp);
		
		uint64_t num_chain_blocks = (header->num_sectors / 32) + (header->num_sectors % 32 > 0 ? 1 : 0);
		
		ffs2_chain_block_t* chain_blocks = (ffs2_chain_block_t*) malloc(sizeof(ffs2_chain_block_t)*num_chain_blocks);
		fread(chain_blocks,sizeof(ffs2_chain_block_t),num_chain_blocks,fp);
		
		uint64_t root_data_sector = header->root_directory.start_sector;
		while(chain_blocks[root_data_sector/32].entries[root_data_sector%32] != 0xFFFFFFFFFFFFFFFF){
			root_data_sector = chain_blocks[root_data_sector/32].entries[root_data_sector%32];
		}
		ffs2_node_t* file = (ffs2_node_t*) malloc(sizeof(ffs2_node_t));
		strcpy(file->name,name);
		file->type = 1;
		file->length = src_size;
		
		uint64_t sectors[src_sectors];
		uint64_t curr = 0;
		for(uint64_t i = 0; i < src_sectors; i++){
			for(uint64_t j = curr; j < header->num_sectors; j++){
				if(chain_blocks[j/32].entries[j%32] == 0){
					curr = j+1;
					sectors[i] = j;
					break;
				}
			}
		}
		file->start_sector = sectors[0];

		uint8_t root_pos = header->root_directory.length % 512;
		
		fseek(fp,root_data_sector*512+root_pos,SEEK_SET);
		fwrite(file, sizeof(ffs2_node_t),1,fp);
		
		for(uint64_t i = 0; i < src_sectors; i++){
			uint64_t sector = sectors[i];
			if(i != src_sectors-1)
				chain_blocks[sector/32].entries[sector%32] = sectors[i+1];
			else
				chain_blocks[sector/32].entries[sector%32] = 0xFFFFFFFFFFFFFFFF;
		}
		uint8_t* buffer = (uint8_t*) malloc(512);
		for(uint64_t i = 0; i < src_sectors; i++){
			fread(buffer,512,1,src);
			fseek(fp,sectors[i]*512,SEEK_SET);
			fwrite(buffer,512,1,fp);
			memset(buffer,0,512);
		}
		fseek(fp,(header->chain_block_start)*512,SEEK_SET);
		fwrite(chain_blocks, sizeof(ffs2_chain_block_t),num_chain_blocks,fp);
		
		header->root_directory.length += sizeof(ffs2_node_t);
		fseek(fp,3*512,SEEK_SET);
		fwrite(header,sizeof(ffs2_header_t),1,fp);
		
		fclose(fp);
		fclose(src);
	}else{
		printf("Error: Usage ./ffs2_utils.o <mkfs,copyfile> <disk file>\n");
		exit(1);
	}
}
