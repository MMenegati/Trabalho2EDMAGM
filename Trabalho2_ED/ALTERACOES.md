# Análise das Mudanças: De hash(1).c para hashm.c

## Visão Geral

Este documento analisa as principais mudanças realizadas no código original `hash(1).c` para criar o código adaptado `hashm.c`, seguindo as instruções específicas do trabalho.

## 1. Suporte a Hash Simples e Hash Duplo

### Mudanças Principais:

**Enum para Tipos de Hash:**
```c
// Adicionado em hashm.c
typedef enum {
    HASH_SIMPLES, 
    HASH_DUPLO    
} TipoHash;
```

**Estrutura thash Expandida:**
```c
// Original (hash(1).c)
typedef struct {
     uintptr_t * table;
     int size;
     int max;
     uintptr_t deleted;
     char * (*get_key)(void *);
}thash;

// Modificado (hashm.c)
typedef struct {
    uintptr_t *table;
    int size;
    int max;
    uintptr_t deleted;
    char *(*get_key)(void *);
    TipoHash tipo;              // NOVO: tipo de hash
    float taxa_ocupacao_max;    // NOVO: taxa máxima de ocupação
} thash;
```

**Nova Função de Hash Duplo:**
```c
// Adicionado em hashm.c
uint32_t hashf2(const char *str, uint32_t h) {
    for (; *str; ++str) {
        h = (h * 31) + *str;
    }
    return (h % 97) + 1;  // Garante que nunca retorne 0
}
```

**Lógica de Sondagem Modificada:**
```c
// Original: apenas sondagem linear
pos = (pos+1) % h->max;

// Modificado: sondagem baseada no tipo
int passo = (h->tipo == HASH_DUPLO) ? hashf2(key, SEED) : 1;
pos = (pos + passo) % h->max;
```

### Impacto:
- **Hash Simples**: Usa sondagem linear (passo = 1)
- **Hash Duplo**: Usa sondagem baseada em segunda função hash, reduzindo clustering

## 2. Taxa de Ocupação e Redimensionamento Dinâmico

### Mudanças Principais:

**Função de Redimensionamento:**
```c
// Completamente nova em hashm.c
void hash_redimensiona(thash *h) {
    printf("-> Redimensionando tabela de %d para %d buckets...\n", h->max, h->max * 2);
    int max_antigo = h->max;
    uintptr_t *table_antiga = h->table;

    // Duplica o tamanho
    int novo_max = h->max * 2;
    h->table = calloc(sizeof(uintptr_t), novo_max);
    
    // Reinsere todos os elementos
    h->max = novo_max;
    h->size = 0;
    for (int i = 0; i < max_antigo; i++) {
        if (table_antiga[i] != 0 && table_antiga[i] != h->deleted) {
            hash_insere(h, (void *)table_antiga[i]);
        }
    }
    free(table_antiga);
}
```

**Inserção com Verificação de Taxa:**
```c
// Original: apenas verifica se está cheio
if (h->max == (h->size+1)){
    free(bucket);
    return EXIT_FAILURE;
}

// Modificado: verifica taxa de ocupação
if (h->taxa_ocupacao_max > 0) {
    float taxa_atual = (float)h->size / h->max;
    if (taxa_atual >= h->taxa_ocupacao_max) {
        hash_redimensiona(h);
    }
}
```

**Função de Construção Modificada:**
```c
// Original
int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *) )

// Modificado
int hash_constroi(thash *h, int nbuckets, char *(*get_key)(void *), TipoHash tipo, float taxa_max)
```

### Impacto:
- **Crescimento Dinâmico**: Tabela cresce automaticamente quando necessário
- **Controle de Performance**: Taxa de ocupação configurável (padrão: 85%)
- **Flexibilidade**: Pode desabilitar redimensionamento (taxa_max = 0)

## 3. Estrutura para CEPs

### Mudanças Principais:

**Nova Estrutura de Dados:**
```c
// Substituiu a estrutura taluno
typedef struct {
    char cep[10];
    char cidade[100];
    char estado[5];
    char chave_cep[6];  // Para armazenar os 5 primeiros dígitos
} tcep;
```

