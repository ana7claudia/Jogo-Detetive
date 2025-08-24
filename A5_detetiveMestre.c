#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   Detective Quest - Capítulo Final (Salas + Pistas + Julgamento)
   ------------------------------------------------------------
   Requisitos atendidos:
   - Árvore binária de cômodos (mapa fixo).
   - Pistas associadas por lógica fixa com base no nome da sala.
   - BST de pistas coletadas (ordem alfabética).
   - Tabela Hash: pista -> suspeito.
   - Exploração interativa (e/d/s), listagem final e acusação.
   - Verificação automática: pelo menos 2 pistas precisam apontar
     para o suspeito acusado para condenar.
   ============================================================ */

/* ========================= Estruturas ========================= */

/* Árvore de Salas (mapa) */
typedef struct Sala {
    char *nome;
    struct Sala *esq;
    struct Sala *dir;
} Sala;

/* BST de Pistas Coletadas (ordenadas alfabeticamente) */
typedef struct PistaNode {
    char *texto;             /* conteúdo da pista */
    int count;               /* quantas vezes coletada */
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Tabela Hash (encadeamento) para pista -> suspeito */
typedef struct HashNode {
    char *chavePista;        /* pista (key) */
    char *suspeito;          /* suspeito (value) */
    struct HashNode *prox;
} HashNode;

typedef struct HashTable {
    size_t capacidade;
    HashNode **buckets;
} HashTable;

/* ===================== Utilidades de string ==================== */

/* Implementação própria do strdup para portabilidade */
static char *duplicaString(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

/* Remove newline/espacos finais (qualquer \r\n e espaços) */
static void rstrip(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[--n] = '\0';
    }
}

/* ==================== Hash (pista -> suspeito) ==================== */

/* Hash DJB2 (boa distribuição para strings) */
static unsigned long djb2(const unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }
    return hash;
}

HashTable *criarHash(size_t capacidade) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (!ht) {
        fprintf(stderr, "Erro ao alocar HashTable.\n");
        exit(EXIT_FAILURE);
    }
    ht->capacidade = capacidade;
    ht->buckets = (HashNode **)calloc(capacidade, sizeof(HashNode *));
    if (!ht->buckets) {
        fprintf(stderr, "Erro ao alocar buckets da HashTable.\n");
        free(ht);
        exit(EXIT_FAILURE);
    }
    return ht;
}

/* inserirNaHash() – insere associação pista/suspeito na tabela hash. */
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!ht || !pista || !suspeito) return;
    unsigned long h = djb2((const unsigned char *)pista) % ht->capacidade;

    /* Atualiza se já existir mesma chave */
    for (HashNode *no = ht->buckets[h]; no; no = no->prox) {
        if (strcmp(no->chavePista, pista) == 0) {
            /* substitui suspeito */
            free(no->suspeito);
            no->suspeito = duplicaString(suspeito);
            return;
        }
    }
    /* não encontrado: insere novo no início da lista */
    HashNode *novo = (HashNode *)malloc(sizeof(HashNode));
    if (!novo) {
        fprintf(stderr, "Erro ao alocar nó da HashTable.\n");
        exit(EXIT_FAILURE);
    }
    novo->chavePista = duplicaString(pista);
    novo->suspeito   = duplicaString(suspeito);
    novo->prox = ht->buckets[h];
    ht->buckets[h] = novo;
}

/* encontrarSuspeito() – consulta o suspeito correspondente a uma pista. */
const char *encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!ht || !pista) return NULL;
    unsigned long h = djb2((const unsigned char *)pista) % ht->capacidade;
    for (HashNode *no = ht->buckets[h]; no; no = no->prox) {
        if (strcmp(no->chavePista, pista) == 0) {
            return no->suspeito;
        }
    }
    return NULL;
}

void liberarHash(HashTable *ht) {
    if (!ht) return;
    for (size_t i = 0; i < ht->capacidade; ++i) {
        HashNode *no = ht->buckets[i];
        while (no) {
            HashNode *prox = no->prox;
            free(no->chavePista);
            free(no->suspeito);
            free(no);
            no = prox;
        }
    }
    free(ht->buckets);
    free(ht);
}

/* ==================== BST de pistas coletadas ==================== */

/* inserirPista() / adicionarPista() – insere a pista coletada na BST. */
void inserirPista(PistaNode **raiz, const char *texto) {
    if (!texto || texto[0] == '\0') return;
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
        inserirPista(&(*raiz)->esq, texto);
    } else {
        inserirPista(&(*raiz)->dir, texto);
    }
}

/* Percorre em ordem, aplicando callback (útil para contagens ou impressão) */
typedef void (*VisitaPista)(const PistaNode *n, void *udata);

void percorrerInOrder(const PistaNode *r, VisitaPista f, void *udata) {
    if (!r) return;
    percorrerInOrder(r->esq, f, udata);
    f(r, udata);
    percorrerInOrder(r->dir, f, udata);
}

