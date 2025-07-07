#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HASH_FUNCTION_SEED 0x12345678
#define DATA_SOURCE_FILE "ceps_exemplo.csv"
#define MAX_RECORDS_FROM_SOURCE 734914
#define RESIZE_THRESHOLD_FACTOR 0.85f
#define BENCHMARK_ITERATIONS 5000000


typedef enum {
    PROBE_LINEAR,
    PROBE_DOUBLE
} ProbingStrategy;

typedef struct {
    char zip_code[10];
    char city_name[100];
    char state_code[5];
    char search_key[6];
} PostalRecord;

typedef struct {
    uintptr_t *entries;
    int item_count;
    int capacity;
    uintptr_t deleted_marker;
    char *(*get_key_function)(void *);
    ProbingStrategy probe_strategy;
    float max_load_factor;
} HashMap;


// --- Funções de Hashing ---

uint32_t primary_hash(const char *key_string) {
    uint32_t hash = HASH_FUNCTION_SEED;
    for (; *key_string; ++key_string) {
        hash ^= *key_string;
        hash *= 0x5bd1e995;
        hash ^= hash >> 15;
    }
    return hash;
}

uint32_t secondary_hash_step(const char *key_string) {
    uint32_t hash = HASH_FUNCTION_SEED;
    for (; *key_string; ++key_string) {
        hash = (hash * 31) + *key_string;
    }
    return (hash % 97) + 1; // Retorna um passo entre 1 e 97
}


// --- Funções de Manipulação da Tabela Hash ---

void map_reinsert(HashMap* map, void* record);

void map_resize(HashMap* map) {
    printf("-> Redimensionando tabela de %d para %d...\n", map->capacity, map->capacity * 2);
    int old_capacity = map->capacity;
    uintptr_t *old_entries = map->entries;

    int new_capacity = map->capacity * 2;
    map->entries = calloc(new_capacity, sizeof(uintptr_t));
    if (map->entries == NULL) {
        printf("ERRO CRÍTICO: Falha de alocação de memória no redimensionamento.\n");
        map->entries = old_entries;
        return;
    }

    map->capacity = new_capacity;
    map->item_count = 0;

    for (int i = 0; i < old_capacity; i++) {
        if (old_entries[i] != 0 && old_entries[i] != map->deleted_marker) {
            map_reinsert(map, (void *)old_entries[i]);
        }
    }
    free(old_entries);
}

void map_put(HashMap* map, void* record) {
    if (map->max_load_factor > 0.0f) {
        float current_load_factor = (float)map->item_count / map->capacity;
        if (current_load_factor >= map->max_load_factor) {
            map_resize(map);
        }
    }
    map_reinsert(map, record);
}

void map_reinsert(HashMap* map, void* record) {
    if (map->item_count >= map->capacity - 1) {
        return;
    }

    char *key = map->get_key_function(record);
    uint32_t hash_value = primary_hash(key);
    int index = hash_value % map->capacity;
    int step = (map->probe_strategy == PROBE_DOUBLE) ? secondary_hash_step(key) : 1;

    while (map->entries[index] != 0) {
        if (map->entries[index] == map->deleted_marker) break;
        index = (index + step) % map->capacity;
    }
    map->entries[index] = (uintptr_t)record;
    map->item_count++;
}

void* map_get(HashMap map, const char* key) {
    uint32_t hash_value = primary_hash(key);
    int initial_index = hash_value % map.capacity;
    int index = initial_index;
    int step = (map.probe_strategy == PROBE_DOUBLE) ? secondary_hash_step(key) : 1;

    while (map.entries[index] != 0) {
        if (map.entries[index] != map.deleted_marker) {
            if (strcmp(map.get_key_function((void*)map.entries[index]), key) == 0) {
                return (void*)map.entries[index];
            }
        }
        index = (index + step) % map.capacity;
        if (index == initial_index) break;
    }
    return NULL;
}

void map_destroy(HashMap* map) {
    for (int i = 0; i < map->capacity; i++) {
        if (map->entries[i] != 0 && map->entries[i] != map->deleted_marker) {
            free((void*)map->entries[i]);
        }
    }
    free(map->entries);
}

void map_create(HashMap* map, int initial_capacity, char* (*key_func)(void*), ProbingStrategy strategy, float load_factor) {
    map->entries = calloc(initial_capacity, sizeof(uintptr_t));
    map->capacity = initial_capacity;
    map->item_count = 0;
    map->deleted_marker = (uintptr_t)&(map->item_count);
    map->get_key_function = key_func;
    map->probe_strategy = strategy;
    map->max_load_factor = load_factor;
}