**Função de Extração de Chave:**
```c
// Original: usava nome do aluno
char * get_key(void * reg){
    return (*((taluno *)reg)).nome;
}

// Modificado: usa primeiros 5 dígitos do CEP
char *get_key_cep(void *reg) {
    tcep *cep_data = (tcep *)reg;
    strncpy(cep_data->chave_cep, cep_data->cep, 5);
    cep_data->chave_cep[5] = '\0';
    return cep_data->chave_cep;
}
```

**Função de Carregamento de Dados:**
```c
// Completamente nova
int carregar_ceps(const char *nome_arquivo, tcep *ceps_vetor[]) {
    FILE *f = fopen(nome_arquivo, "r");
    // Lê arquivo CSV
    // Processa cada linha
    // Cria estruturas tcep
    // Retorna número de registros carregados
}
```

### Impacto:
- **Domínio Específico**: Adaptado para trabalhar com dados de CEP
- **Chave Otimizada**: Usa apenas 5 dígitos para busca eficiente
- **Dados Reais**: Carrega base de dados real de CEPs

## 4. Experimentos de Performance

### 4.1 Teste de Busca por Taxa de Ocupação

**Funções Específicas Criadas:**
```c
// 20 funções específicas para gprof
void busca10_simples() { experimento_busca(0.10f, HASH_SIMPLES); }
void busca20_simples() { experimento_busca(0.20f, HASH_SIMPLES); }
// ... até busca99_simples()

void busca10_duplo() { experimento_busca(0.10f, HASH_DUPLO); }
void busca20_duplo() { experimento_busca(0.20f, HASH_DUPLO); }
// ... até busca99_duplo()
```

**Função de Experimento:**
```c
void experimento_busca(float taxa, TipoHash tipo) {
    // Cria tabela com 6100 buckets
    // Insere registros até atingir taxa desejada
    // Executa 5 milhões de buscas
    // Permite medição precisa com gprof
}
```

### 4.2 Teste de Overhead de Inserção

**Funções Específicas:**
```c
void insere6100() { insere_todos(6100, HASH_DUPLO); }
void insere1000() { insere_todos(1000, HASH_DUPLO); }

void insere_todos(int buckets_iniciais, TipoHash tipo) {
    // Insere todos os ~735k registros
    // Mede tempo de redimensionamentos
    // Calcula overhead da estrutura dinâmica
}
```

### Impacto:
- **Medição Precisa**: Funções específicas para cada teste
- **Comparação Direta**: Hash simples vs duplo
- **Análise de Overhead**: Estrutura estática vs dinâmica

## 5. Outras Melhorias

### Constantes e Configurações:
```c
#define ARQUIVO_CEPS "Lista_de_CEPs.csv"
#define TOTAL_CEPS_ARQUIVO 734914
#define TAXA_REDIMENSIONAMENTO 0.85f
```

### Tratamento de Dados:
```c
// Função para remover aspas dos dados CSV
char* trim_quotes(char* str) {
    if (str == NULL || *str != '"') return str;
    char* end = str + strlen(str) - 1;
    *end = '\0';
    return str + 1;
}
```

### Demonstração Funcional:
```c
// Seção de demonstração no main()
printf("--- Demonstracao da Busca (Item 3) ---\n");
// Mostra busca real por CEP
```

## Resumo das Transformações

| Aspecto | hash(1).c | hashm.c |
|---------|-----------|---------|
| **Tipo de Hash** | Apenas linear | Simples + Duplo |
| **Redimensionamento** | Estático | Dinâmico |
| **Domínio** | Genérico (alunos) | Específico (CEPs) |
| **Performance** | Testes básicos | Experimentos completos |
| **Dados** | Hardcoded | Arquivo CSV real |
| **Medição** | Assertions | gprof integration |

## Conclusão

As modificações transformaram um código educacional simples em uma implementação robusta para análise de performance de estruturas hash, com:

1. **Flexibilidade**: Suporte a diferentes estratégias de hash
2. **Escalabilidade**: Redimensionamento automático
3. **Aplicabilidade**: Domínio real de CEPs
4. **Mensurabilidade**: Experimentos estruturados para análise

O código resultante permite análise comparativa detalhada entre hash simples e duplo, além de medir o overhead de estruturas dinâmicas vs estáticas.
