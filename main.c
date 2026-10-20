#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define V             100
#define SPARSE_E      100
#define DENSE_E       4000
#define TRIALS        1000    
#define DEFAULT_SEED  2026
 

typedef struct {
    int u, v;
} Edge;
 
static struct {
    int  n;                
    Edge edges[DENSE_E];    
    char exist[V][V];      
} gen;
 
static void make_graph(int e_cnt)
{
    memset(&gen, 0, sizeof(gen));
 
    while (gen.n < e_cnt) {
        int u = rand() % V;
        int v = rand() % V;
 
        if (u == v) continue;          
        if (gen.exist[u][v]) continue; 
 
        gen.exist[u][v] = gen.exist[v][u] = 1;
        gen.edges[gen.n].u = u;
        gen.edges[gen.n].v = v;
        gen.n++;
    }
}
 

typedef struct {
    int m[V][V];
    int edge_cnt;
} AdjMatrix;
 
static void *mat_create(void)
{
    return calloc(1, sizeof(AdjMatrix));
}
 
static void mat_destroy(void *p)
{
    free(p);
}
 
static size_t mat_memory(void *p)
{
    (void)p;
    return sizeof(AdjMatrix);        
}
 
/* 삽입 : 이미 있는지 matrix[u][v] 한 칸만 확인 -> 비교 1회 */
static int mat_insert(void *p, int u, int v, long *cmp)
{
    AdjMatrix *g = p;
 
    (*cmp)++;
    if (g->m[u][v]) return 0;
 
    g->m[u][v] = 1;
    g->m[v][u] = 1;
    g->edge_cnt++;
    return 1;
}
 

static int mat_delete(void *p, int u, int v, long *cmp)
{
    AdjMatrix *g = p;
 
    (*cmp)++;
    if (!g->m[u][v]) return 0;
 
    g->m[u][v] = 0;
    g->m[v][u] = 0;
    g->edge_cnt--;
    return 1;
}
 
static int mat_connected(void *p, int u, int v, long *cmp)
{
    AdjMatrix *g = p;
 
    (*cmp)++;
    return g->m[u][v];
}
 
static int mat_adjacent(void *p, int u, long *cmp, int *out)
{
    AdjMatrix *g = p;
    int deg = 0;
 
    for (int v = 0; v < V; v++) {
        (*cmp)++;
        if (g->m[u][v]) {
            if (out) out[deg] = v;
            deg++;
        }
    }
    return deg;
}
 
typedef struct Node {
    int vertex;
    struct Node *next;
} Node;
 
typedef struct {
    Node *head[V];
    int   edge_cnt;
} AdjList;
 
static void *list_create(void)
{
    return calloc(1, sizeof(AdjList));
}
 
static void list_destroy(void *p)
{
    AdjList *g = p;
 
    for (int i = 0; i < V; i++) {
        Node *cur = g->head[i];
        while (cur) {
            Node *nx = cur->next;
            free(cur);
            cur = nx;
        }
    }
    free(g);
}
 

static size_t list_memory(void *p)
{
    AdjList *g = p;
    size_t nodes = 0;
 
    for (int i = 0; i < V; i++)
        for (Node *cur = g->head[i]; cur; cur = cur->next)
            nodes++;
 
    return sizeof(AdjList) + nodes * sizeof(Node);
}
 

static Node *find_node(AdjList *g, int u, int v, long *cmp)
{
    for (Node *cur = g->head[u]; cur; cur = cur->next) {
        (*cmp)++;
        if (cur->vertex == v) return cur;
    }
    return NULL;
}
 
static void push_front(AdjList *g, int u, int v)
{
    Node *n = malloc(sizeof(Node));
 
    if (!n) {
        fprintf(stderr, "malloc 실패\n");
        exit(1);
    }
    n->vertex = v;
    n->next = g->head[u];
    g->head[u] = n;
}

static int unlink_node(AdjList *g, int u, int v, long *cmp)
{
    Node *prev = NULL;
 
    for (Node *cur = g->head[u]; cur; prev = cur, cur = cur->next) {
        (*cmp)++;
        if (cur->vertex == v) {
            if (prev) prev->next = cur->next;
            else      g->head[u] = cur->next;
            free(cur);
            return 1;
        }
    }
    return 0;
}
 

