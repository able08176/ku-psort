//
// Created by Admin on 2024-11-27.
//

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

// AVL 트리 노드 정의
typedef struct AVLNode {
    unsigned char data;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

// 우선순위 큐 구조 정의
typedef struct PriorityQueue {
    AVLNode *root;
} PriorityQueue;

typedef struct thread_argument {
    char *filename;
    PriorityQueue *priorityQueue;
    off_t offset;
    off_t quota;
} thread_argument;

// 노드의 높이 반환
int getHeight(AVLNode *node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// 두 정수 중 큰 값을 반환
int max(int a, int b) {
    return (a > b) ? a : b;
}

// 새로운 AVL 노드 생성
AVLNode *createNode(unsigned char data) {
    AVLNode *node = (AVLNode *) malloc(sizeof(AVLNode));
    if (!node) {
        fprintf(stderr, "메모리 할당 실패\n");
        exit(EXIT_FAILURE);
    }
    node->data = data;
    node->left = node->right = NULL;
    node->height = 1; // 새로운 노드의 높이는 1
    return node;
}

// 오른쪽 회전
AVLNode *rightRotate(AVLNode *y) {
    if (y == NULL || y->left == NULL) {
        return y;
    }

    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    // 회전 수행
    x->right = y;
    y->left = T2;

    // 높이 업데이트
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    // 새로운 루트 반환
    return x;
}

// 왼쪽 회전
AVLNode *leftRotate(AVLNode *x) {
    if (x == NULL || x->right == NULL) {
        return x;
    }

    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    // 회전 수행
    y->left = x;
    x->right = T2;

    // 높이 업데이트
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    // 새로운 루트 반환
    return y;
}

// 균형 인수 계산
int getBalance(AVLNode *node) {
    if (node == NULL)
        return 0;
    return getHeight(node->left) - getHeight(node->right);
}

// AVL 트리에 데이터 삽입
AVLNode *insertNode(AVLNode *node, unsigned char data) {
    // 일반적인 BST 삽입
    if (node == NULL)
        return createNode(data);

    if (data < node->data)
        node->left = insertNode(node->left, data);
    else
        node->right = insertNode(node->right, data);

    // 노드의 높이 업데이트
    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    // 균형 인수 계산
    int balance = getBalance(node);

    // 불균형이 발생한 경우 4가지 경우를 처리

    // 왼쪽 왼쪽 경우
    if (balance > 1 && data < node->left->data)
        return rightRotate(node);

    // 오른쪽 오른쪽 경우
    if (balance < -1 && data > node->right->data)
        return leftRotate(node);

    // 왼쪽 오른쪽 경우
    if (balance > 1 && data > node->left->data) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // 오른쪽 왼쪽 경우
    if (balance < -1 && data < node->right->data) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    // 균형이 맞으면 노드 반환
    return node;
}

// AVL 트리에서 최소값 노드 찾기
AVLNode *findMinNode(AVLNode *node) {
    AVLNode *current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

// AVL 트리에서 노드 삭제
AVLNode *deleteNode(AVLNode *root, unsigned char data) {
    // 일반적인 BST 삭제
    if (root == NULL)
        return root;

    if (data < root->data)
        root->left = deleteNode(root->left, data);
    else if (data > root->data)
        root->right = deleteNode(root->right, data);
    else {
        // 노드 하나 또는 자식이 없는 경우
        if ((root->left == NULL) || (root->right == NULL)) {
            AVLNode *temp = root->left ? root->left : root->right;

            // 자식이 없는 경우
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else {
                // 자식이 한 개 있는 경우
                *root = *temp;
            }

            free(temp);
        } else {
            // 두 자식이 있는 경우: 오른쪽 서브트리에서 최소값 노드를 찾아 대체
            AVLNode *temp = findMinNode(root->right);
            root->data = temp->data;
            root->right = deleteNode(root->right, temp->data);
        }
    }

    // 노드가 하나인 경우
    if (root == NULL)
        return root;

    // 노드의 높이 업데이트
    root->height = 1 + max(getHeight(root->left), getHeight(root->right));

    // 균형 인수 계산
    int balance = getBalance(root);

    // 불균형이 발생한 경우 4가지 경우를 처리

    // 왼쪽 왼쪽 경우
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    // 왼쪽 오른쪽 경우
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // 오른쪽 오른쪽 경우
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    // 오른쪽 왼쪽 경우
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

// 우선순위 큐 초기화
PriorityQueue *createPriorityQueue() {
    PriorityQueue *pq = (PriorityQueue *) malloc(sizeof(PriorityQueue));
    if (!pq) {
        fprintf(stderr, "메모리 할당 실패\n");
        exit(EXIT_FAILURE);
    }
    pq->root = NULL;
    return pq;
}

// 우선순위 큐가 비어있는지 확인
int isEmpty(PriorityQueue *pq) {
    return pq->root == NULL;
}

// 우선순위 큐에 데이터 삽입 (오름차순 유지)
void enqueue(PriorityQueue *pq, unsigned char data) {
    pq->root = insertNode(pq->root, data);
}

// 우선순위 큐에서 가장 우선순위가 높은 데이터 제거 및 반환
unsigned char dequeue(PriorityQueue *pq) {
    if (isEmpty(pq)) {
        fprintf(stderr, "큐가 비어있습니다.\n");
        exit(EXIT_FAILURE);
    }

    // 가장 작은 값을 찾기
    AVLNode *minNode = findMinNode(pq->root);
    unsigned char minData = minNode->data;

    // 해당 노드 삭제
    pq->root = deleteNode(pq->root, minData);

    return minData;
}

// 우선순위 큐의 가장 우선순위가 높은 데이터 확인
unsigned char peek(PriorityQueue *pq) {
    if (isEmpty(pq)) {
        fprintf(stderr, "큐가 비어있습니다.\n");
        exit(EXIT_FAILURE);
    }
    AVLNode *minNode = findMinNode(pq->root);
    return minNode->data;
}

// AVL 트리 메모리 해제
void freeAVLTree(AVLNode *node) {
    if (node == NULL)
        return;
    freeAVLTree(node->left);
    freeAVLTree(node->right);
    free(node);
}

// 우선순위 큐 전체 삭제
void destroyPriorityQueue(PriorityQueue *pq) {
    freeAVLTree(pq->root);
    free(pq);
}

// 우선순위 큐 중위 순회 출력 (디버깅용)
void inorderTraversal(AVLNode *node) {
    if (node == NULL)
        return;
    inorderTraversal(node->left);
    printf("%u ", node->data);
    inorderTraversal(node->right);
}

void printPriorityQueue(PriorityQueue *pq) {
    printf("Priority Queue (오름차순): ");
    inorderTraversal(pq->root);
    printf("\n");
}

off_t getStartOffset(char *filename) {
    int fd = open(filename, O_RDONLY | O_BINARY);
    unsigned char bytes[4];

    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    lseek(fd, 10, SEEK_SET);
    for (int i = 0; i < 4; ++i) {
        read(fd, &bytes[i], 1);
    }
    off_t offset = bytes[0] | ((off_t) bytes[1] << 8) | ((off_t) bytes[2] << 16) | ((off_t) bytes[3] << 24);

    close(fd);

    return offset;
}

off_t getDataSize(char *filename) {
    int fd = open(filename, O_RDONLY | O_BINARY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    off_t size = lseek(fd, 0, SEEK_END);
    off_t start = getStartOffset(filename);
    size = size - start;

    close(fd);

    return size;
}

void *thread_function(void *arg) {
    thread_argument *argument = (thread_argument *) arg;
    int fd;
    unsigned char buf;

    fd = open(argument->filename, O_RDONLY | O_BINARY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    lseek(fd, argument->offset, SEEK_SET);
    pthread_mutex_lock(&mutex);
    off_t i;
    for (i = 0; i < argument->quota; ++i) {
        if (read(fd, &buf, 1) == -1) {
            if (errno == EINTR) {
                i--;
                continue;
            }
            perror("read");
            close(fd);
            exit(EXIT_FAILURE);
        }
        enqueue(argument->priorityQueue, buf);
    }
    pthread_mutex_unlock(&mutex);
    close(fd);

    return NULL;
}

int main() {
    int n = 1;
    char *input = "673aef41575027558828.bmp";
    char *output = "output.bmp";
    PriorityQueue *pq = createPriorityQueue();
    thread_argument **thread_args;
    pthread_t *thread_ids;
    int status;
    int read_fd;
    int write_fd;
    unsigned char buf;

    thread_args = (thread_argument **) malloc(sizeof(thread_argument) * n);
    for (int i = 0; i < n; ++i) {
        thread_args[i] = (thread_argument *) malloc(sizeof(thread_argument));
    }

    off_t size = getDataSize(input);
    off_t quota = size / n;
    off_t start = getStartOffset(input);
    for (int i = 0; i < n - 1; ++i) {
        thread_args[i]->filename = input;
        thread_args[i]->priorityQueue = pq;
        thread_args[i]->offset = start + i * quota;
        thread_args[i]->quota = quota;
    }
    thread_args[n - 1]->filename = input;
    thread_args[n - 1]->priorityQueue = pq;
    thread_args[n - 1]->offset = start + (n - 1) * quota;
    thread_args[n - 1]->quota = size - (n - 1) * quota;

    thread_ids = (pthread_t *) malloc(sizeof(pthread_t) * n);
    for (int i = 0; i < n; ++i) {
        status = pthread_create(&thread_ids[i], NULL, thread_function, thread_args[i]);

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

    read_fd = open(input, O_RDONLY | O_BINARY);
    write_fd = open(output, O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, 0777);
    if (read_fd == -1 || write_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    off_t i;
    for (i = 0; i < start; ++i) {
        if (read(read_fd, &buf, 1) == -1) {
            if (errno == EINTR) {
                i--;
                continue;
            }
            perror("read");
            close(read_fd);
            close(write_fd);
            exit(EXIT_FAILURE);
        }

        if (write(write_fd, &buf, 1) == -1) {
            perror("write");
            close(read_fd);
            close(write_fd);
            exit(EXIT_FAILURE);
        }
    }

    while (pq->root != NULL) {
        buf = dequeue(pq);
        if (write(write_fd, &buf, 1) == -1) {
            perror("write");
            close(read_fd);
            close(write_fd);
            exit(EXIT_FAILURE);
        }
    }

    close(read_fd);
    close(write_fd);
    pthread_mutex_destroy(&mutex);
    free(thread_ids);
    for (int i = 0; i < n; ++i) {
        free(thread_args[i]);
    }
    free(thread_args);
    destroyPriorityQueue(pq);

    return 0;
}
