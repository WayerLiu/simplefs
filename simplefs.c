/*
 * SimpleFS, un semplice filesystem
 */


/* To avoid getline warning during compilation */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

//#define BENCHMARK
#define DEBUG
/*
 * Imposed restrictions.
 */
#define MAX_NAME_LENGTH   255
#define MAX_TREE_HEIGHT   254
#define MAX_SONS_PER_NODE 1024

/*
 * Every directory can have no more than MAX_SONS_PER_NODE sons.
 * Since that number could be very big and we don't want to waste memory,
 * we use a dobule linked list of groups of sons, growing it dinamically.
 * SONS_PER_GROUP represents the number of sons in each group.
 * GROUPS_PER_NODE represents the number of groups for each directory,
 * that is the maximum length of the linked list.
 */ 
#define SONS_PER_GROUP 64
#define GROUPS_PER_NODE MAX_SONS_PER_NODE/SONS_PER_GROUP

struct simplefs_node {
    /*
     * Parent of the node.
     * Only NULL for the root.
     */
    struct simplefs_node *parent;
    
    /* 
     * Name of the resource.
     */
    char name[MAX_NAME_LENGTH];
   
    /*
     * If the node is a directory, dir will point to the group.
     * Otherwise it will be NULL.
     */ 
    struct simplefs_node_group *dir;
    int number_of_groups;
    /*
     * If the node is a file, file_content will contain the content of the file.
     * Otherwise it will be NULL.
     */
    char *file_content;
};

struct simplefs_node_group {
    /* struct simplefs_node_group *previous; */
    struct simplefs_node *parent;

    struct simplefs_node_group *next;
    
    /* sons is an array of pointers to nodes. */
    struct simplefs_node *sons[SONS_PER_GROUP];
    
    /* free_sons keeps track of how many "free slots" are remained */
    int free_sons;
};

struct sll_node {
    char *data;
    struct sll_node *next;
};

int split_path(char *path, char destination[MAX_NAME_LENGTH][MAX_TREE_HEIGHT]) {
    int depth = 0;
    char *token = NULL;
    char *path_copy = strdup(path);  
    char *c;
    
    if(strcmp("/", path) == 0) {
        return 0;
    }
    
    path_copy++;
    c = path_copy + strlen(path_copy) - 1;
    if(*c == '/') *c = '\0';    
    
    while((token = strsep(&path_copy, "/")) != NULL) {
        if(strlen(token) > MAX_NAME_LENGTH) {
            return -2;
        }
        strcpy(destination[depth], token); 
        depth++;
        if(depth >= MAX_TREE_HEIGHT) {
            return -1;
        } 
    }
    
    return depth; 
}

char *get_last_path_entry(char *path) {
    int length = strlen(path);
    if( (length > 1) && *(path+length-1) != '/' ) {
        return strdup(strrchr(path, '/')+1);
    } else {
        return NULL;
    }
}

char *remove_last_path_entry(char *path) {
    char *last_slash = NULL;
    int length = strlen(path);
    char *new_path = strdup(path);
    if(length == 1) {
        return new_path;
    }
    
    last_slash = strrchr(new_path, '/');
    
    if(last_slash == new_path) {
        char *t = malloc(sizeof(char) * 2);
        t[0] = '/';
        t[1] = '\0';
        return t;
        //return strdup("/");
    }
    
    *last_slash = '\0';
    return strdup(new_path);
}

/*
 * Initialize a new node.
 */
struct simplefs_node *simplefs_init_node(struct simplefs_node *parent, 
                                         char *name,
                                         struct simplefs_node_group *dir,
                                         char *file_content) {
    if(strlen(name) > MAX_NAME_LENGTH) {
        return NULL;
    }
    
    struct simplefs_node *node = NULL;
    node = malloc(sizeof(struct simplefs_node));
    if(node == NULL) {
        return NULL;
    }

    node->parent = parent;
    
    strcpy(node->name, name); 
    /* 
     * Adding the first group.
     * Iterating every group to find how many of them there are.
     */
    node->dir = dir;
    node->number_of_groups = 0;
    if(dir != NULL) {
        int i = 1;
        struct simplefs_node_group *iterator = node->dir;
        while(iterator->next != NULL) {
            iterator = iterator->next;
            i++;
        }
        node->number_of_groups = i;
    }
    
    if(file_content != NULL) {
        node->file_content = strdup(file_content);
    }

    return node;  
}

