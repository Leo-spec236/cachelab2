// student id: 2023200419
// please change the above line to your student id


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#define MAX_CACHE_SIZE 1024
#define MAX_SETS (MAX_CACHE_SIZE / 64)  // 假设最大缓存大小为1024字节，每块64字节

typedef struct {
    int valid;
    int tag;
    int last_used;
} CacheLine;

typedef struct {
    int num_sets;
    int num_lines_per_set;
    int block_size;
    CacheLine* cache;
    int next_to_replace;
    int hits;
    int misses;
    int evictions;
} Cache;

void printSummary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

void printHelp(const char* name) {
    printf(
        "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
        "Options:\n"
        "  -h         Print this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n\n"
        "Examples:\n"
        "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
        "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
        name, name, name);
}

void initCache(Cache* cache, int num_sets, int num_lines_per_set, int block_size) {
    cache->num_sets = num_sets;
    cache->num_lines_per_set = num_lines_per_set;
    cache->block_size = block_size;
    cache->cache = (CacheLine*)malloc(num_sets * num_lines_per_set * sizeof(CacheLine));
    for (int i = 0; i < num_sets * num_lines_per_set; i++) {
        cache->cache[i].valid = 0;
        cache->cache[i].tag = 0;
        cache->cache[i].last_used = 0;
    }
    cache->next_to_replace = 0;
    cache->hits = 0;
    cache->misses = 0;
    cache->evictions = 0;
}

void freeCache(Cache* cache) {
    free(cache->cache);
}

void accessCache(Cache* cache, int address) {
    int num_sets = cache->num_sets;
    int num_lines_per_set = cache->num_lines_per_set;
    int block_size = cache->block_size;

    int tag = address >> (num_sets + block_size);
    int set_index = (address >> block_size) & ((1 << num_sets) - 1);
    int block_offset = address & ((1 << block_size) - 1);

    int hit = 0;
    int evict = 0;

    for (int i = 0; i < num_lines_per_set; i++) {
        int index = set_index * num_lines_per_set + i;
        if (cache->cache[index].valid && cache->cache[index].tag == tag) {
            hit = 1;
            cache->cache[index].last_used = cache->next_to_replace;
            cache->next_to_replace = (cache->next_to_replace + 1) % (num_sets * num_lines_per_set);
            break;
        }
    }

    if (!hit) {
        cache->misses++;
        int replace_index = cache->next_to_replace;
        cache->next_to_replace = (cache->next_to_replace + 1) % (num_sets * num_lines_per_set);

        for (int i = 0; i < num_lines_per_set; i++) {
            int index = set_index * num_lines_per_set + i;
            if (!cache->cache[index].valid) {
                replace_index = index;
                break;
            }
        }

        if (cache->cache[replace_index].valid) {
            evict = 1;
        }

        cache->cache[replace_index].valid = 1;
        cache->cache[replace_index].tag = tag;
        cache->cache[replace_index].last_used = cache->next_to_replace - 1;
        cache->next_to_replace = (cache->next_to_replace + 1) % (num_sets * num_lines_per_set);

        if (evict) {
            cache->evictions++;
        }
    } else {
        cache->hits++;
    }
}

int main(int argc, char* argv[]) {
    int opt;
    int s = 0, E = 0, b = 0;
    char* trace_file = NULL;
    int verbose = 0;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printHelp(argv[0]);
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                printHelp(argv[0]);
                return 1;
        }
    }

    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printHelp(argv[0]);
        return 1;
    }

    Cache cache;
    initCache(&cache, s, E, b);

    FILE* trace_fp = fopen(trace_file, "r");
    if (trace_fp == NULL) {
        perror("fopen");
        freeCache(&cache);
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), trace_fp)) {
        char operation;
        int address;
        sscanf(line, " %c %x", &operation, &address);
        if (operation == 'L' || operation == 'S') {
            accessCache(&cache, address);
        }
    }

    fclose(trace_fp);
    freeCache(&cache);

    printSummary(cache.hits, cache.misses, cache.evictions);

    return 0;
}
