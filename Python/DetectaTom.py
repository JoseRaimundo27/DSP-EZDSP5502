import librosa
import numpy as np

# ==============================================================================
# 🎛️ CONFIGURAÇÃO
# ==============================================================================
ARQUIVO_AUDIO = "original.wav"

def detectar_tom_medio(arquivo):
    print(f"Analisando '{arquivo}'... (Isso pode levar alguns segundos)")
    
    # 1. Carregar áudio
    # sr=None mantém a taxa de amostragem original
    y, sr = librosa.load(arquivo, sr=None)
    
    # 2. Extrair Pitch (Frequência Fundamental - F0) usando pYIN
    # fmin e fmax definem a faixa de busca (C2 a C6 cobre voz masculina e feminina)
    f0, voiced_flag, voiced_probs = librosa.pyin(y, 
                                                 fmin=librosa.note_to_hz('C2'), 
                                                 fmax=librosa.note_to_hz('C6'),
                                                 sr=sr)
    
    # 3. Filtrar apenas os momentos onde há voz (ignorar 'nan' e silêncio)
    # f0 contém 'nan' onde não há som detectado
    pitch_validos = f0[~np.isnan(f0)]
    
    if len(pitch_validos) == 0:
        print("Erro: Não foi possível detectar nenhuma nota. O áudio está mudo ou muito ruidoso?")
        return

    # 4. Calcular a Mediana
    # Usamos mediana em vez de média porque a média é muito sensível a erros de oitava
    frequencia_media = np.median(pitch_validos)
    
    # 5. Converter Hz para Nome da Nota
    # librosa.hz_to_note retorna algo como 'C#3', 'A4', etc.
    nota = librosa.hz_to_note(frequencia_media)
    
    print("\n" + "="*40)
    print(f"RESULTADO DA ANÁLISE")
    print("="*40)
    print(f"Frequência Média Detectada: {frequencia_media:.2f} Hz")
    print(f"Nota Musical Aproximada:    {nota}")
    print("="*40)
    
    return nota

if __name__ == "__main__":
    detectar_tom_medio(ARQUIVO_AUDIO)