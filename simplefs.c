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
#define MAX_TREE_HEIGTH   255
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

/*
 * Given a path it returns an array of strings 
 * where every element is a subdirectory of the path,
 * this is achieved by splitting the path string at every '/'.
 *
 * path: string representing the path to be split.
 *
 * return: array of strings containing subdirs of path.
 */
char **split_path(char *path) {
    char **result = malloc(sizeof(char *));
    int depth = 0;
    char *token = NULL;
    char *path_copy = strdup(path);
   
    if(strcmp("/", path) == 0) {
        result[0] = strdup("/");
        return result;
    }

    if(*path_copy == '/') path_copy++;

    while((token = strsep(&path_copy, "/")) != NULL) {
        result[depth] = token; 
        depth++;
        result = realloc(result, depth * sizeof(char *));
    }
    result[depth] = NULL;
    return result; 
}

/*
 * Given a path it will return the last resource in it.
 * ATTENTION, path will be modified! 
 * The last resource returned as a value will be removed from it.
 *
 * path: string representing the path to be modified.
 *
 * returns: string after last '/' character.
 */
char *remove_last_entry_from_path(char *path) {
    char *tmp = path + strlen(path) - 1;
    
    if(*tmp == '/') {
        *tmp = '\0';
        tmp--;
    }
    
    while(*tmp != '/') tmp--; /* now tmp points to the first '/' */
    
    if(tmp == path) {
        char *result = strdup(tmp+1);
        *(path+1) = '\0';
        return result;
    }
    *tmp = '\0';
    return tmp+1;
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
    return 1;
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
                /* TOREMOVE */
                /* printf("%s %p\n", (iterator->sons)[i]->name, (void *)iterator); */
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
    char **directories = split_path(path);
    int i = 0;
    struct simplefs_node *nextup = root, *result;

    if(strcmp(directories[0], "/") == 0) return root;

    while(directories[i] != NULL){
        result = nextup;
        nextup = simplefs_find_son(nextup, directories[i]);
        i++;
    }
    return nextup;
}

int simplefs_add_son_to_node(struct simplefs_node *parent, struct simplefs_node *son) {
    int i = 0;
    struct simplefs_node_group *iterator = parent->dir;
    while(iterator != NULL) {
        if(iterator->free_sons > 0) {
            for(i = 0; i < SONS_PER_GROUP; i++) {
                if((iterator->sons)[i] == NULL) {
                    (iterator->sons)[i] = son;
                    iterator->free_sons--;
                    return 1;
                }
            }
        }
        iterator = iterator->next;
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

int simplefs_create_node(struct simplefs_node *root, char *path) {
    struct simplefs_node *parent_node;
    struct simplefs_node *new_node;
    char *new_node_name = NULL;

    
    new_node_name = remove_last_entry_from_path(path); 
    
    parent_node = simplefs_walk_path(root, path);
    
    if(parent_node == NULL) {
        return 0;
    }
    
    new_node = simplefs_init_node(parent_node, new_node_name, NULL, NULL);

    if(new_node == NULL) {
        return 0;
    } 

    if(simplefs_add_son_to_node(parent_node, new_node) == 0) {
        return 0;
    }

    return 1;
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
        printf("create called! %s\n", first_argument);
    } else if(strcmp("create_dir", command_name) == 0){
        printf("create_dir called!\n");    
    } else if(strcmp("read", command_name) == 0){
        printf("read called!\n");    
    } else if(strcmp("write", command_name) == 0){
        printf("write called!\n");    
    } else if(strcmp("delete", command_name) == 0){
        printf("delete called!\n");    
    } else if(strcmp("delete_r", command_name) == 0){
        printf("delete_r called!\n");    
    } else if(strcmp("find", command_name) == 0){
        printf("find called!\n");    
    } else if(strcmp("exit", command_name) == 0){
        printf("exit called!\n");    
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

    //printf("%s\n", split_path("/")[0]);
    
    char *path = strdup("/a/b/");
    printf("%s\n", remove_last_entry_from_path(path));

    struct simplefs_node *root;
    root = simplefs_init_node(NULL, "root", NULL, NULL);
   
    struct simplefs_node_group *root_dir;
    root_dir = simplefs_init_node_group(root, NULL);
    simplefs_add_group_to_node(root, root_dir);

    int i = 0;
    char a[10] = "/aX";
    for(i = 0; i < 1040; i++) {
        sprintf(a, "/a%d", i);
        printf("%d: ", i);
        if(simplefs_create_node(root, a) == 0) printf("no\n");
        else printf("ok\n");
    }
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

    /*
    while ((read = getline(&line, &len, stdin)) != -1) {
        number_of_arguments = simplefs_parse_command_line(line, &command_name, &first_argument, &second_argument);
        simplefs_execute_command(root, command_name, first_argument, second_argument); 
        command_name = NULL;
        first_argument = NULL;
        second_argument = NULL;
    }

    if (line)
        free(line);
    */
    exit(EXIT_SUCCESS);
}
