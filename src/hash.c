#include "hash.h"
#include "common.h"
#include <assert.h>

typedef struct hash_node {
	void *key;
	void *value;
	struct hash_node *prev;
	struct hash_node *next;
} hash_node_t;


struct hash {
	unsigned int buckets;
	hashfunc_t hash_func;
	hash_node_t **nodes;
};

hash_node_t** hash_get_bucket(hash_t *hash, void *key);
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size);



hash_t* hash_alloc(unsigned int buckets, hashfunc_t hash_func)//创建hash
{
    hash_t * hash = (hash_t*)malloc(sizeof(hash_t));
    hash->buckets = buckets;
    hash->hash_func = hash_func;
    int size = buckets*sizeof(hash_node_t*);
    hash->nodes = (hash_node_t**)malloc(size);
    memset(hash->nodes,0,size);
    return hash;
}
void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size)   //在hash中查找
{
    hash_node_t *node = hash_get_node_by_key(hash,key,key_size);
    if(node==NULL){
        return NULL;
    }
    return node->value;
}
void hash_add_entry(hash_t *hash, void *key, unsigned int key_size, void *value, unsigned int value_size) //往hash中添加一项
{
    if(hash_lookup_entry(hash,key,key_size))    //是否已经存在
    {
        fprintf(stderr,"duplicate hash key\n");
        return;
    }
    hash_node_t *node = malloc(sizeof(hash_node_t));
    node->prev=NULL;
    node->next=NULL;

    node->key = malloc(key_size);
    memcpy(node->key,key,key_size);

    node->value = malloc(value_size);
    memcpy(node->value,value,value_size);

    //将节点的添加  后继 前驱 
    hash_node_t**bucket = hash_get_bucket(hash,key);
    if(*bucket==NULL){
        *bucket = node;
    }else{
        //头插法

        node->next = *bucket;
        (*bucket)->prev = node;
        *bucket = node;
    }
    
}
void hash_free_entry(hash_t *hash, void *key, unsigned int key_size) //删除
{
    //释放节点      先找到，在释放
    hash_node_t*node =  hash_get_node_by_key(hash,key,key_size);
    if(node==NULL){
        return;
    }
    free(node->key);
    free(node->value);
    //找到     
    if(node->prev)
    {
        node->prev->next = node->next;
    }else{  //如果删除的时第一个节点
        hash_node_t **bucket = hash_get_bucket(hash,key);
        *bucket = node->next;
    }
    if(node->next)
    {
        node->next->prev=node->prev;  
    }
    free(node);
}

hash_node_t** hash_get_bucket(hash_t *hash, void *key)  //得到桶号
{
    unsigned int bucket = hash->hash_func(hash->buckets,key);   //通过hash函数找到tongu桶号
    if(bucket>=hash->buckets){
        fprintf(stderr,"bad bucket lookup\n");
        exit(EXIT_FAILURE);
    }
    return &(hash->nodes[bucket]);  //返回这个指针的地址
}
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size)
{
    hash_node_t**bucket = hash_get_bucket(hash,key);
    hash_node_t*node = *bucket;
    if(node==NULL){
        return NULL;
    }
    while(node&&memcmp(node->key,key,key_size)!=0){
        node = node->next;
    }
    return node;
}