// --- funções para os dados de CEP ---

char* extract_postal_key(void* record) {
    PostalRecord* data = (PostalRecord*)record;
    strncpy(data->search_key, data->zip_code, 5);
    data->search_key[5] = '\0';
    return data->search_key;
}

PostalRecord* create_postal_record(const char* zip, const char* city, const char* state) {
    PostalRecord* new_record = malloc(sizeof(PostalRecord));
    if (new_record) {
        strncpy(new_record->zip_code, zip, sizeof(new_record->zip_code) - 1);
        new_record->zip_code[sizeof(new_record->zip_code) - 1] = '\0';
        strncpy(new_record->city_name, city, sizeof(new_record->city_name) - 1);
        new_record->city_name[sizeof(new_record->city_name) - 1] = '\0';
        strncpy(new_record->state_code, state, sizeof(new_record->state_code) - 1);
        new_record->state_code[sizeof(new_record->state_code) - 1] = '\0';
    }
    return new_record;
}

char* sanitize_field(char* text) {
    if (!text || *text != '"') return text;
    char* end = text + strlen(text) - 1;
    *end = '\0';
    return text + 1;
}

int load_data_from_csv(const char* filename, PostalRecord* data_vector[]) {
    FILE* file_ptr = fopen(filename, "r");
    if (!file_ptr) {
        perror("Não foi possível abrir o arquivo de dados");
        return 0;
    }
    char line_buffer[512];
    int count = 0;
    fgets(line_buffer, sizeof(line_buffer), file_ptr);

    while (fgets(line_buffer, sizeof(line_buffer), file_ptr) && count < MAX_RECORDS_FROM_SOURCE) {
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;
        
        char* state_field = strtok(line_buffer, ",");
        char* city_field = strtok(NULL, ",");
        strtok(NULL, ",");
        char* zip_field = strtok(NULL, ",");
        
        if (state_field && city_field && zip_field) {
            char* clean_state = sanitize_field(state_field);
            char* clean_city = sanitize_field(city_field);
            char* clean_zip = sanitize_field(zip_field);
            data_vector[count] = create_postal_record(clean_zip, clean_city, clean_state);
            if (data_vector[count]) {
                count++;
            }
        }
    }
    fclose(file_ptr);
    return count;
}

// --- Lógica de Execução dos Experimentos ---

PostalRecord* g_loaded_records[MAX_RECORDS_FROM_SOURCE];
int g_total_records_loaded = 0;

void run_search_benchmark(float load_factor, ProbingStrategy strategy) {
    HashMap map;
    int initial_buckets = 6100;
    map_create(&map, initial_buckets, extract_postal_key, strategy, 0.0f); // Sem redimensionamento para este teste

    int items_to_insert = (int)(initial_buckets * load_factor);
    if (items_to_insert > g_total_records_loaded) items_to_insert = g_total_records_loaded;

    for (int i = 0; i < items_to_insert; i++) {
        map_put(&map, g_loaded_records[i]);
    }

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        const char* search_key = extract_postal_key(g_loaded_records[i % items_to_insert]);
        map_get(map, search_key);
    }
    
    free(map.entries);
}

void run_insertion_benchmark(int initial_buckets, ProbingStrategy strategy) {
    HashMap map;
    PostalRecord** data_copy = malloc(g_total_records_loaded * sizeof(PostalRecord*));
    for (int i=0; i < g_total_records_loaded; i++) {
        data_copy[i] = create_postal_record(g_loaded_records[i]->zip_code, g_loaded_records[i]->city_name, g_loaded_records[i]->state_code);
    }
    
    map_create(&map, initial_buckets, extract_postal_key, strategy, RESIZE_THRESHOLD_FACTOR);
    for (int i = 0; i < g_total_records_loaded; i++) {
        map_put(&map, data_copy[i]);
    }
    map_destroy(&map);
    free(data_copy);
}

