#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// -----------------------
// 1. 구조체 정의
// -----------------------

// 노드 구조체 정의
typedef struct Node {
    unsigned char data; // 1바이트 데이터 (0x00 ~ 0xFF)
    int height; // 노드의 높이
    struct Node *left; // 왼쪽 자식 노드
    struct Node *right; // 오른쪽 자식 노드
} Node;

// 우선순위 큐 구조체 정의
typedef struct priority_queue {
    Node *root; // AVL 트리의 루트 노드
} priority_queue;

typedef struct thread_arg {
    char *file_name;
    priority_queue *pq_ptr;
    off_t quota;
    off_t offset;
} thread_arg;

// -----------------------
// 2. 함수 선언 (프로토타입)
// -----------------------

// 우선순위 큐 초기화
void init_priority_queue(priority_queue *pq);

// 삽입 연산 (enqueue)
void enqueue(priority_queue *pq, unsigned char data);

// 삭제 연산 (dequeue)
unsigned char dequeue(priority_queue *pq);

// 루트 노드 반환
Node *get_root(priority_queue *pq);

// 중위 순회 (디버깅 용도)
void in_order(Node *root);

// 트리의 모든 노드 해제
void free_tree(Node *root);

// 내부 함수 선언 (헤더에 노출되지 않음)

// 노드의 높이 반환
static int get_height(Node *node);

// 노드의 균형 인수 계산
static int get_balance_factor(Node *node);

// 노드의 높이 갱신
static void update_height(Node *node);

// 오른쪽 회전
static Node *rotate_right(Node *y);

// 왼쪽 회전
static Node *rotate_left(Node *x);

// 노드 생성
static Node *create_node(unsigned char data);

// 삽입 연산 (오름차순 정렬)
static Node *insert_node(Node *node, unsigned char data);

// 가장 큰 값의 노드 찾기 (우선순위가 가장 높은 원소)
static Node *find_min(Node *node);

// 삭제 연산 (우선순위가 가장 높은 원소 제거)
static Node *delete_min(Node *node);

void *thread_func(void *arg);

off_t find_offset(char *file_name);

off_t find_size(int n, char *file_name);

// -----------------------
// 4. 메인 함수
// -----------------------

