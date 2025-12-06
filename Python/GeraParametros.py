import numpy as np
import scipy.io.wavfile as wav
import pygad
from scipy.signal import hilbert

# ==============================================================================
# 1. CLASSE DO REVERB (Sua implementação)
# ==============================================================================
class SchroederReverb:
    def __init__(self, fs=48000, c_lens=[1687, 1601, 2053, 2251], ap_lens=[225, 341]):
        self.fs = fs
        self.c_lens = c_lens
        self.ap_lens = ap_lens
        # Os ganhos serão injetados dinamicamente pelo GA

    def process(self, audio, c_gains, ap_gain):
        # Versão simplificada para velocidade no GA
        # (Removemos verificações de segurança para rodar rápido)
        out = np.zeros_like(audio)
        combs = [np.zeros(l) for l in self.c_lens]
        aps = [np.zeros(l) for l in self.ap_lens]
        idx_c = [0] * 4
        idx_ap = [0] * 2
        
        # Otimização de acesso
        cl = self.c_lens
        al = self.ap_lens

        # Input Gain fixo (>>2)
        audio_scaled = audio * 0.25

        for i, x in enumerate(audio_scaled):
            sum_combs = 0.0
            
            # Combs
            for k in range(4):
                buf_val = combs[k][idx_c[k]]
                combs[k][idx_c[k]] = x + (buf_val * c_gains[k])
                sum_combs += buf_val
                idx_c[k] = (idx_c[k] + 1) % cl[k]
            
            # AP 1
            ap1_in = sum_combs
            ap1_buf = aps[0][idx_ap[0]]
            aps[0][idx_ap[0]] = ap1_in + (ap1_buf * ap_gain)
            ap1_out = ap1_buf - (ap1_in * ap_gain)
            idx_ap[0] = (idx_ap[0] + 1) % al[0]
            
            # AP 2
            ap2_in = ap1_out
            ap2_buf = aps[1][idx_ap[1]]
            aps[1][idx_ap[1]] = ap2_in + (ap2_buf * ap_gain)
            ap2_out = ap2_buf - (ap2_in * ap_gain)
            idx_ap[1] = (idx_ap[1] + 1) % al[1]
            
            out[i] = ap2_out

        return out

# ==============================================================================
# 2. PREPARAÇÃO DOS DADOS (Carregar e Alinhar UMA VEZ)
# ==============================================================================

# Defina seus arquivos aqui
FILE_DRY = 'original.wav' 
FILE_TARGET = '11-REV-STAGEDb.wav' 

# Configurações globais para o GA acessar
FS = 48000
AUDIO_DRY_CUT = None
ENV_TARGET = None
REVERB_ENGINE = SchroederReverb()

def prepare_data():
    global AUDIO_DRY_CUT, ENV_TARGET, FS
    
    # Carregar
    fs1, dry = wav.read(FILE_DRY)
    fs2, target = wav.read(FILE_TARGET)
    
    # Mono e Float
    if len(dry.shape) > 1: dry = dry[:,0]
    if len(target.shape) > 1: target = target[:,0]
    
    dry = dry.astype(np.float32) / np.max(np.abs(dry))
    target = target.astype(np.float32) / np.max(np.abs(target))
    
    # --- ALINHAMENTO ---
    # Achar picos
    idx_dry = np.argmax(np.abs(dry))
    idx_target = np.argmax(np.abs(target))
    
    # Definir região de interesse (ex: 2 segundos a partir do pico)
    duration_samples = 48000 * 2 
    
    # Cortar ambos começando do pico
    dry_cut = dry[idx_dry : idx_dry + duration_samples]
    target_cut = target[idx_target : idx_target + duration_samples]
    
    # Calcular Envelope do ALVO (Referência)
    # Suavização pesada para o GA não ficar louco com picos
    analytic = hilbert(target_cut)
    env_target = np.abs(analytic)
    # Converter para dB
    env_target_db = 20 * np.log10(env_target + 1e-6)
    
    # Salvar nas globais
    AUDIO_DRY_CUT = dry_cut
    ENV_TARGET = env_target_db
    FS = fs1
    
    print("Dados preparados e alinhados!")

# ==============================================================================
# 3. FUNÇÃO DE FITNESS (O Coração do GA)
# ==============================================================================

def fitness_func(ga_instance, solution, solution_idx):
    # solution contém: [G1, G2, G3, G4, AP_GAIN]
    c_gains = solution[0:4]
    ap_gain = solution[4]
    
    # 1. Gerar Reverb com os parâmetros atuais do GA
    generated = REVERB_ENGINE.process(AUDIO_DRY_CUT, c_gains, ap_gain)
    
    # Normalizar (O GA deve focar na forma, não no volume absoluto)
    max_val = np.max(np.abs(generated))
    if max_val > 0:
        generated /= max_val
        
    # 2. Calcular Envelope do Gerado
    analytic = hilbert(generated)
    env_gen = np.abs(analytic)
    env_gen_db = 20 * np.log10(env_gen + 1e-6)
    
    # 3. Calcular Erro (MSE - Mean Squared Error)
    # Comparar apenas até -60dB (faixa audível importante)
    # Ignorar silêncio absoluto para não viciar o dado
    mask = ENV_TARGET > -60
    
    error = np.mean((ENV_TARGET[mask] - env_gen_db[mask])**2)
    
    # Fitness = Inverso do erro (Quanto menor o erro, maior o fitness)
    fitness = 1.0 / (error + 0.00001)
    return fitness

# ==============================================================================
# 4. EXECUÇÃO DO PYGAD
# ==============================================================================

if __name__ == "__main__":
    prepare_data()
    
    # Configuração dos Genes
    # Gene 0-3: Combs Gains (0.70 a 0.95)
    # Gene 4: All Pass Gain (0.5 a 0.7)
    gene_space = [
        {'low': 0.70, 'high': 0.95}, # C1 Gain
        {'low': 0.70, 'high': 0.95}, # C2 Gain
        {'low': 0.70, 'high': 0.95}, # C3 Gain
        {'low': 0.70, 'high': 0.95}, # C4 Gain
        {'low': 0.50, 'high': 0.70}  # AP Gain
    ]

    ga_instance = pygad.GA(
        num_generations=30,      # Quantas vezes ele vai evoluir (aumente se precisar)
        num_parents_mating=4,
        fitness_func=fitness_func,
        sol_per_pop=10,          # População (10 variações por vez)
        num_genes=5,
        gene_space=gene_space,
        parent_selection_type="rws",
        keep_parents=1,
        crossover_type="single_point",
        mutation_type="random",
        mutation_percent_genes=20
    )

    print("Iniciando otimização genética... (Isso pode demorar um pouco)")
    ga_instance.run()

    # Resultados
    solution, solution_fitness, solution_idx = ga_instance.best_solution()
    print("\n============================================")
    print("MELHORES PARÂMETROS ENCONTRADOS:")
    print("============================================")
    print(f"C1_GAIN (float): {solution[0]:.4f} -> Q15: {int(solution[0]*32767)}")
    print(f"C2_GAIN (float): {solution[1]:.4f} -> Q15: {int(solution[1]*32767)}")
    print(f"C3_GAIN (float): {solution[2]:.4f} -> Q15: {int(solution[2]*32767)}")
    print(f"C4_GAIN (float): {solution[3]:.4f} -> Q15: {int(solution[3]*32767)}")
    print(f"AP_GAIN (float): {solution[4]:.4f} -> Q15: {int(solution[4]*32767)}")
    print("============================================")