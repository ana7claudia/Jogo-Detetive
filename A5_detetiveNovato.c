#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   Detective Quest - Mapa da Mansão (Árvore Binária)
   ------------------------------------------------------------
   Requisitos atendidos:
   - Árvore binária com alocação dinâmica (struct Sala).
   - Exploração interativa a partir do Hall (e/d/s).
   - Mansão montada automaticamente na main() via criarSala().
   - Exibe cada sala visitada e encerra ao chegar em um nó-folha
     ou quando o jogador escolher sair.
   - Código organizado, legível e comentado.
   ============================================================ */

typedef struct Sala {
    char *nome;               // nome da sala (string dinâmica)
    struct Sala *esq;         // caminho à esquerda
    struct Sala *dir;         // caminho à direita
} Sala;

/* ----------------- Utilidades de string ----------------- */
/* strdup é POSIX; para portabilidade, implementamos nossa própria */
static char *duplicaString(const char *s) {
    size_t n = strlen(s) + 1;
    char *copia = (char *)malloc(n);
    if (!copia) return NULL;
    memcpy(copia, s, n);
    return copia;
}

/* ----------------- Criação e destruição ----------------- */
/* criarSala() – cria, de forma dinâmica, uma sala com nome. */
Sala *criarSala(const char *nome) {
    Sala *nova = (Sala *)malloc(sizeof(Sala));
    if (!nova) {
        fprintf(stderr, "Erro: falha ao alocar memoria para sala \"%s\".\n", nome);
        exit(EXIT_FAILURE);
    }
    nova->nome = duplicaString(nome);
    if (!nova->nome) {
        fprintf(stderr, "Erro: falha ao alocar memoria para nome da sala.\n");
        free(nova);
        exit(EXIT_FAILURE);
    }
    nova->esq = nova->dir = NULL;
    return nova;
}

/* liberarArvore() – libera toda a árvore (pós-ordem). */
void liberarArvore(Sala *raiz) {
    if (!raiz) return;
    liberarArvore(raiz->esq);
    liberarArvore(raiz->dir);
    free(raiz->nome);
    free(raiz);
}

/* ----------------- Visual e interação ----------------- */
static void cabecalho() {
    printf("\n==============================================\n");
    printf("        Detective Quest - Mansao Enigma        \n");
    printf("==============================================\n");
}

/* Mostra as opções contextuais com base nos caminhos disponíveis */
static void mostrarOpcoes(const Sala *atual) {
    printf("\nVoce esta em: %s\n", atual->nome);
    printf("Caminhos disponiveis:\n");
    if (atual->esq)  printf("  (e) Esquerda: %s\n", atual->esq->nome);
    if (atual->dir)  printf("  (d) Direita : %s\n", atual->dir->nome);
    if (!atual->esq && !atual->dir)
        printf("  Nenhum. (fim de caminho)\n");
    printf("  (s) Sair da exploracao\n");
    printf("Escolha [e/d/s]: ");
}

/* Ler a primeira letra não-espaco da linha e normalizar para minúsculo */
static char lerOpcao() {
    char linha[64];
    if (!fgets(linha, sizeof(linha), stdin)) return 's'; // em caso de EOF, sair
    for (size_t i = 0; linha[i]; ++i) {
        if (!isspace((unsigned char)linha[i])) {
            return (char)tolower((unsigned char)linha[i]);
        }
    }
    return 's';
}

/* explorarSalas() – permite a navegação do jogador pela árvore.
   Guarda o caminho percorrido para exibir ao final. */
void explorarSalas(Sala *raiz) {
    if (!raiz) {
        printf("Mapa vazio.\n");
        return;
    }

    /* Para registrar o trajeto visitado (até 128 salas – mais que suficiente aqui) */
    const int MAX_TRAJETO = 128;
    const char *trajeto[128];
    int passos = 0;

    Sala *atual = raiz;
    cabecalho();
    printf("Bem-vindo(a)! Iniciando no Hall de entrada.\n");

    while (atual) {
        /* registra a sala atual no trajeto */
        if (passos < MAX_TRAJETO) trajeto[passos++] = atual->nome;

        /* se for folha, finaliza a exploração */
        if (!atual->esq && !atual->dir) {
            printf("\nVoce chegou ao fim do caminho em: %s\n", atual->nome);
            break;
        }

        /* mostra opções e lê escolha */
        mostrarOpcoes(atual);
        char op = lerOpcao();

        if (op == 's') {
            printf("\nExploracao encerrada pelo jogador.\n");
            break;
        } else if (op == 'e') {
            if (atual->esq) {
                atual = atual->esq;
            } else {
                printf("Nao ha caminho a esquerda a partir de %s.\n", atual->nome);
            }
        } else if (op == 'd') {
            if (atual->dir) {
                atual = atual->dir;
            } else {
                printf("Nao ha caminho a direita a partir de %s.\n", atual->nome);
            }
        } else {
            printf("Opcao invalida. Use 'e', 'd' ou 's'.\n");
        }
    }

    /* Exibe o trajeto completo percorrido */
    printf("\n---------- Salas visitadas ----------\n");
    for (int i = 0; i < passos; ++i) {
        printf("%s%s", trajeto[i], (i + 1 < passos ? " -> " : "\n"));
    }
    printf("-------------------------------------\n");
}

/* ----------------- Montagem do mapa ----------------- */
/*
   Mapa proposto (exemplo):

                 [Hall de entrada]
                  /               \
        [Sala de Estar]        [Corredor]
            /      \             /      \
   [Biblioteca]  [Cozinha]  [Escritorio] [Jardim]
       /   \        \                      \
 [Adega] [Deposito] [Despensa]           [Estufa]

  - Vários nós-folha para ilustrar finais de caminho.
*/
Sala *montarMapa() {
    Sala *hall        = criarSala("Hall de entrada");
    Sala *estar       = criarSala("Sala de Estar");
    Sala *corredor    = criarSala("Corredor");
    Sala *biblio      = criarSala("Biblioteca");
    Sala *cozinha     = criarSala("Cozinha");
    Sala *escritorio  = criarSala("Escritorio");
    Sala *jardim      = criarSala("Jardim");
    Sala *adega       = criarSala("Adega");
    Sala *deposito    = criarSala("Deposito");
    Sala *despensa    = criarSala("Despensa");
    Sala *estufa      = criarSala("Estufa");

    /* ligações */
    hall->esq = estar;
    hall->dir = corredor;

    estar->esq = biblio;
    estar->dir = cozinha;

    corredor->esq = escritorio;
    corredor->dir = jardim;

    biblio->esq = adega;
    biblio->dir = deposito;

    /* folhas adicionais */
    cozinha->dir = despensa;  /* cozinha -> despensa */
    jardim->dir  = estufa;    /* jardim -> estufa    */

    return hall; /* raiz da árvore */
}

/* ----------------- main() ----------------- */
/* main() – monta o mapa inicial e dá início à exploração. */
int main(void) {
    Sala *raiz = montarMapa();

    /* Loop simples com menu para começar ou sair (novato-friendly) */
    while (1) {
        printf("\n===== Menu =====\n");
        printf("1 - Explorar a mansao\n");
        printf("0 - Sair\n");
        printf("Opcao: ");

        char linha[32];
        if (!fgets(linha, sizeof(linha), stdin)) break;
        int opcao = atoi(linha);

        if (opcao == 1) {
            explorarSalas(raiz);
        } else if (opcao == 0) {
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    liberarArvore(raiz);
    printf("Programa encerrado. Ate a proxima!\n");
    return 0;
}