struct simplefs_node_group *simplefs_init_node_group(struct simplefs_node *parent,
                                                     struct simplefs_node_group *next) {

    struct simplefs_node_group *new_group = NULL;
    new_group = malloc(sizeof(struct simplefs_node_group));
    if(new_group == NULL) {
        return NULL;
    }
    new_group->parent = parent;
    new_group->next = next;
    new_group->free_sons = SONS_PER_GROUP;
    
    /* point every son to NULL */
    int i = 0;
    for(i = 0; i < SONS_PER_GROUP; i++) {
        (new_group->sons)[i] = NULL;
    }

    return new_group;
}

int simplefs_update_file_node(struct simplefs_node *node, char *content) {
    if( (node->file_content = strdup(content)) == NULL) {
        return 0;
    }
    return strlen(content);
}

int simplefs_add_group_to_node(struct simplefs_node *node,
                               struct simplefs_node_group *group) {
    node->number_of_groups++;

    if(group == NULL || node->number_of_groups > GROUPS_PER_NODE) {
        return 0;
    }

    group->parent = node;

    if(node->dir == NULL) {
        node->dir = group;
        return 1;
    }

    struct simplefs_node_group *iterator;
    iterator = node->dir;
    while(iterator->next != NULL){
        iterator = iterator->next;
    }
    iterator->next = group;
    
    return 1;
}

struct simplefs_node *simplefs_find_son(struct simplefs_node *node, char *name) {
    if(node->number_of_groups == 0 || 
       node->dir == NULL || 
       strlen(name) > MAX_NAME_LENGTH) 
    {
        return NULL;
    }
    int i = 0;
    struct simplefs_node_group *iterator = node->dir;
    while(iterator != NULL) {
        for(i = 0; i < SONS_PER_GROUP; i++) {
            if((iterator->sons)[i] != NULL) {
                if(strcmp((iterator->sons)[i]->name, name)==0) {
                    return (iterator->sons)[i];
                }
            }
        }
        iterator = iterator->next;
    }
    return NULL;
}


struct simplefs_node *simplefs_walk_path(struct simplefs_node *root, char *path) {
    char directories[MAX_NAME_LENGTH][MAX_TREE_HEIGHT];
    int depth = 0;
    int i = 0;
    struct simplefs_node *nextup = root;

    depth = split_path(path, directories);

    if(depth == 0) return root;

    if(depth < 0) return NULL;

    for(i = 0; i < depth; i++){
        nextup = simplefs_find_son(nextup, directories[i]);
        if(nextup == NULL) break;
    }
    return nextup;
}

int simplefs_add_son_to_node(struct simplefs_node *parent, struct simplefs_node *son) {
    int i = 0, first_free_son = MAX_SONS_PER_NODE + 1;
    struct simplefs_node_group *iterator = parent->dir, 
                               *group_containing_first_free_son = NULL;
    
    while(iterator != NULL) {
        if(iterator->free_sons > 0) {
            for(i = 0; i < SONS_PER_GROUP; i++) {
                if((iterator->sons)[i] == NULL && first_free_son == MAX_SONS_PER_NODE + 1) {
                    first_free_son = i;
                    group_containing_first_free_son = iterator;
                } else if((iterator->sons)[i] != NULL) {
                    if(strcmp(son->name, (iterator->sons)[i]->name) == 0) {
                        return 0;
                    }    
                }
            }
        }
        iterator = iterator->next;
    }

    if(group_containing_first_free_son != NULL) {
        (group_containing_first_free_son->sons)[first_free_son] = son;
        group_containing_first_free_son->free_sons--;
        return 1;
    }

    if(parent->number_of_groups < GROUPS_PER_NODE){
        struct simplefs_node_group *new_group;
        new_group = simplefs_init_node_group(parent, NULL);
        if(new_group == NULL) {
            return 0;
        }
        if(simplefs_add_group_to_node(parent, new_group) == 0) {
            return 0;
        }
        return simplefs_add_son_to_node(parent, son);
    }
    return 0;
}

struct simplefs_node *simplefs_create_node(struct simplefs_node *root, char  *path) {
    struct simplefs_node *parent_node;
    struct simplefs_node *new_node;
    char *new_node_name = NULL, *new_path = NULL;

    new_node_name = get_last_path_entry(path);
    new_path = remove_last_path_entry(path); 

    parent_node = simplefs_walk_path(root, new_path);
    if(parent_node == NULL) {
        return NULL;
    }
    
    new_node = simplefs_init_node(parent_node, new_node_name, NULL, NULL);

    if(new_node == NULL) {
        return NULL;
    } 

    if(simplefs_add_son_to_node(parent_node, new_node) == 0) {
        return NULL;
    }

    return new_node;
}

struct simplefs_node *simplefs_create_dir(struct simplefs_node *root, char *path) {
    struct simplefs_node *new_node = NULL;
    struct simplefs_node_group *new_group = NULL;

