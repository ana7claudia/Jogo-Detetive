#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   Detective Quest - Coleta de Pistas
   ------------------------------------------------------------
   Requisitos atendidos:
   - Árvore binária para o mapa (Salas com nome e pista).
   - Árvore BST para armazenar pistas coletadas.
   - Exploração a partir do Hall (e/d/s), coleta automática.
   - Exibição das pistas em ordem alfabética ao final.
   - Código organizado, nomes claros e comentários.
   ============================================================ */

/* ---------------------------- Estruturas ---------------------------- */
typedef struct Sala {
    char *nome;             /* nome do cômodo */
    char *pista;            /* pista opcional (pode ser NULL) */
    struct Sala *esq;       /* caminho esquerda */
    struct Sala *dir;       /* caminho direita */
} Sala;

/* Nó da BST de pistas (armazenadas ordenadas por texto) */
typedef struct PistaNode {
    char *texto;                /* conteúdo da pista */
    int count;                  /* qtd de vezes coletada (caso repetida) */
    struct PistaNode *esq, *dir;
} PistaNode;

/* ----------------------- Utilidades de string ----------------------- */
/* Implementação própria do strdup para portabilidade. */
static char *duplicaString(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

/* ------------------ Criação e destruição (Salas) ------------------- */
/* criarSala() – cria dinamicamente um cômodo com ou sem pista. */
Sala *criarSala(const char *nome, const char *pista) {
    Sala *nova = (Sala *)malloc(sizeof(Sala));
    if (!nova) {
        fprintf(stderr, "Erro ao alocar sala \"%s\".\n", nome);
        exit(EXIT_FAILURE);
    }
    nova->nome = duplicaString(nome);
    if (!nova->nome) {
        fprintf(stderr, "Erro ao alocar nome da sala.\n");
        free(nova);
        exit(EXIT_FAILURE);
    }
    /* pista é opcional; se string vazia, trate como NULL */
    if (pista && pista[0] != '\0') {
        nova->pista = duplicaString(pista);
        if (!nova->pista) {
            fprintf(stderr, "Erro ao alocar pista da sala.\n");
            free(nova->nome);
            free(nova);
            exit(EXIT_FAILURE);
        }
    } else {
        nova->pista = NULL;
    }
    nova->esq = nova->dir = NULL;
    return nova;
}

void liberarArvoreSalas(Sala *r) {
    if (!r) return;
    liberarArvoreSalas(r->esq);
    liberarArvoreSalas(r->dir);
    free(r->nome);
    free(r->pista);
    free(r);
}

/* -------------------- Criação e destruição (BST) -------------------- */
/* inserirPista() – insere nova pista na BST (ordem alfabética).
   Se a pista já existir, apenas incrementa o contador. */
void inserirPista(PistaNode **raiz, const char *texto) {
    if (!texto || texto[0] == '\0') return; /* ignora pistas vazias */

    if (*raiz == NULL) {
        PistaNode *novo = (PistaNode *)malloc(sizeof(PistaNode));
        if (!novo) {
            fprintf(stderr, "Erro ao alocar nó de pista.\n");
            exit(EXIT_FAILURE);
        }
        novo->texto = duplicaString(texto);
        if (!novo->texto) {
            fprintf(stderr, "Erro ao alocar texto da pista.\n");
            free(novo);
            exit(EXIT_FAILURE);
        }
        novo->count = 1;
        novo->esq = novo->dir = NULL;
        *raiz = novo;
        return;
    }

    int cmp = strcmp(texto, (*raiz)->texto);
    if (cmp == 0) {
        (*raiz)->count++;
    } else if (cmp < 0) {
        inserirPista(&((*raiz)->esq), texto);
    } else {
        inserirPista(&((*raiz)->dir), texto);
    }
}

/* exibirPistas() – imprime a BST in-order (ordem alfabética). */
void exibirPistas(PistaNode *r) {
    if (!r) return;
    exibirPistas(r->esq);
    if (r->count > 1)
        printf("- %s (x%d)\n", r->texto, r->count);
    else
        printf("- %s\n", r->texto);
    exibirPistas(r->dir);
}

void liberarArvorePistas(PistaNode *r) {
    if (!r) return;
    liberarArvorePistas(r->esq);
    liberarArvorePistas(r->dir);
    free(r->texto);
    free(r);
}

/* ------------------------- UI e interação -------------------------- */
static void cabecalho() {
    printf("\n==============================================\n");
    printf("   Detective Quest - Coleta de Pistas (BST)   \n");
    printf("==============================================\n");
}

/* Lê primeira letra não-espaço e normaliza minúscula */
static char lerOpcao() {
    char linha[64];
    if (!fgets(linha, sizeof(linha), stdin)) return 's';
    for (size_t i = 0; linha[i]; ++i) {
        if (!isspace((unsigned char)linha[i]))
            return (char)tolower((unsigned char)linha[i]);
    }
    return 's';
}

/* explorarSalasComPistas() – controla a navegação e coleta de pistas.
   A cada sala visitada, se houver pista, ela é inserida na BST. */
void explorarSalasComPistas(Sala *hall, PistaNode **pistas) {
    if (!hall) {
        printf("Mapa inexistente.\n");
        return;
    }

    cabecalho();
    Sala *atual = hall;

    /* coleta pista do Hall imediatamente */
    if (atual->pista) {
        inserirPista(pistas, atual->pista);
        printf("Voce esta no %s.\n", atual->nome);
        printf("Pista encontrada aqui: \"%s\"\n", atual->pista);
    } else {
        printf("Voce esta no %s. (Sem pista aqui)\n", atual->nome);
    }

    while (1) {
        printf("\nCaminhos disponiveis a partir de \"%s\":\n", atual->nome);
        if (atual->esq) printf("  (e) Esquerda: %s\n", atual->esq->nome);
        if (atual->dir) printf("  (d) Direita : %s\n", atual->dir->nome);
        printf("  (s) Sair da exploracao\n");
        printf("Escolha [e/d/s]: ");

        char op = lerOpcao();
        if (op == 's') {
            printf("\nExploracao encerrada pelo jogador.\n");
            break;
        } else if (op == 'e') {
            if (!atual->esq) {
                printf("Nao ha caminho a esquerda.\n");
                continue;
            }
            atual = atual->esq;
        } else if (op == 'd') {
            if (!atual->dir) {
                printf("Nao ha caminho a direita.\n");
                continue;
            }
            atual = atual->dir;
        } else {
            printf("Opcao invalida. Use 'e', 'd' ou 's'.\n");
            continue;
        }

        /* Ao entrar na nova sala, coletar pista (se houver) */
        if (atual->pista) {
            inserirPista(pistas, atual->pista);
            printf("\nVoce entrou em: %s\n", atual->nome);
            printf("Pista encontrada: \"%s\"\n", atual->pista);
        } else {
            printf("\nVoce entrou em: %s (Sem pista aqui)\n", atual->nome);
        }
    }
}

/* ---------------------- Montagem do mapa fixo ---------------------- */
/*
   Exemplo de mapa (mesmo layout base; agora com pistas):

                 [Hall de Entrada]  -> "Pegadas de lama"
                   /             \
        [Sala de Estar]       [Corredor] -> "Perfume forte"
           /        \           /     \
   [Biblioteca]   [Cozinha] [Escritorio] [Jardim] -> "Luva perdida"
      /     \           \                       \
 [Adega]  [Deposito]  [Despensa]             [Estufa]

   Alguns cômodos têm pistas, outros não.
*/
Sala *montarMapa() {
    Sala *hall        = criarSala("Hall de Entrada",     "Pegadas de lama");
    Sala *estar       = criarSala("Sala de Estar",       NULL);
    Sala *corredor    = criarSala("Corredor",            "Perfume forte");
    Sala *biblio      = criarSala("Biblioteca",          "Livro fora do lugar");
    Sala *cozinha     = criarSala("Cozinha",             NULL);
    Sala *escritorio  = criarSala("Escritorio",          "Janela entreaberta");
    Sala *jardim      = criarSala("Jardim",              "Luva perdida");
    Sala *adega       = criarSala("Adega",               "Taça quebrada");
    Sala *deposito    = criarSala("Deposito",            NULL);
    Sala *despensa    = criarSala("Despensa",            "Rastro de açúcar");
    Sala *estufa      = criarSala("Estufa",              "Terra revolvida");

    /* ligações */
    hall->esq = estar;
    hall->dir = corredor;

    estar->esq = biblio;
    estar->dir = cozinha;

    corredor->esq = escritorio;
    corredor->dir = jardim;

    biblio->esq = adega;
    biblio->dir = deposito;

    cozinha->dir = despensa;   /* folha com pista */
    jardim->dir  = estufa;     /* folha com pista */

    return hall; /* raiz da árvore de salas */
}

/* ------------------------------- main ------------------------------ */
int main(void) {
    Sala *mapa = montarMapa();

    while (1) {
        printf("\n===== Menu =====\n");
        printf("1 - Explorar mansao e coletar pistas\n");
        printf("0 - Sair\n");
        printf("Opcao: ");

        char linha[32];
        if (!fgets(linha, sizeof(linha), stdin)) break;
        int opcao = atoi(linha);

        if (opcao == 1) {
            /* BST de pistas inicia vazia para cada exploração */
            PistaNode *pistas = NULL;

            explorarSalasComPistas(mapa, &pistas);

            printf("\n=========== Pistas coletadas (ordem alfabetica) ===========\n");
            if (pistas) {
                exibirPistas(pistas);
            } else {
                printf("(Nenhuma pista coletada)\n");
            }
            printf("===========================================================\n");

            liberarArvorePistas(pistas);
        } else if (opcao == 0) {
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    liberarArvoreSalas(mapa);
    printf("Programa encerrado. Ate a proxima!\n");
    return 0;
}