static int list_insert(void *p, int u, int v, long *cmp)
{
    AdjList *g = p;
 
    if (find_node(g, u, v, cmp)) return 0;
 
    push_front(g, u, v);
    push_front(g, v, u);
    g->edge_cnt++;
    return 1;
}

static int list_delete(void *p, int u, int v, long *cmp)
{
    AdjList *g = p;
 
    if (!unlink_node(g, u, v, cmp)) return 0;
    unlink_node(g, v, u, cmp);
    g->edge_cnt--;
    return 1;
}
 
static int list_connected(void *p, int u, int v, long *cmp)
{
    return find_node((AdjList *)p, u, v, cmp) != NULL;
}
 
static int list_adjacent(void *p, int u, long *cmp, int *out)
{
    AdjList *g = p;
    int deg = 0;
 
    for (Node *cur = g->head[u]; cur; cur = cur->next) {
        (*cmp)++;
        if (out) out[deg] = cur->vertex;
        deg++;
    }
    return deg;
}
 

typedef struct {
    const char *name;
    void  *(*create)(void);
    void   (*destroy)(void *);
    size_t (*memory)(void *);
    int    (*insert)(void *, int, int, long *);
    int    (*del)(void *, int, int, long *);
    int    (*connected)(void *, int, int, long *);
    int    (*adjacent)(void *, int, long *, int *);
} Ops;
 
static const Ops MATRIX_OPS = {
    "Adjacency Matrix", mat_create, mat_destroy, mat_memory,
    mat_insert, mat_delete, mat_connected, mat_adjacent
};
 
static const Ops LIST_OPS = {
    "Adjacency List", list_create, list_destroy, list_memory,
    list_insert, list_delete, list_connected, list_adjacent
};
 

