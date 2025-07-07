# Implementação e Análise de Tabelas Hash

## Descrição do Projeto

Este projeto implementa e analisa o desempenho de duas estratégias de resolução de colisões em tabelas hash utilizando uma base de dados real de CEPs brasileiros:

### Objetivos Implementados:

1. **Suporte a Múltiplas Estratégias**: Implementação de hash com sondagem linear (hash simples) e hash duplo para comparação de performance.

2. **Redimensionamento Dinâmico**: Estrutura adaptativa que monitora a taxa de ocupação e redimensiona automaticamente quando necessário.

3. **Sistema de Busca por CEP**: Funcionalidade para buscar informações de cidade e estado usando os primeiros 5 dígitos do CEP como chave.

4. **Análise Comparativa de Performance**:
   - Medição de tempo de busca em diferentes níveis de ocupação (10% a 99%)
   - Comparação de overhead entre estruturas estáticas e dinâmicas
   - Avaliação do impacto do redimensionamento na performance

## Estrutura do Projeto

- `hashm.c` - Implementação principal da tabela hash
- `Lista_de_CEPs.csv` - Base de dados com ~735.000 registros de CEPs
- `ALTERACOES.md` - Documentação das modificações implementadas

## Instruções de Execução

### 1. Compilação
```bash
gcc -Wall -pg hashm.c -o hashm
```

### 2. Execução
```bash
./hashm.exe
```

### 3. Geração de Relatório de Performance
```bash
gprof hashm.exe gmon.out > relatorio.txt
```

## Experimentos Realizados

### Teste de Busca por Taxa de Ocupação
- Tabela com 6100 buckets
- Testes com ocupação variando de 10% a 99%
- 5 milhões de buscas por experimento
- Comparação entre hash simples e duplo

### Teste de Overhead de Inserção
- Comparação entre tabelas iniciais de 6100 vs 1000 buckets
- Inserção de toda a base de dados (~735k registros)
- Medição do tempo de redimensionamento

## Arquivos Gerados

- `gmon.out` - Dados de profiling do gprof
- `relatorio.txt` - Relatório detalhado de performance por função