/* exibirPistas() – imprime a árvore de pistas em ordem alfabética. */
void exibirPistas(const PistaNode *r) {
    if (!r) return;
    exibirPistas(r->esq);
    if (r->count > 1) printf("- %s (x%d)\n", r->texto, r->count);
    else              printf("- %s\n", r->texto);
    exibirPistas(r->dir);
}

void liberarBST(PistaNode *r) {
    if (!r) return;
    liberarBST(r->esq);
    liberarBST(r->dir);
    free(r->texto);
    free(r);
}

/* ====================== Árvore de Salas (mapa) ====================== */

/* criarSala() – cria dinamicamente um cômodo. */
Sala *criarSala(const char *nome) {
    Sala *s = (Sala *)malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro ao alocar sala \"%s\".\n", nome);
        exit(EXIT_FAILURE);
    }
    s->nome = duplicaString(nome);
    if (!s->nome) {
        fprintf(stderr, "Erro ao alocar nome da sala.\n");
        free(s);
        exit(EXIT_FAILURE);
    }
    s->esq = s->dir = NULL;
    return s;
}

void liberarArvoreSalas(Sala *r) {
    if (!r) return;
    liberarArvoreSalas(r->esq);
    liberarArvoreSalas(r->dir);
    free(r->nome);
    free(r);
}

/* Lógica fixa: dada uma sala, retorna a pista (ou NULL se não houver). */
const char *pistaDaSala(const char *nomeSala) {
    /* Você pode ajustar livremente os textos e quantidades.
       Alguns compartilham o mesmo suspeito (ver tabela hash). */
    if (strcmp(nomeSala, "Hall de Entrada") == 0) return "Pegadas de lama";
    if (strcmp(nomeSala, "Sala de Estar") == 0)   return "Almofada fora do lugar";
    if (strcmp(nomeSala, "Corredor") == 0)        return "Perfume forte";
    if (strcmp(nomeSala, "Biblioteca") == 0)      return "Livro raro deslocado";
    if (strcmp(nomeSala, "Cozinha") == 0)         return NULL;
    if (strcmp(nomeSala, "Escritorio") == 0)      return "Janela entreaberta";
    if (strcmp(nomeSala, "Jardim") == 0)          return "Luva de couro";
    if (strcmp(nomeSala, "Adega") == 0)           return "Taça com batom";
    if (strcmp(nomeSala, "Deposito") == 0)        return NULL;
    if (strcmp(nomeSala, "Despensa") == 0)        return "Rastro de acucar";
    if (strcmp(nomeSala, "Estufa") == 0)          return "Terra revolvida";
    return NULL;
}

/* ================== Exploração + coleta de pistas ================== */

/* Ler primeira letra não-espaço e normalizar */
static char lerOpcao() {
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) return 's';
    for (size_t i = 0; buf[i]; ++i) {
        if (!isspace((unsigned char)buf[i])) return (char)tolower((unsigned char)buf[i]);
    }
    return 's';
}

/* explorarSalas() – navega pela árvore e ativa o sistema de pistas.
   - Exibe sala atual, mostra/insere pista (BST) e informa suspeito (hash).
   - Caminhos: e/d/s. Exploração termina em 's'. */
void explorarSalas(Sala *hall, PistaNode **pistas, HashTable *mapaPistaSuspeito) {
    if (!hall) { printf("Mapa inexistente.\n"); return; }

    Sala *atual = hall;
    printf("\n==============================================\n");
    printf("    Detective Quest - Exploracao Final        \n");
    printf("==============================================\n");

    while (1) {
        printf("\nVoce esta em: %s\n", atual->nome);

        /* Coleta da pista da sala (se houver) */
        const char *p = pistaDaSala(atual->nome);
        if (p && p[0] != '\0') {
            inserirPista(pistas, p);
            const char *sus = encontrarSuspeito(mapaPistaSuspeito, p);
            if (sus) {
                printf("Pista encontrada: \"%s\" -> suspeito associado: %s\n", p, sus);
            } else {
                printf("Pista encontrada: \"%s\" (sem suspeito associado)\n", p);
            }
        } else {
            printf("Nenhuma pista encontrada aqui.\n");
        }

        /* Opções de navegação */
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
            if (!atual->esq) { printf("Nao ha caminho a esquerda.\n"); continue; }
            atual = atual->esq;
        } else if (op == 'd') {
            if (!atual->dir) { printf("Nao ha caminho a direita.\n"); continue; }
            atual = atual->dir;
        } else {
            printf("Opcao invalida. Use 'e', 'd' ou 's'.\n");
        }
    }
}

/* ========================== Julgamento ========================== */

/* Estrutura auxiliar para contagem por suspeito */
typedef struct {
    const HashTable *ht;
    const char *acusado;
    int total; /* total de pistas coletadas que mapeiam para o acusado */
} ContadorSuspeitoCtx;

