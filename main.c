#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// -----------------------
// 1. 구조체 정의
// -----------------------

// 노드 구조체 정의
typedef struct Node {
    unsigned char data;    // 1바이트 데이터
    int height;
    struct Node* left;
    struct Node* right;
} Node;

// 우선순위 큐 구조체 정의
typedef struct PriorityQueue {
    Node* root;
} PriorityQueue;

// -----------------------
// 2. 함수 선언 (프로토타입)
// -----------------------

// 우선순위 큐 초기화
void initPriorityQueue(PriorityQueue* pq);

// 삽입 연산
void enqueue(PriorityQueue* pq, unsigned char data);

// 삭제 연산
unsigned char dequeue(PriorityQueue* pq);

// 루트 노드 반환
Node* getRoot(PriorityQueue* pq);

// 중위 순회 (디버깅 용도)
void inOrder(Node* root);

// 트리의 모든 노드 해제
void freeTree(Node* root);

// 내부 함수 선언 (헤더에 노출되지 않음)

// 노드의 높이 반환
static int getHeight(Node* node);

// 노드의 균형 인수 계산
static int getBalanceFactor(Node* node);

// 노드의 높이 갱신
static void updateHeight(Node* node);

// 오른쪽 회전
static Node* rotateRight(Node* y);

// 왼쪽 회전
static Node* rotateLeft(Node* x);

// 노드 생성
static Node* createNode(unsigned char data);

// 삽입 연산 (오름차순 정렬)
static Node* insertNode(Node* node, unsigned char data);

// 가장 작은 값의 노드 찾기 (최소 우선순위 큐)
static Node* findMin(Node* node);

// 삭제 연산 (최소 우선순위 큐에서 가장 작은 원소 제거)
static Node* deleteMin(Node* node);

// -----------------------
// 4. 메인 함수
// -----------------------

int main() {
//    PriorityQueue pq;
//    initPriorityQueue(&pq);
//
//    // 원소 삽입 (중복된 값 포함)
//    enqueue(&pq, 0x1E); // 30
//    enqueue(&pq, 0x0A); // 10
//    enqueue(&pq, 0x14); // 20
//    enqueue(&pq, 0x28); // 40
//    enqueue(&pq, 0x32); // 50
//    enqueue(&pq, 0x28); // 40 중복
//    enqueue(&pq, 0x32); // 50 중복
//
//    // 트리 출력
//    printf("우선순위 큐 (중위 순회): ");
//    inOrder(pq.root);
//    printf("\n");
//
//    // 루트 노드 반환 및 출력
//    Node* rootNode = getRoot(&pq);
//    if (rootNode != NULL) {
//        printf("우선순위 큐의 루트 노드 값: %02X\n", rootNode->data);
//    }
//
//    // 우선순위가 가장 높은 원소 제거 (가장 작은 값)
//    unsigned char minValue = dequeue(&pq);
//    printf("우선순위가 가장 높은 원소 제거: %02X\n", minValue);
//
//    // 다시 트리 출력
//    printf("우선순위 큐 (중위 순회): ");
//    inOrder(pq.root);
//    printf("\n");
//
//    // 추가적인 삭제 연산
//    minValue = dequeue(&pq);
//    printf("우선순위가 가장 높은 원소 제거: %02X\n", minValue);
//    printf("우선순위 큐 (중위 순회): ");
//    inOrder(pq.root);
//    printf("\n");
//
//    // 트리의 모든 노드 해제
//    freeTree(pq.root);

    PriorityQueue pq;
    initPriorityQueue(&pq);

    int fd = open("673aef4158a883920599.bmp", O_RDONLY);
    unsigned char buf;

    for (int i = 0; i < 30; ++i) {
        read(fd, &buf, 1);
        printf("%d\n", (int)buf);
    }
    printf("\n");

    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < 30; ++i) {
        read(fd, &buf, 1);
        enqueue(&pq, buf);
    }
    for (int i = 0; i < 30; ++i) {
        printf("%d\n", dequeue(&pq));
    }

    return 0;
}

// -----------------------
// 3. 함수 정의
// -----------------------