    new_node = simplefs_create_node(root, path);
    if(new_node == NULL) {
        return NULL;
    }

    new_group = simplefs_init_node_group(new_node, NULL);
    if(simplefs_add_group_to_node(new_node, new_group) == 0) {
        return NULL;
    }
    return new_node;
}

char *simplefs_read(struct simplefs_node *root, char *path) {
    struct simplefs_node *target = NULL;

    target = simplefs_walk_path(root, path);

    if(target == NULL) {
        return NULL;
    } else if(target->dir != NULL) {
        return NULL;
    }

    return target->file_content;
}

int simplefs_write(struct simplefs_node *root, char *path, char *content) {
    struct simplefs_node *target = NULL;
    
    target = simplefs_walk_path(root, path);

    if(target == NULL) {
        return 0;
    } else if(target->dir != NULL) {
        return 0;
    }

    return simplefs_update_file_node(target, content);
}

int simplefs_get_free_sons_for_node(struct simplefs_node *node) {
    struct simplefs_node_group *iterator;
    int free_sons = 0;
        
    if(node->dir == NULL) {
        if(node->file_content != NULL) { 
            return -1;
        } else {
            return -2;
        }
    }

    iterator = node->dir;

    while(iterator != NULL) {
        free_sons += iterator->free_sons;
        iterator = iterator->next;
    }

    return free_sons + ((GROUPS_PER_NODE - node->number_of_groups) * SONS_PER_GROUP);
}

int simplefs_delete_node(struct simplefs_node *node) {
    struct simplefs_node_group *iterator = NULL;
    int i = 0;
    int free_sons = simplefs_get_free_sons_for_node(node); 
    if(free_sons < 0 || free_sons == MAX_SONS_PER_NODE) {
        /*
         * freeing target from parent.
         */
        iterator = node->parent->dir;
        while(iterator != NULL) {
            for(i = 0; i < SONS_PER_GROUP; i++) {
                if((iterator->sons)[i] != NULL) {
                    if(strcmp((iterator->sons)[i]->name, node->name)==0) {
                        (iterator->sons)[i] = NULL;
                        break;
                    }
                }
            }
            iterator = iterator->next;
        }
       
        
        if(free_sons == -1) { 
            return 1;
        }
        
        node = NULL;
        return 1;
    } else {
        return 0;
    }
}

int simplefs_delete_node_recursive(struct simplefs_node *node) {
    struct simplefs_node_group *iterator = NULL;
    struct simplefs_node *tmp = NULL;
    int i = 0; 
    int result;
    
    node->dir = NULL;
    return simplefs_delete_node(node);
}

int simplefs_delete(struct simplefs_node *root, char *path) {
    struct simplefs_node *target = NULL;

    target = simplefs_walk_path(root, path);

    if(target == NULL) {
        return 0;
    }

    return simplefs_delete_node(target);
}

int simplefs_delete_recursive(struct simplefs_node *root, char *path) {
    struct simplefs_node *tmp = NULL;
    
    tmp = simplefs_walk_path(root, path); 
    if(tmp == NULL) {
       return 0;
    }
    return simplefs_delete_node_recursive(tmp);
}

struct sll_node *init_sll_node(struct sll_node *next, char *data) {
    struct sll_node *new_node = NULL;
    new_node = malloc(sizeof(struct sll_node));
    new_node->next = next;
    new_node->data = malloc(sizeof(char) * strlen(data));
    strcpy(new_node->data, data);
    return new_node;
}

void print_sll(struct sll_node *start) {
    struct sll_node *i = NULL;
    i = start;
    if(i == NULL) {
        printf("no\n");
    }
    while(i != NULL) {
        printf("ok %s\n", i->data);
        i = i->next;
    }
}

int add_to_sll(struct sll_node **start_node, char *data) {
    struct sll_node *node, *iterator, *new_node;
    int comparison;
    
    node = *start_node;
    iterator = node;
    
    new_node = init_sll_node(NULL, data);
    if(new_node == NULL) return 0;

    if(iterator->next == NULL) {
        iterator->next = new_node;
        return 1;
    }

    if(strcmp(data, node->data) < 0) {
        new_node->next = *start_node;
        *start_node = new_node;
        return 1;
    }

    while(iterator->next != NULL) {
        comparison = strcmp(data, iterator->next->data);
        if(comparison < 0) {
            new_node->next = iterator->next;
            iterator->next = new_node;
            return 1;
        } else if(comparison > 0) {
            iterator = iterator->next;
        } else {
            return 0;
        }
    }
    iterator->next = new_node;
    return 1;
}


