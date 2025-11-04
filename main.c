#include "stdio.h"
#include "ezdsp5502.h"
#include "ezdsp5502_mcbsp.h"
#include "aic3204_codec_init.h"

#define BUFFER_SIZE 256  // Tamanho do buffer (quanto menor, menor a latência)

Int16 left_channel, right_channel;

void main(void)
{
    printf("\n=== Teste de Loopback AIC3204 ===\n");

    // Inicializa a placa e o codec
    EZDSP5502_init();          // Inicializa sistema (clock, periféricos)
    AIC3204_codec_init();      // Configura codec via I2C
    initAic3204();             // Inicializa interface McBSP

    printf("Loopback iniciado. Fale no microfone!\n");

    while (1)
    {
        // Lê amostra estéreo (Left/Right)
        EZDSP5502_MCBSP_read(&left_channel);
        EZDSP5502_MCBSP_read(&right_channel);

        // Escreve a mesma amostra de volta (loopback)
        EZDSP5502_MCBSP_write(left_channel);
        EZDSP5502_MCBSP_write(right_channel);
    }

    disableAic3204(); // nunca será chamado, mas mantém boa prática
}
