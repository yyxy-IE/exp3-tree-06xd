/**
 * 实验：目录树查看器（仿 Linux tree 命令）
 * 学号：2504020406 姓名：王文瑕
 * 说明：已补全所有 TODO，可直接编译运行，匹配实验要求
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// ================== 二叉树结点定义 ==================
typedef struct FileNode {
    char *name;                  // 文件/目录名
    int isDir;                   // 1:目录 0:文件
    struct FileNode *firstChild; // 左孩子：第一个子项
    struct FileNode *nextSibling;// 右兄弟：下一个同层项
} FileNode;

// ================== 函数声明 ==================
FileNode* createNode(const char *name, int isDir);
int cmpNode(const void *a, const void *b);
FileNode* buildTree(const char *path);
void printTree(FileNode *node, const char *prefix, int isLast);
int countNodes(FileNode *root);
int countLeaves(FileNode *root);
int treeHeight(FileNode *root);
void countDirFile(FileNode *root, int *dirs, int *files);
void freeTree(FileNode *root);
char* getBaseName(void);

// ================== 函数实现 ==================

// 创建新结点
FileNode* createNode(const char *name, int isDir) {
    FileNode *node = (FileNode*)malloc(sizeof(FileNode));
    if (!node) return NULL;
    node->name = (char*)malloc(strlen(name) + 1);
    if (!node->name) { free(node); return NULL; }
    strcpy(node->name, name);
    node->isDir = isDir;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}

// 排序比较函数（按名称升序，目录优先）
int cmpNode(const void *a, const void *b) {
    FileNode *na = *(FileNode**)a;
    FileNode *nb = *(FileNode**)b;
    // 目录排在文件前面
    if (na->isDir != nb->isDir) {
        return nb->isDir - na->isDir;
    }
    // 同名则按字符串比较
    return strcmp(na->name, nb->name);
}

// 递归构建目录树
FileNode* buildTree(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return NULL;

    // 提取当前目录名
    const char *name = strrchr(path, '/');
    if (!name) name = path;
    else name++;

    FileNode *root = createNode(name, 1);
    if (!root) { closedir(dir); return NULL; }

    FileNode **children = NULL;
    int childCnt = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        FileNode *node = NULL;
        if (S_ISDIR(st.st_mode)) {
            node = buildTree(full);
        } else if (S_ISREG(st.st_mode)) {
            node = createNode(ent->d_name, 0);
        }
        if (node) {
            children = (FileNode**)realloc(children, (childCnt+1)*sizeof(FileNode*));
            children[childCnt++] = node;
        }
    }
    closedir(dir);

    // 排序并链接成兄弟链表
    if (childCnt > 0) {
        qsort(children, childCnt, sizeof(FileNode*), cmpNode);
        root->firstChild = children[0];
        for (int i=0; i<childCnt-1; i++) {
            children[i]->nextSibling = children[i+1];
        }
    }
    free(children);
    return root;
}

// 树形打印（和题目格式完全一致）
void printTree(FileNode *node, const char *prefix, int isLast) {
    if (!node) return;

    printf("%s%s%s", prefix, isLast ? "`-- " : "|-- ", node->name);
    if (node->isDir) printf("/");
    printf("\n");

    const char *newPrefix = isLast ? "    " : "|   ";
    char newPre[1024];
    snprintf(newPre, sizeof(newPre), "%s%s", prefix, newPrefix);

    FileNode *child = node->firstChild;
    int cnt = 0;
    for (FileNode *p=child; p; p=p->nextSibling) cnt++;

    int idx = 0;
    while (child) {
        printTree(child, newPre, ++idx == cnt);
        child = child->nextSibling;
    }
}

// 统计结点总数
int countNodes(FileNode *root) {
    if (!root) return 0;
    return 1 + countNodes(root->firstChild) + countNodes(root->nextSibling);
}

// 统计叶子结点数（无孩子的结点）
int countLeaves(FileNode *root) {
    if (!root) return 0;
    if (!root->firstChild) return 1 + countLeaves(root->nextSibling);
    return countLeaves(root->firstChild) + countLeaves(root->nextSibling);
}

// 计算树高度（根深度为1）
int treeHeight(FileNode *root) {
    if (!root) return 0;
    int maxH = 0;
    for (FileNode *p=root->firstChild; p; p=p->nextSibling) {
        int h = treeHeight(p);
        if (h > maxH) maxH = h;
    }
    return 1 + maxH;
}

// 统计目录数和文件数
void countDirFile(FileNode *root, int *dirs, int *files) {
    if (!root) return;
    if (root->isDir) (*dirs)++;
    else (*files)++;
    countDirFile(root->firstChild, dirs, files);
    countDirFile(root->nextSibling, dirs, files);
}

// 释放整棵树内存
void freeTree(FileNode *root) {
    if (!root) return;
    freeTree(root->firstChild);
    freeTree(root->nextSibling);
    free(root->name);
    free(root);
}

// 获取当前目录名
char* getBaseName(void) {
    char *cwd = getcwd(NULL, 0);
    if (!cwd) return NULL;
    char *name = strrchr(cwd, '/');
    if (name) name++;
    else name = cwd;

    char *ret = (char*)malloc(strlen(name)+1);
    strcpy(ret, name);
    free(cwd);
    return ret;
}

// ================== main函数 ==================
int main(int argc, char *argv[]) {
    char targetPath[1024];
    if (argc >= 2) {
        strncpy(targetPath, argv[1], sizeof(targetPath)-1);
        targetPath[sizeof(targetPath)-1] = '\0';
    } else {
        if (getcwd(targetPath, sizeof(targetPath)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }
    int len = strlen(targetPath);
    if (len > 0 && targetPath[len-1] == '/')
        targetPath[len-1] = '\0';

    struct stat st;
    if (stat(targetPath, &st) != 0) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: %s 不是目录\n", targetPath);
        return 1;
    }

    FileNode *root = buildTree(targetPath);
    if (!root) {
        fprintf(stderr, "无法构建目录树\n");
        return 1;
    }

    char *displayName = NULL;
    if (argc >= 2) {
        displayName = root->name;
    } else {
        displayName = getBaseName();
    }
    printf("%s/\n", displayName);
    if (argc < 2) free(displayName);

    FileNode *child = root->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) { childCount++; tmp = tmp->nextSibling; }

    int idx = 0;
    while (child) {
        int isLast = (++idx == childCount);
        printTree(child, "", isLast);
        child = child->nextSibling;
    }

    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d 个目录, %d 个文件\n", dirs-1, files); // 减去根目录本身
    printf("二叉树结点总数: %d\n", countNodes(root));
    printf("叶子结点数: %d\n", countLeaves(root));
    printf("树的高度: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}