int main(int argc, char *argv[]) {
    int n = 1;
    char *string = "673aef41575027558828.bmp";
    char *string2 = "output.bmp";
    priority_queue pq;
    init_priority_queue(&pq);
    thread_arg **thread_args;
    pthread_t *thread_ids;
    int status;
    int read_fd;
    int write_fd;
    unsigned char buf;

    thread_args = (thread_arg **) malloc(n * sizeof(thread_arg *));
    for (int i = 0; i < n; i++) {
        thread_args[i] = (thread_arg *) malloc(sizeof(thread_arg));
    }

    off_t size = find_size(n, string);
    off_t quota = size / n;
    off_t start_offset = find_offset(string);
    for (int i = 0; i < n - 1; i++) {
        thread_args[i]->file_name = string;
        thread_args[i]->pq_ptr = &pq;
        thread_args[i]->quota = quota;
        thread_args[i]->offset = start_offset + i * quota;
    }
    thread_args[n - 1]->file_name = string;
    thread_args[n - 1]->pq_ptr = &pq;
    thread_args[n - 1]->quota = size - (n - 1) * quota;
    thread_args[n - 1]->offset = start_offset + (n - 1) * quota;

    thread_ids = (pthread_t *) malloc(n * sizeof(pthread_t));
    for (int i = 0; i < n; ++i) {
        status = pthread_create(&thread_ids[i], NULL, thread_func, thread_args[i]);

        if (status != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; ++i) {
        status = pthread_join(thread_ids[i], NULL);
        if (status != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    read_fd = open(string, O_RDONLY | O_BINARY);
    write_fd = open(string2, O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, 0777);
    if (read_fd < 0 || write_fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < start_offset; ++i) {
        if (read(read_fd, &buf, sizeof(buf)) < 0) {
            if (errno == EINTR) {
                i--;
                continue;
            }
            perror("read");
            close(read_fd);
            exit(EXIT_FAILURE);
        }

        if (write(write_fd, &buf, sizeof(buf)) < 0) {
            perror("write");
            close(write_fd);
            exit(EXIT_FAILURE);
        }
    }

    while (pq.root != NULL) {
        buf = dequeue(&pq);
        if (write(write_fd, &buf, sizeof(buf)) < 0) {
            perror("write");
            close(write_fd);
            exit(EXIT_FAILURE);
        }
    }

    close(read_fd);
    close(write_fd);
    pthread_mutex_destroy(&mutex);
    free(thread_ids);
    for (int i = 0; i < n; i++) {
        free(thread_args[i]);
    }
    free(thread_args);
    free_tree(pq.root);
}

// -----------------------
// 3. 함수 정의
// -----------------------

// 우선순위 큐 초기화
void init_priority_queue(priority_queue *pq) {
    if (pq == NULL)
        return;
    pq->root = NULL;
}

// 우선순위 큐의 삽입 함수 (enqueue)
void enqueue(priority_queue *pq, unsigned char data) {
    if (pq == NULL)
        return;
    pthread_mutex_lock(&mutex);
    pq->root = insert_node(pq->root, data);
    pthread_mutex_unlock(&mutex);
}

unsigned char dequeue(priority_queue *pq) {
    if (pq == NULL || pq->root == NULL) {
        // fprintf(stderr, "우선순위 큐가 비어 있습니다.\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&mutex);

    // 가장 작은 값의 노드를 찾음
    Node *minNode = find_min(pq->root);
    unsigned char minValue = minNode->data;

    // 가장 작은 값의 노드를 삭제
    pq->root = delete_min(pq->root);

    pthread_mutex_unlock(&mutex);

    return minValue;
}


// 노드의 높이 반환
static int get_height(Node *node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// 노드의 균형 인수 계산
static int get_balance_factor(Node *node) {
    if (node == NULL)
        return 0;
    return get_height(node->left) - get_height(node->right);
}

// 노드의 높이 갱신
static void update_height(Node *node) {
    if (node == NULL)
        return;
    int leftHeight = get_height(node->left);
    int rightHeight = get_height(node->right);
    node->height = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
}

// 오른쪽 회전
static Node *rotate_right(Node *y) {
    if (y == NULL || y->left == NULL) {
        // fprintf(stderr, "rotate_right: 회전 불가\n");
        return y;
    }

    Node *x = y->left;
    Node *T2 = x->right;

    // 회전 수행
    x->right = y;
    y->left = T2;

    // 높이 갱신
    update_height(y);
    update_height(x);

    return x; // 새로운 루트 반환
}

// 왼쪽 회전
static Node *rotate_left(Node *x) {
    if (x == NULL || x->right == NULL) {
        // fprintf(stderr, "rotate_left: 회전 불가\n");
        return x;
    }

    Node *y = x->right;
    Node *T2 = y->left;

    // 회전 수행
    y->left = x;
    x->right = T2;

    // 높이 갱신
    update_height(x);
    update_height(y);

    return y; // 새로운 루트 반환
}

// 노드 생성
static Node *create_node(unsigned char data) {
    Node *node = (Node *) malloc(sizeof(Node)); // 수정된 부분
    if (node == NULL) {
        perror("메모리 할당 실패");
        exit(EXIT_FAILURE);
    }
    node->data = data;
    node->height = 1; // 새 노드의 초기 높이는 1
    node->left = NULL;
    node->right = NULL;
    return node;
}

// 삽입 연산 (오름차순 정렬)
static Node *insert_node(Node *node, unsigned char data) {
    // 이진 탐색 트리 삽입
    if (node == NULL)
        return create_node(data);

    if (data < node->data)
        node->left = insert_node(node->left, data);
    else // 중복된 값은 오른쪽 서브트리에 삽입
        node->right = insert_node(node->right, data);

    // 높이 갱신
    update_height(node);

    // 균형 인수 계산
    int balance = get_balance_factor(node);

    // 균형 조정
    // LL 회전
    if (balance > 1 && data < node->left->data)
        return rotate_right(node);

    // RR 회전
    if (balance < -1 && data > node->right->data)
        return rotate_left(node);

    // LR 회전
    if (balance > 1 && data > node->left->data) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // RL 회전
    if (balance < -1 && data < node->right->data) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node; // 노드 포인터 반환
}

// 가장 큰 값의 노드 찾기 (우선순위가 가장 높은 원소)
static Node *find_min(Node *node) {
    Node *current = node;
    while (current->left != NULL) {
        current = current->left; // 왼쪽 자식으로 계속 이동
    }
    return current;
}

// 가장 작은 값을 가진 노드 삭제
static Node *delete_min(Node *node) {
    if (node == NULL) {
        // fprintf(stderr, "delete_min: NULL 노드 발견\n");
        return NULL;
    }

    if (node->left == NULL) {
        // printf("delete_min: 삭제할 노드: %d\n", node->data);
        Node *temp = node->right;
        free(node);
        return temp;
    }

    // printf("delete_min: 현재 노드: %d\n", node->data);
    node->left = delete_min(node->left);

    // 높이 갱신
    update_height(node);

    // 균형 조정
    int balance = get_balance_factor(node);
    // printf("delete_min: 노드 %d 균형 인수: %d\n", node->data, balance);

    if (balance > 1 && get_balance_factor(node->left) >= 0) {
        return rotate_right(node);
    }

    if (balance > 1 && get_balance_factor(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    if (balance < -1 && get_balance_factor(node->right) <= 0) {
        return rotate_left(node);
    }

    if (balance < -1 && get_balance_factor(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}


// 우선순위 큐의 루트 노드를 반환하는 함수
Node *get_root(priority_queue *pq) {
    if (pq == NULL) {
        // fprintf(stderr, "우선순위 큐가 초기화되지 않았습니다.\n");
        return NULL;
    }
    return pq->root;
}

// 중위 순회 (디버깅 용도) - 오름차순 출력
void in_order(Node *root) {
    if (root == NULL)
        return;
    in_order(root->left);
    printf("%02X ", root->data); // 16진수 두자리로 출력
    in_order(root->right);
}

// 트리의 모든 노드 해제 (후위 순회)
void free_tree(Node *root) {
    if (root == NULL)
        return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

void *thread_func(void *arg) {
    thread_arg *thread_argument = (thread_arg *) arg;
    int fd;
    unsigned char buf;
    ssize_t ret;

    fd = open(thread_argument->file_name, O_RDONLY | O_BINARY);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    lseek(fd, thread_argument->offset, SEEK_SET);
    // pthread_mutex_lock(&mutex);
    off_t i;
    for (i = 0; i < thread_argument->quota; ++i) {
        ret = read(fd, &buf, 1);
        if (ret < 0) {
            if (errno == EINTR) {
                i--;
                continue;
            }
            perror("read");
            close(fd);
            exit(EXIT_FAILURE);
        } else if (ret == 0) {
            break;
        }
        enqueue(thread_argument->pq_ptr, buf);
    }
    // pthread_mutex_unlock(&mutex);

    close(fd);
    return NULL;
}

off_t find_offset(char *file_name) {
    int fd = open(file_name, O_RDONLY | O_BINARY);
    unsigned char bytes[4];

    lseek(fd, 10, SEEK_SET);
    for (int i = 0; i < 4; ++i) {
        read(fd, &bytes[i], 1);
    }
    off_t offset = bytes[0] | ((off_t) bytes[1] << 8) | ((off_t) bytes[2] << 16) | ((off_t) bytes[3] << 24);
    close(fd);

    return offset;
}

off_t find_size(int n, char *file_name) {
    int fd = open(file_name, O_RDONLY | O_BINARY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    off_t size = lseek(fd, 0, SEEK_END);
    off_t start = find_offset(file_name);
    size -= start;
    close(fd);

    return size;
}