static void contarSeDoAcusado(const PistaNode *n, void *ud) {
    ContadorSuspeitoCtx *ctx = (ContadorSuspeitoCtx *)ud;
    const char *sus = encontrarSuspeito((HashTable *)ctx->ht, n->texto);
    if (sus && strcmp(sus, ctx->acusado) == 0) {
        ctx->total += n->count;
    }
}

/* verificarSuspeitoFinal() – conduz à fase de julgamento final. */
void verificarSuspeitoFinal(PistaNode *pistas, HashTable *ht) {
    printf("\n=========== Pistas coletadas (ordem alfabetica) ===========\n");
    if (pistas) exibirPistas(pistas);
    else        printf("(Nenhuma pista coletada)\n");
    printf("===========================================================\n");

    /* Entrada do acusado */
    char entrada[128];
    printf("Informe o nome do suspeito para acusacao (ex.: \"Srta. Violeta\"): ");
    if (!fgets(entrada, sizeof(entrada), stdin)) {
        printf("Entrada invalida. Encerrando julgamento.\n");
        return;
    }
    rstrip(entrada);
    if (entrada[0] == '\0') {
        printf("Nenhum nome informado. Encerrando julgamento.\n");
        return;
    }

    /* Conta quantas pistas coletadas apontam para o acusado */
    ContadorSuspeitoCtx ctx = { ht, entrada, 0 };
    percorrerInOrder(pistas, contarSeDoAcusado, &ctx);

    if (ctx.total >= 2) {
        printf("\nVEREDITO: CULPADO!\n");
        printf("Ha pelo menos %d pista(s) que apontam para %s. Caso encerrado.\n", ctx.total, entrada);
    } else {
        printf("\nVEREDITO: INSUFICIENTE.\n");
        printf("Apenas %d pista(s) apontam para %s. Investigacao inconclusiva.\n", ctx.total, entrada);
    }
}

/* ======================== Montagem do Mapa ======================== */
/*
   Layout fixo (mesmo do capítulo anterior):

                 [Hall de Entrada]
                   /           \
         [Sala de Estar]     [Corredor]
            /      \           /     \
     [Biblioteca] [Cozinha] [Escritorio] [Jardim]
        /     \         \                    \
    [Adega] [Deposito] [Despensa]          [Estufa]
*/
Sala *montarMapa() {
    Sala *hall        = criarSala("Hall de Entrada");
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

    hall->esq = estar;      hall->dir = corredor;
    estar->esq = biblio;    estar->dir = cozinha;
    corredor->esq = escritorio; corredor->dir = jardim;
    biblio->esq = adega;    biblio->dir = deposito;
    cozinha->dir = despensa;
    jardim->dir = estufa;

    return hall; /* raiz */
}

/* ======================= Povoamento da Hash ======================= */
/* Popula a tabela hash com associações pista -> suspeito.
   Ajuste os nomes para o enredo desejado. */
void popularMapaPistas(HashTable *ht) {
    /* Suspeitos de exemplo */
    /* - Sr. Mostarda
       - Srta. Violeta
       - Coronel Mostarda (se preferir variação)
       - Dra. Orquidea
       - Professor Carvalho
       - Sra. Branca
       - Jardineiro */

    inserirNaHash(ht, "Pegadas de lama",        "Jardineiro");
    inserirNaHash(ht, "Almofada fora do lugar", "Sra. Branca");
    inserirNaHash(ht, "Perfume forte",          "Srta. Violeta");
    inserirNaHash(ht, "Livro raro deslocado",   "Professor Carvalho");
    inserirNaHash(ht, "Janela entreaberta",     "Sr. Mostarda");
    inserirNaHash(ht, "Luva de couro",          "Sr. Mostarda");
    inserirNaHash(ht, "Taca com batom",         "Srta. Violeta");
    inserirNaHash(ht, "Rastro de acucar",       "Dra. Orquidea");
    inserirNaHash(ht, "Terra revolvida",        "Jardineiro");

    /* Alias com acentos removidos nos textos das pistas para simplificar
       comparação em ambientes sem locale configurado. */
}

/* =============================== main ============================== */
int main(void) {
    /* 1) Monta o mapa fixo */
    Sala *mapa = montarMapa();

    /* 2) Cria a tabela hash e popula com pista -> suspeito */
    HashTable *ht = criarHash(101);
    popularMapaPistas(ht);

    /* 3) Loop simples de menu */
    while (1) {
        printf("\n===== Menu =====\n");
        printf("1 - Explorar mansao e coletar pistas\n");
        printf("0 - Sair\n");
        printf("Opcao: ");

        char linha[32];
        if (!fgets(linha, sizeof(linha), stdin)) break;
        int opcao = atoi(linha);

        if (opcao == 1) {
            /* BST de pistas inicia vazia a cada exploração */
            PistaNode *pistas = NULL;

            explorarSalas(mapa, &pistas, ht);
            verificarSuspeitoFinal(pistas, ht);

            liberarBST(pistas);
        } else if (opcao == 0) {
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    liberarHash(ht);
    liberarArvoreSalas(mapa);
    printf("Programa encerrado. Ate a proxima!\n");
    return 0;
}