// 노드의 높이 반환
static int getHeight(Node* node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// 노드의 균형 인수 계산
static int getBalanceFactor(Node* node) {
    if (node == NULL)
        return 0;
    return getHeight(node->left) - getHeight(node->right);
}

// 노드의 높이 갱신
static void updateHeight(Node* node) {
    if (node == NULL)
        return;
    int leftHeight = getHeight(node->left);
    int rightHeight = getHeight(node->right);
    node->height = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
}

// 오른쪽 회전
static Node* rotateRight(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    // 회전 수행
    x->right = y;
    y->left = T2;

    // 높이 갱신
    updateHeight(y);
    updateHeight(x);

    return x; // 새로운 루트
}

// 왼쪽 회전
static Node* rotateLeft(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    // 회전 수행
    y->left = x;
    x->right = T2;

    // 높이 갱신
    updateHeight(x);
    updateHeight(y);

    return y; // 새로운 루트
}

// 노드 생성
static Node* createNode(unsigned char data) {
    Node* node = (Node*)malloc(sizeof(Node));
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
static Node* insertNode(Node* node, unsigned char data) {
    // 이진 탐색 트리 삽입
    if (node == NULL)
        return createNode(data);

    if (data < node->data)
        node->left = insertNode(node->left, data);
    else // 중복된 값은 오른쪽 서브트리에 삽입
        node->right = insertNode(node->right, data);

    // 높이 갱신
    updateHeight(node);

    // 균형 인수 계산
    int balance = getBalanceFactor(node);

    // LL 회전
    if (balance > 1 && data < node->left->data)
        return rotateRight(node);

    // RR 회전
    if (balance < -1 && data > node->right->data)
        return rotateLeft(node);

    // LR 회전
    if (balance > 1 && data > node->left->data) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    // RL 회전
    if (balance < -1 && data < node->right->data) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node; // 노드 포인터 반환
}

// 가장 작은 값의 노드 찾기 (최소 우선순위 큐)
static Node* findMin(Node* node) {
    Node* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

// 삭제 연산 (최소 우선순위 큐에서 가장 작은 원소 제거)
static Node* deleteMin(Node* node) {
    if (node == NULL)
        return node;

    if (node->left == NULL) {
        Node* temp = node->right;
        free(node);
        return temp;
    }

    node->left = deleteMin(node->left);

    // 높이 갱신
    updateHeight(node);

    // 균형 인수 계산
    int balance = getBalanceFactor(node);

    // 균형 유지
    // LL 회전
    if (balance > 1 && getBalanceFactor(node->left) >= 0)
        return rotateRight(node);

    // LR 회전
    if (balance > 1 && getBalanceFactor(node->left) < 0) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    // RR 회전
    if (balance < -1 && getBalanceFactor(node->right) <= 0)
        return rotateLeft(node);

    // RL 회전
    if (balance < -1 && getBalanceFactor(node->right) > 0) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

// 우선순위 큐 초기화
void initPriorityQueue(PriorityQueue* pq) {
    if (pq == NULL)
        return;
    pq->root = NULL;
}

// 우선순위 큐의 삽입 함수
void enqueue(PriorityQueue* pq, unsigned char data) {
    if (pq == NULL)
        return;
    pq->root = insertNode(pq->root, data);
}

// 우선순위 큐의 삭제 함수
unsigned char dequeue(PriorityQueue* pq) {
    if (pq == NULL || pq->root == NULL) {
        fprintf(stderr, "우선순위 큐가 비어 있습니다.\n");
        exit(EXIT_FAILURE);
    }

    Node* minNode = findMin(pq->root);
    unsigned char minValue = minNode->data;
    pq->root = deleteMin(pq->root);
    return minValue;
}

// 우선순위 큐의 루트 노드를 반환하는 함수
Node* getRoot(PriorityQueue* pq) {
    if (pq == NULL) {
        fprintf(stderr, "우선순위 큐가 초기화되지 않았습니다.\n");
        return NULL;
    }
    return pq->root;
}

// 중위 순회 (디버깅 용도) - 오름차순 출력
void inOrder(Node* root) {
    if (root == NULL)
        return;
    inOrder(root->left);
    printf("%02X ", root->data); // 16진수 두자리로 출력
    inOrder(root->right);
}

// 후위 순회를 이용하여 트리의 모든 노드 해제
void freeTree(Node* root) {
    if (root == NULL)
        return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}
