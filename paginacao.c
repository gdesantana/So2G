#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int presente;
    int frame;
    int modificada;
    int referenciada;
    int tempo_carga;
    int ultimo_acesso;
} Pagina;

typedef struct {
    int pid;
    int tamanho;
    int num_paginas;
    Pagina *tabela_paginas;
} Processo;

typedef struct {
    int num_frames;
    int *frames;          
    int *tempo_carga;
} MemoriaFisica;

typedef struct {
    int tempo_atual;
    int tamanho_pagina;
    int tamanho_memoria_fisica;
    int num_processos;
    Processo *processos;
    MemoriaFisica memoria;
    int total_acessos;
    int page_faults;
    int algoritmo; 
} Simulador;


Simulador* inicializar_simulador(int tamanho_pagina, int tamanho_memoria_fisica) {
    Simulador *sim = (Simulador *)malloc(sizeof(Simulador));
    sim->tempo_atual = 0;
    sim->tamanho_pagina = tamanho_pagina;
    sim->tamanho_memoria_fisica = tamanho_memoria_fisica;
    sim->num_processos = 0;
    sim->processos = NULL;
    sim->total_acessos = 0;
    sim->page_faults = 0;
    sim->algoritmo = 0; 

    int num_frames = tamanho_memoria_fisica / tamanho_pagina;
    sim->memoria.num_frames = num_frames;
    sim->memoria.frames = (int *)malloc(num_frames * sizeof(int));
    sim->memoria.tempo_carga = (int *)malloc(num_frames * sizeof(int));

    for (int i = 0; i < num_frames; i++) {
        sim->memoria.frames[i] = -1;        
        sim->memoria.tempo_carga[i] = -1;    
    }

    return sim;
}
