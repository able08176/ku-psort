#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

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
    priority_queue *pq_ptr;
    int quota;
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
static Node *find_max(Node *node);

// 삭제 연산 (우선순위가 가장 높은 원소 제거)
static Node *delete_max(Node *node);

void *thread_func(void *arg);

off_t find_offset(char *file_name);

int find_size(int n, char *file_name);

// -----------------------
// 4. 메인 함수
// -----------------------

int main(int argc, char *argv[]) {
    int n = 10;
    char *string = "673aef41575027558828.bmp";
    // priority_queue pq;
    // init_priority_queue(&pq);
    //
    // thread_arg **thread_args = (thread_arg **) malloc(n * sizeof(thread_arg *));
    // for (int i = 0; i < n; i++) {
    //     thread_args[i] = (thread_arg *) malloc(sizeof(thread_arg));
    // }
    //
    // int size = find_size(n, string);
    // int quota = size / n;
    // off_t start_offset = find_offset(string);
    // for (int i = 0; i < n - 1; i++) {
    //     thread_args[i]->pq_ptr = &pq;
    //     thread_args[i]->quota = quota;
    //     thread_args[i]->offset = start_offset + i * quota;
    // }
    // thread_args[n - 1]->pq_ptr = &pq;
    // thread_args[n - 1]->quota = size - (n - 1) * quota;
    // thread_args[n - 1]->offset = start_offset + (n - 1) * quota;
    //
    // printf("Pixel Size: %d\n", size);
    // printf("Quota: %d\n", quota);
    // printf("Start Offset: %ld\n\n", start_offset);
    // for (int i = 0; i < n; ++i) {
    //     printf("Offset: %ld, Quota: %d\n", thread_args[i]->offset, thread_args[i]->quota);
    // }
    //
    // for (int i = 0; i < n; i++) {
    //     free(thread_args[i]);
    // }
    // free(thread_args);
    //
    // free_tree(pq.root);
    int size = find_size(n, string);

    return 0;
}

// -----------------------
// 3. 함수 정의
// -----------------------

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
    Node *node = (Node *) malloc(sizeof(node));
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
static Node *find_max(Node *node) {
    Node *current = node;
    while (current->right != NULL)
        current = current->right;
    return current;
}

// 삭제 연산 (우선순위가 가장 높은 원소 제거)
static Node *delete_max(Node *node) {
    if (node == NULL)
        return node;

    if (node->right == NULL) {
        Node *temp = node->left;
        free(node);
        return temp;
    }

    node->right = delete_max(node->right);

    // 높이 갱신
    update_height(node);

    // 균형 인수 계산
    int balance = get_balance_factor(node);

    // 균형 조정
    // LL 회전
    if (balance > 1 && get_balance_factor(node->left) >= 0)
        return rotate_right(node);

    // LR 회전
    if (balance > 1 && get_balance_factor(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // RR 회전
    if (balance < -1 && get_balance_factor(node->right) <= 0)
        return rotate_left(node);

    // RL 회전
    if (balance < -1 && get_balance_factor(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

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
    pq->root = insert_node(pq->root, data);
}

// 우선순위 큐의 삭제 함수 (dequeue)
unsigned char dequeue(priority_queue *pq) {
    if (pq == NULL || pq->root == NULL) {
        fprintf(stderr, "우선순위 큐가 비어 있습니다.\n");
        exit(EXIT_FAILURE);
    }

    Node *maxNode = find_max(pq->root);
    unsigned char maxValue = maxNode->data;
    pq->root = delete_max(pq->root);
    return maxValue;
}

// 우선순위 큐의 루트 노드를 반환하는 함수
Node *get_root(priority_queue *pq) {
    if (pq == NULL) {
        fprintf(stderr, "우선순위 큐가 초기화되지 않았습니다.\n");
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

off_t find_offset(char *file_name) {
    int fd = open(file_name, O_RDONLY);
    unsigned char bytes[4];

    lseek(fd, 10, SEEK_SET);
    for (int i = 0; i < 4; ++i) {
        read(fd, &bytes[i], 1);
    }
    off_t offset = bytes[0] | ((off_t) bytes[1] << 8) | ((off_t) bytes[2] << 16) | ((off_t) bytes[3] << 24);
    close(fd);

    return offset;
}

// int find_size(int n, char *file_name) {
//     int fd = open(file_name, O_RDONLY);
//     if (fd == -1) {
//         perror("open");
//         exit(EXIT_FAILURE);
//     }
//
//     off_t offset = find_offset(file_name);
//     unsigned char buf;
//     int size = 0;
//
//     if (lseek(fd, offset, SEEK_SET) != offset) {
//         perror("lseek");
//         close(fd);
//         exit(EXIT_FAILURE);
//     }
//     printf("Start Offset: %ld\n", offset);
//
//     while (1) {
//         ssize_t bytes_read = read(fd, (void *)&buf, 1);
//         if (bytes_read == -1) {
//             perror("read");
//             close(fd);
//             exit(EXIT_FAILURE);
//         }
//         if (bytes_read == 0) {
//             break;
//         }
//         size++;
//     }
//
//     close(fd);
//     printf("Size : %ld\n", size);
//
//     return size;
// }

int find_size(int n, char *file_name) {
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) { // 파일 열기 실패
        perror("파일 열기 실패");
        return -1; // 에러 반환
    }

    unsigned char buf;
    int size = 0;

    lseek(fd, 0, SEEK_SET);
    while (1) {
        ssize_t bytes_read = read(fd, &buf, 1);
        if (bytes_read == -1) { // 읽기 오류
            perror("읽기 오류");
            close(fd);
            return -1; // 에러 반환
        }
        if (bytes_read == 0) { // EOF
            break;
        }
        printf("%lld\n", bytes_read);
        size++;
    }
    printf("%d\n", size);
    printf("%d\n", lseek(fd, 0, SEEK_END));


    close(fd); // 파일 닫기
    return size; // 총 바이트 수 반환
}