void benchmark_search_10_linear() { run_search_benchmark(0.10f, PROBE_LINEAR); }
void benchmark_search_20_linear() { run_search_benchmark(0.20f, PROBE_LINEAR); }
void benchmark_search_30_linear() { run_search_benchmark(0.30f, PROBE_LINEAR); }
void benchmark_search_40_linear() { run_search_benchmark(0.40f, PROBE_LINEAR); }
void benchmark_search_50_linear() { run_search_benchmark(0.50f, PROBE_LINEAR); }
void benchmark_search_60_linear() { run_search_benchmark(0.60f, PROBE_LINEAR); }
void benchmark_search_70_linear() { run_search_benchmark(0.70f, PROBE_LINEAR); }
void benchmark_search_80_linear() { run_search_benchmark(0.80f, PROBE_LINEAR); }
void benchmark_search_90_linear() { run_search_benchmark(0.90f, PROBE_LINEAR); }
void benchmark_search_99_linear() { run_search_benchmark(0.99f, PROBE_LINEAR); }

void benchmark_search_10_double() { run_search_benchmark(0.10f, PROBE_DOUBLE); }
void benchmark_search_20_double() { run_search_benchmark(0.20f, PROBE_DOUBLE); }
void benchmark_search_30_double() { run_search_benchmark(0.30f, PROBE_DOUBLE); }
void benchmark_search_40_double() { run_search_benchmark(0.40f, PROBE_DOUBLE); }
void benchmark_search_50_double() { run_search_benchmark(0.50f, PROBE_DOUBLE); }
void benchmark_search_60_double() { run_search_benchmark(0.60f, PROBE_DOUBLE); }
void benchmark_search_70_double() { run_search_benchmark(0.70f, PROBE_DOUBLE); }
void benchmark_search_80_double() { run_search_benchmark(0.80f, PROBE_DOUBLE); }
void benchmark_search_90_double() { run_search_benchmark(0.90f, PROBE_DOUBLE); }
void benchmark_search_99_double() { run_search_benchmark(0.99f, PROBE_DOUBLE); }


void benchmark_insertion_large_table() { run_insertion_benchmark(6100, PROBE_DOUBLE); }
void benchmark_insertion_small_table() { run_insertion_benchmark(1000, PROBE_DOUBLE); }

int main() {
    printf("Carregando base de dados do arquivo '%s'...\n", DATA_SOURCE_FILE);
    g_total_records_loaded = load_data_from_csv(DATA_SOURCE_FILE, g_loaded_records);

    if (g_total_records_loaded == 0) {
        printf("\nERRO: Nenhum dado foi carregado. Verifique o arquivo '%s'.\n", DATA_SOURCE_FILE);
        return 1;
    }
    printf("%d registros carregados.\n\n", g_total_records_loaded);

    printf("--- Demonstração de Busca Funcional ---\n");
    HashMap demo_map;
    map_create(&demo_map, 100, extract_postal_key, PROBE_DOUBLE, 0.9f);
    for (int i = 0; i < 50; i++) {
        PostalRecord* record_copy = create_postal_record(g_loaded_records[i]->zip_code, g_loaded_records[i]->city_name, g_loaded_records[i]->state_code);
        map_put(&demo_map, record_copy);
    }
    
    char* example_key = extract_postal_key(g_loaded_records[25]);
    PostalRecord* result = map_get(demo_map, example_key);
    if (result) {
        printf("Busca pela chave '%s':\n  -> Encontrado: Cidade: %s, Estado: %s\n", example_key, result->city_name, result->state_code);
    }
    map_destroy(&demo_map);
    printf("-------------------------------------\n\n");
    
    printf("Iniciando bateria de testes para análise com GPROF. Este processo pode ser longo.\n");

    printf("Executando testes de busca com sondagem linear...\n");
    benchmark_search_10_linear(); benchmark_search_20_linear(); benchmark_search_30_linear();
    benchmark_search_40_linear(); benchmark_search_50_linear(); benchmark_search_60_linear();
    benchmark_search_70_linear(); benchmark_search_80_linear(); benchmark_search_90_linear();
    benchmark_search_99_linear();
    printf("...concluído.\n");

    printf("Executando testes de busca com sondagem dupla...\n");
    benchmark_search_10_double(); benchmark_search_20_double(); benchmark_search_30_double();
    benchmark_search_40_double(); benchmark_search_50_double(); benchmark_search_60_double();
    benchmark_search_70_double(); benchmark_search_80_double(); benchmark_search_90_double();
    benchmark_search_99_double();
    printf("...concluído.\n");

    printf("Executando testes de inserção...\n");
    benchmark_insertion_large_table();
    benchmark_insertion_small_table();
    printf("...concluído.\n");

    printf("\nExecução finalizada. Compile com '-pg' e use o gprof para analisar 'gmon.out'.\n");

    for (int i = 0; i < g_total_records_loaded; i++) {
        free(g_loaded_records[i]);
    }

    return 0;
}