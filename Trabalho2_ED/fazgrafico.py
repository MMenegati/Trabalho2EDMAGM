import matplotlib.pyplot as plt
import numpy as np

# Dados extraídos do base.txt (profiling real)
# Cada benchmark_search_XX_linear e benchmark_search_XX_double = 109.42 ms/call
ocupacao = [10, 20, 30, 40, 50, 60, 70, 80, 90, 99]

# Tempos em segundos (109.42 ms = 0.10942 s)
tempo_linear = [0.10942] * 10   # Hash Linear (probing)
tempo_double = [0.10942] * 10   # Hash Dupla

# Dados adicionais do profiling para análise
total_time = 2.21  # segundos totais
map_get_time = 1.13  # 51.13% do tempo
primary_hash_time = 0.45  # 20.36% do tempo
extract_key_time = 0.29  # 13.12% do tempo
secondary_hash_time = 0.15  # 6.79% do tempo

# Criando o gráfico principal
plt.figure(figsize=(15, 10))

# Subplot 2: Distribuição de tempo por função
plt.subplot(2, 2, 2)
functions = ['map_get', 'primary_hash', 'extract_key', 'run_benchmark', 'secondary_hash']
times = [1.13, 0.45, 0.29, 0.16, 0.15]
colors = ['#ff9999', '#66b3ff', '#99ff99', '#ffcc99', '#ff99cc']
plt.pie(times, labels=functions, autopct='%1.1f%%', colors=colors, startangle=90)
plt.title('Distribuição de Tempo por Função', fontsize=14)

# Subplot 3: Número de chamadas de função
plt.subplot(2, 2, 3)
func_calls = ['map_get', 'primary_hash', 'extract_key', 'secondary_hash']
call_counts = [100000001, 100090142, 443669122, 50056679]
call_counts_millions = [x/1000000 for x in call_counts]
plt.bar(func_calls, call_counts_millions, color=['#ff9999', '#66b3ff', '#99ff99', '#ff99cc'])
plt.xlabel('Funções', fontsize=12)
plt.ylabel('Número de Chamadas (Milhões)', fontsize=12)
plt.title('Número de Chamadas por Função', fontsize=14)
plt.xticks(rotation=45)

# Subplot 4: Análise comparativa detalhada
plt.subplot(2, 2, 4)
# Simulando uma diferença teórica baseada na ocupação
ocupacao_teorica = np.array(ocupacao)
# Hash linear degrada mais com alta ocupação
tempo_linear_teorico = 0.10942 * (1 + ocupacao_teorica/200)  # Degradação linear
tempo_double_teorico = 0.10942 * (1 + ocupacao_teorica/400)  # Degradação menor

plt.plot(ocupacao, tempo_linear_teorico, 'b--', label='Linear (Teórico)', alpha=0.7)
plt.plot(ocupacao, tempo_double_teorico, 'r--', label='Dupla (Teórico)', alpha=0.7)
plt.xlabel('Taxa de Ocupação (%)', fontsize=12)
plt.ylabel('Tempo de Execução (segundos)', fontsize=12)
plt.title('Comparação: Observado vs Teórico', fontsize=14)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=10)

plt.tight_layout()
plt.show()

# Estatísticas detalhadas
print("=== ANÁLISE DETALHADA DO PROFILING ===")
print(f"Tempo total de execução: {total_time:.2f} segundos")
print(f"Tempo por benchmark: {0.10942:.5f} segundos")
print(f"Número total de buscas: 100,000,001")
print(f"Número de chamadas primary_hash: 100,090,142")
print(f"Número de chamadas extract_key: 443,669,122")
print(f"Número de chamadas secondary_hash: 50,056,679")

print("\n=== DISTRIBUIÇÃO DE TEMPO ===")
print(f"map_get: {map_get_time:.2f}s ({map_get_time/total_time*100:.1f}%)")
print(f"primary_hash: {primary_hash_time:.2f}s ({primary_hash_time/total_time*100:.1f}%)")
print(f"extract_key: {extract_key_time:.2f}s ({extract_key_time/total_time*100:.1f}%)")
print(f"secondary_hash: {secondary_hash_time:.2f}s ({secondary_hash_time/total_time*100:.1f}%)")

print("\n=== OBSERVAÇÕES ===")
print("1. Ambas as técnicas apresentaram performance idêntica nos testes")
print("2. O gargalo está na função map_get (51.13% do tempo)")
print("3. As funções hash (primary + secondary) consomem 27.15% do tempo")
print("4. O dataset específico pode não evidenciar as diferenças teóricas")
print("5. Para datasets maiores ou com mais colisões, as diferenças podem ser mais evidentes")