void simplefs_find(struct simplefs_node *node, char *name, struct sll_node **s, char *full_path) {
    char g_path[5000];
    int i = 0;
    struct simplefs_node_group *iterator = NULL;
    memset(g_path, 0, 5000);
    iterator = node->dir;
    while(iterator != NULL) {
        for(i = 0; i < SONS_PER_GROUP; i++) {
            if((iterator->sons)[i] != NULL) {
                strcpy(g_path, full_path);
                strcat(g_path, "/");
                strcat(g_path, (iterator->sons)[i]->name);
                if(strcmp((iterator->sons)[i]->name,name) == 0) {
                    add_to_sll(s, g_path);
                }
                simplefs_find((iterator->sons)[i], name, s, g_path);
            }
        }
        iterator = iterator->next;
    }
    return;
        
}
/*
 * Parsing a line representing a command.
 * The first argument is the line to be parsed; 
 * the second is the command itself and the other two are the two arguments.
 * A double pointer is needed because no memory is going to be allocated in here:
 * command_name, first_argument and second_argument are pointers to a region of the memory 
 * pointed by line.
 */
int simplefs_parse_command_line(char *line, char **command_name, char **first_argument, char **second_argument) {
    int number_of_arguments = 0;
    *command_name = line;
    while(*line != '\0'){
        /* 
         * The strategy is to iterare every character.
         * When a space is found it may be possible that an argument will follow
         * and we have to search for it.
         * When a character different from the space or the terminator is found,
         * we point the arguments respectively to it.
         */
        if(*line == ' ') {
            *line = '\0';
            if(*(line+1) != ' ' && *(line+1) != '\0') {
                number_of_arguments++;
                if(number_of_arguments == 1) {
                    *first_argument = line+1;
                } else if(number_of_arguments == 2) {
                    *second_argument = line+1;
                }
            }
        } 
        line++;
    }
    *(line-1) = '\0';
    return number_of_arguments; 
}

int simplefs_execute_command( struct simplefs_node *root, char *command_name, char *first_argument, char *second_argument) {
    if(strcmp("create", command_name) == 0){
        if(simplefs_create_node(root, first_argument) != NULL) {
            simplefs_write(root, first_argument, "");
            printf("ok\n");
        } else {
            printf("no\n");
        }
    } else if(strcmp("create_dir", command_name) == 0){
        if(simplefs_create_dir(root, first_argument) != NULL) {
            printf("ok\n");
        } else {
            printf("no\n");
        }
    } else if(strcmp("read", command_name) == 0){
        char *content = NULL;
        content = simplefs_read(root, first_argument);
        if(content == NULL) {
            printf("no\n");
        } else {
            printf("contenuto %s\n", content);
        }
    } else if(strcmp("write", command_name) == 0){
        second_argument++;
        *(second_argument + strlen(second_argument) - 1) = '\0';
        int written_chars = simplefs_write(root, first_argument, second_argument);
        if(written_chars == 0) {
            printf("no\n");
        } else {
            printf("ok %d\n", written_chars);
        }
    } else if(strcmp("delete", command_name) == 0){
        if(simplefs_delete(root, first_argument) == 0) {
            printf("no\n");
        } else {
            printf("ok\n");
        }
    } else if(strcmp("delete_r", command_name) == 0){
        if(simplefs_delete_recursive(root, first_argument) == 0) {
            printf("no\n");
        } else {
            printf("ok\n");
        }

    } else if(strcmp("find", command_name) == 0){
        struct sll_node *n = init_sll_node(NULL, "");
        simplefs_find(root, first_argument, &n, ""); 
        n = n->next;
        print_sll(n);
    } else if(strcmp("exit", command_name) == 0){
        exit(EXIT_SUCCESS);
    }
}

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
   
    char *command_name = NULL;
    char *first_argument = NULL;
    char *second_argument = NULL;
    
    int number_of_arguments = 0;


    struct simplefs_node *root;
    root = simplefs_init_node(NULL, "root", NULL, NULL);
   
    struct simplefs_node_group *root_dir;
    root_dir = simplefs_init_node_group(root, NULL);
    simplefs_add_group_to_node(root, root_dir);

    struct timespec start, end;
    int i = 1;
    while ((read = getline(&line, &len, stdin)) != -1) {
        number_of_arguments = simplefs_parse_command_line(line, &command_name, &first_argument, &second_argument);
#ifdef BENCHMARK
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif
        simplefs_execute_command(root, command_name, first_argument, second_argument); 
#ifdef BENCHMARK
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        printf("%ld\n\n", end.tv_nsec - start.tv_nsec);
#endif
        command_name = NULL;
        first_argument = NULL;
        second_argument = NULL;
        i++;
    }

    if (line)
        free(line);
   
    exit(EXIT_SUCCESS);
}