typedef struct {
    int iu, iv;  
    int du, dv;     
    int cu, cv;    
    int av;        
 
static void make_query(Query *q)
{
  
    do {
        q->iu = rand() % V;
        q->iv = rand() % V;
    } while (q->iu == q->iv || gen.exist[q->iu][q->iv]);
 

    Edge e = gen.edges[rand() % gen.n];
    if (rand() % 2) { q->du = e.u; q->dv = e.v; }
    else            { q->du = e.v; q->dv = e.u; }
 

    do {
        q->cu = rand() % V;
        q->cv = rand() % V;
    } while (q->cu == q->cv);
 
    q->av = rand() % V;
}
 

typedef struct {
    size_t mem;
    long   ins, del, conn, adj;
    int    linked;      /* 연결 여부 결과 */
    int    deg;
    int    nb[V];       /* 인접 정점 목록 */
} Result;
 
static void build_graph(const Ops *ops, void *g)
{
    long dummy = 0;   /* 생성 과정의 비교 횟수는 측정 대상 아님 */
 
    for (int i = 0; i < gen.n; i++)
        ops->insert(g, gen.edges[i].u, gen.edges[i].v, &dummy);
}
 
static void measure(const Ops *ops, void *g, const Query *q, Result *r)
{
    memset(r, 0, sizeof(*r));
 
    r->mem = ops->memory(g); 
 
    r->linked = ops->connected(g, q->cu, q->cv, &r->conn);
    r->deg    = ops->adjacent(g, q->av, &r->adj, r->nb);
 
    ops->insert(g, q->iu, q->iv, &r->ins);
    ops->del(g, q->du, q->dv, &r->del);
}
 

typedef struct { double ins, del, conn, adj; } Avg;
 
static Avg average(const Ops *ops, void *g, const Query *qs, int n)
{
    long ins = 0, del = 0, conn = 0, adj = 0;
 
    for (int i = 0; i < n; i++) {
        const Query *q = &qs[i];
        long c, dummy = 0;
 
        c = 0;
        ops->insert(g, q->iu, q->iv, &c);
        ins += c;
        ops->del(g, q->iu, q->iv, &dummy);   
 
        c = 0;
        ops->del(g, q->du, q->dv, &c);
        del += c;
        ops->insert(g, q->du, q->dv, &dummy); 
 
        c = 0;
        ops->connected(g, q->cu, q->cv, &c);
        conn += c;
 
        c = 0;
        ops->adjacent(g, q->av, &c, NULL);
        adj += c;
    }
 
    Avg a = { (double)ins / n, (double)del / n, (double)conn / n, (double)adj / n };
    return a;
}
 

static int same_graph(void *m, void *l)
{
    long d = 0;
 
    for (int u = 0; u < V; u++)
        for (int v = 0; v < V; v++)
            if (MATRIX_OPS.connected(m, u, v, &d) != LIST_OPS.connected(l, u, v, &d))
                return 0;
    return 1;
}
 

static void print_case(int no, const char *graph, const Ops *ops, const Query *q, const Result *r)
{
    printf("Case %d : %s Graph - %s\n", no, graph, ops->name);
    printf("Memory                         : %zu Bytes\n", r->mem);
    printf("Edge insertion comparisons     : %ld   (insert %d-%d)\n", r->ins, q->iu, q->iv);
    printf("Edge deletion comparisons      : %ld   (delete %d-%d)\n", r->del, q->du, q->dv);
    printf("Connectivity check comparisons : %ld   (%d-%d -> %s)\n",
           r->conn, q->cu, q->cv, r->linked ? "connected" : "not connected");
    printf("Adjacent vertices comparisons  : %ld   (vertex %d, degree %d)\n", r->adj, q->av, r->deg);
 
    printf("  adjacent of %d :", q->av);
    for (int i = 0; i < r->deg; i++)
        printf(" %d", r->nb[i]);
    printf("\n\n");
}
 
static void print_avg(const char *tag, const Avg *a)
{
    printf("  %-18s ins %8.2f | del %8.2f | conn %8.2f | adj %8.2f\n",
           tag, a->ins, a->del, a->conn, a->adj);
}
 
static void run_graph(const char *name, int e_cnt, int first_case)
{
    static Query qs[TRIALS];
    Query q;
    Result rm, rl;
 
    make_graph(e_cnt);
 
    void *mat = MATRIX_OPS.create();
    void *lst = LIST_OPS.create();
    if (!mat || !lst) {
        fprintf(stderr, "메모리 할당 실패\n");
        exit(1);
    }
 
    build_graph(&MATRIX_OPS, mat);
    build_graph(&LIST_OPS, lst);
 
    printf("==== %s Graph (V=%d, E=%d) ====\n", name, V, gen.n);
    printf("[check] 행렬/리스트 동일 그래프 : %s\n\n", same_graph(mat, lst) ? "OK" : "FAIL");
 

    make_query(&q);
    for (int i = 0; i < TRIALS; i++)
        make_query(&qs[i]);
 
    measure(&MATRIX_OPS, mat, &q, &rm);
    measure(&LIST_OPS, lst, &q, &rl);
 
    print_case(first_case,     name, &MATRIX_OPS, &q, &rm);
    print_case(first_case + 1, name, &LIST_OPS,   &q, &rl);
 

    printf("[check] 삽입/삭제 후에도 동일 그래프 : %s\n\n", same_graph(mat, lst) ? "OK" : "FAIL");
 
    Avg am = average(&MATRIX_OPS, mat, qs, TRIALS);
    Avg al = average(&LIST_OPS, lst, qs, TRIALS);
 
    printf("[참고] %s Graph - 랜덤 정점 쌍 %d회 평균 비교 횟수\n", name, TRIALS);
    print_avg("Adjacency Matrix", &am);
    print_avg("Adjacency List", &al);
    printf("\n");
 
    MATRIX_OPS.destroy(mat);
    LIST_OPS.destroy(lst);
}
 
int main(int argc, char *argv[])
{
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : DEFAULT_SEED;
 
    srand(seed);
    printf("seed = %u\n\n", seed);
 
    run_graph("Sparse", SPARSE_E, 1);
    run_graph("Dense",  DENSE_E,  3);
 
    return 0;
}
 
