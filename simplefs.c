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
/*
 * Imposed restrictions.
 */
#define MAX_NAME_LENGTH   255
#define MAX_TREE_HEIGHT   255
#define MAX_SONS_PER_NODE 1024

/*
 * Every directory can have no more than MAX_SONS_PER_NODE sons.
 * Since that number could be very big and we don't want to waste memory,
 * we use a dobule linked list of groups of sons, growing it dinamically.
 * SONS_PER_GROUP represents the number of sons in each group.
 * GROUPS_PER_NODE represents the number of groups for each directory,
 * that is the maximum length of the linked list.
 */ 
#define SONS_PER_GROUP 128
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
        return strdup("/");
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
    char directories[255][255];
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
    }

    return target->file_content;
}

int simplefs_write(struct simplefs_node *root, char *path, char *content) {
    struct simplefs_node *target = NULL;
    
    target = simplefs_walk_path(root, path);

    if(target == NULL) {
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
            free(node->file_content); 
            return 1;
        }
        
        if(node->dir != NULL) {
            iterator = node->dir;
            while(iterator != NULL) {
                iterator = iterator->next;
                free(node->dir);
                node->dir = iterator;
            }
        }
        free(node);
        return 1;
    } else {
        return 0;
    }
}

int simplefs_delete_node_recursive(struct simplefs_node *node) {
    struct simplefs_node_group *iterator = NULL;
    struct simplefs_node *tmp = NULL;
    int i = 0; 

    iterator = node->dir; 
    while(iterator != NULL) {
        for(i = 0; i < SONS_PER_GROUP; i++) {
            if((iterator->sons)[i] != NULL) {
                if(simplefs_delete_node((iterator->sons)[i]) == 0) {
                    return simplefs_delete_node_recursive((iterator->sons)[i]);
                } 
            }
        }
        iterator = iterator->next;
    }
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
    struct simplefs_node_group *iterator = NULL;
    struct simplefs_node *tmp = NULL;
    int i = 0; 
    char *current_node_path;

    tmp = simplefs_walk_path(root, path); 
    if(tmp == NULL) {
       return 0;
    }
    return simplefs_delete_node_recursive(tmp);
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
        printf("find called!\n");    
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

    int i = 1;
    
    while ((read = getline(&line, &len, stdin)) != -1) {
        number_of_arguments = simplefs_parse_command_line(line, &command_name, &first_argument, &second_argument);
        simplefs_execute_command(root, command_name, first_argument, second_argument); 
        command_name = NULL;
        first_argument = NULL;
        second_argument = NULL;
        i++;
    }
    printf("\n");
    if (line)
        free(line);
    
    exit(EXIT_SUCCESS);
}
/*
    simplefs_create_dir(root, "/a");
    //simplefs_create_node(root, "/a/file1");

    int j = 0;
    char b[10];
    int r = 0;
    for(j=0;j<1029;j++){
        snprintf(b, 10, "/a/j%d", j); 
        if(j<1024) assert(simplefs_create_node(root, b)!=NULL);
        else assert(simplefs_create_node(root, b)==NULL);
        printf("%d:\t%d\t%d\n", j, simplefs_get_free_sons_for_node(root),root->number_of_groups);
        simplefs_write(root, b, "file di prova");
        
        //        assert(simplefs_walk_path(root, b) == NULL);
    }    

    simplefs_write(root, "/a/j2", "file di prova");
    //assert(simplefs_write(root, "/a/file1", "ciaociao")!=0); 

    //printf("%s\n", simplefs_read(root, "/a/file1"));
    //
    simplefs_create_dir(root, "/b");
    simplefs_create_node(root, "/b/b1");
    //simplefs_create_node(root, "/b/b2");
    //simplefs_create_node(root, "/b/b3");
    //simplefs_create_node(root, "/b/b4");
    printf("%d\n", simplefs_delete_recursive(root, "/a"));
    printf("%s\n", simplefs_read(root, "/a/j300"));
    int i = 0;
    char s[255][255];
    int depth = 0;
    
    //simplefs_walk_path(root, "/a/b/b/b/b/b/b/b/b");
    /*
    char a[10]; 
    for(i = 0; i < 1040; i++) {
        snprintf(a, 10, "/a%d", i);
        printf("%d\t%s: ", i, a);
        if((tmp=simplefs_create_node(root, a))!=NULL) {
            printf("ok\n");
            simplefs_update_file_node(tmp, "prova file");
        }
        else printf("no\n");
    }

    printf("%s %s\n", (root->dir->sons)[0]->name,(root->dir->sons)[0]->file_content); 
*/
    //sleep(60);
/*
    simplefs_create_node(root, strdup("/a"));
    printf("%s\n", simplefs_walk_path(root, "/a")->name);
    struct simplefs_node *root;
    struct simplefs_node_group *g1;
    struct simplefs_node_group *g2;
    g2 = simplefs_init_node_group(root, NULL);
    g1 = simplefs_init_node_group(root, g2);
    root = simplefs_init_node(NULL, "root", g1, NULL);
    if(root == NULL){
        printf("Errore nell'inizializzazione di root.");
        exit(EXIT_FAILURE);
    }


    printf("%d\n", root->dir->next->free_sons);
    
    int r = simplefs_update_file_node(root, "riprova");

    printf("%s %d\n", root->file_content, r);

    struct simplefs_node *n1,*n2;
    n1 = simplefs_init_node(root, "n1", NULL, "file n1");
    n2 = simplefs_init_node(root, "n2", NULL, "file n2");
    

    (root->dir->sons)[0] = n1;
    (g2->sons)[0] = n2;
    
    struct simplefs_node *foo = simplefs_init_node(NULL, "foo", NULL, NULL);
    int x = 0;
    for(x = 0; x < 40; x++){
        struct simplefs_node_group *tmp = simplefs_init_node_group(foo, NULL);
        printf("%d: %d\n", x, simplefs_add_group_to_node(foo, tmp));
    }

    printf("%d\n", simplefs_create_node(root, "/n1"));
  */  

    /*int i;
    for(i = 0; i<20; i++){
        printf("%d: %d\n", i, simplefs_add_new_group_to_node(root));
    }
    simplefs_add_new_group_to_node(root);
    printf("%d %d\n", root->dir->free_sons, root->number_of_groups);*/


