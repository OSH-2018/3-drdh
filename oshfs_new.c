#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>


/*
block安排
0：作为Super 元数据
1 - (BLOCK_NUM_FOR_NODE): Node_block
(BLOCK_NUM_FOR_NODE+1) - BLOCK_NUM : Data_block
*/
//4k page_size
#define BLOCK_SIZE (4*1024)
#define BLOCK_NUM (1024*1024)
#define NODE_NUM_PER_BLOCK 9
#define BLOCK_NUM_FOR_NODE 64
#define BLOCK_NUM_FOR_DATA (BLOCK_NUM - BLOCK_NUM_FOR_NODE -1)
#define CONTENT_SIZE_PER_BLOCK (BLOCK_SIZE-3*sizeof(int))
#define PATH_SIZE 4096
//#define MAX_READ_NUM (1024*1024) //栈会爆炸,每个文件最大的大小

#define DEBUG 1
/*
//4k page_size
size_t block_size=4*(size_t)1024;
//64M
int
block_num =16*1024;
int node_num_per_block=8;
*/

typedef struct Node{
	char name[256];
	char type;
	int first_data;
  int last_data;
	struct Node *parent;
	struct Node *child;
	struct Node *next;
	struct stat st;
	int block;
} Node;

typedef struct Node_block{
  int node_num;
  int bitmap[NODE_NUM_PER_BLOCK];
  Node node[NODE_NUM_PER_BLOCK];
}Node_block;

typedef struct Data_block{
	int prev_block;
  int next_block;
  int size;
  char content[CONTENT_SIZE_PER_BLOCK];
}Data_block;


typedef struct Super{
  int node_num;// <= NODE_NUM_PER_BLOCK * BLOCK_NUM_FOR_NODE
	int node_block;// <= BLOCK_NUM_FOR_NODE
	int data_num;// <= BLOCK_NUM - BLOCK_NUM_FOR_NODE -1
	//data_block=data_num
}Super;

static void *mem[BLOCK_NUM];

//root 在mem[1]的第一个位置
//使用全局变量便于操作
Node *root;
Super *super;
//总共的，使用mem[0]中的Super_block来储存
char newFile[256];

/*
//可用的
//static const size_t size = 4 * 1024 * 1024 * (size_t)1024;
//static void *mem[64 * 1024];//blocknum
//blocksize
mem[i] = mmap(NULL, blocksize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
munmap(mem[i], blocksize);

void *memcpy(void *dst, const void *src, size_t n)


//需要重新实现的
//针对node和文件内容
//void *Malloc(size_t n)
//for inode
Node *malloc_node()
void free_node(Node *p)
//for data
//
int malloc_data(void *p, size_t n)
void free_data(int block)
void read_data(void *dst, int block)
*/


Node *malloc_node()
{
	if(super->node_num > NODE_NUM_PER_BLOCK * BLOCK_NUM_FOR_NODE)
		return NULL;//No node memory

	super->node_num++;

	int i;
	for(i=1;i<=BLOCK_NUM_FOR_NODE;i++)
	{
		if(mem[i]==NULL)
		{
			mem[i] = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			Node_block * n_b=(Node_block*)mem[i];
			n_b->node_num=1;
			int j;
			for(j=0;j<NODE_NUM_PER_BLOCK;j++)
				n_b->bitmap[j]=0;

			n_b->bitmap[0]=1;
			super->node_block++;
		Node *temp=&(n_b->node[0]);
		temp->block=i;
			return temp;
		}
		else
		{
			Node_block *n_b=(Node_block *)mem[i];
			if(n_b->node_num < NODE_NUM_PER_BLOCK)
			{
				n_b->node_num++;
				int j;
				for(j=0;j<NODE_NUM_PER_BLOCK;j++)
				{
					if(n_b->bitmap[j]==0)
					{
						Node *temp=&(n_b->node[j]);
						temp->block=i;
						n_b->bitmap[j]=1;
						return temp;
					}
				}
			}
		}
	}
}

void free_node(Node *p)
{
	Node_block *n_b=(Node_block *)mem[(p->block)];
	n_b->node_num--;
	super->node_num--;

#ifdef DEBUG
	printf("\n\n %ld \n\n",&(n_b->node[0]));
  printf("\n\n %ld \n\n",p);
	printf("\n\n %ld \n\n",p - &(n_b->node[0]));
#endif

	int order=((void *)p - (void *)&(n_b->node[0])) / sizeof(Node);

	printf("\norder is %d\n",order);

	n_b->bitmap[order]=0;
	if(n_b->node_num ==0)
	{
		super->node_block--;
		munmap(mem[order], BLOCK_SIZE);
		mem[order]=NULL;
	}
}


int block_for_data=BLOCK_NUM_FOR_NODE+1;
int malloc_data(Node *p, const char *buf, int size)
{
#ifdef DEBUG
		printf("\n\n malloc_data size: %d \n",size);
		//printf(" malloc_data buf : %s",buf);
#endif
	if(size/CONTENT_SIZE_PER_BLOCK+1 > BLOCK_NUM_FOR_DATA)
		return 0;

	int first=1;
	int prev=0;
	int i=BLOCK_NUM_FOR_NODE+1;
	Data_block *prev_b;
	Data_block *curr_b;
	int offset=0;

	for(; size>0; size=size - CONTENT_SIZE_PER_BLOCK, offset+=CONTENT_SIZE_PER_BLOCK)
	{
		for(;block_for_data<BLOCK_NUM,mem[block_for_data]!=NULL;block_for_data++);

		if(mem[block_for_data]==NULL)
			i=block_for_data;
		else
		{
			while(mem[i]!=NULL)
				i++;
		}


#ifdef DEBUG
				printf("\n\nmalloc_data i: %d \n\n",i);
#endif

		mem[i]=mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

#ifdef DEBUG
						printf("\n\nmalloc_data mem[i]: %ld \n\n",mem[i]);
#endif

		super->data_num++;
		curr_b=(Data_block *)mem[i];
		if(first)
		{
			first=0;
			p->first_data=i;

			curr_b->prev_block=0;
			prev=i;
			prev_b=curr_b;
		}
		else
		{
			prev_b->next_block=i;
			curr_b->prev_block=prev;
			prev=i;
			prev_b=curr_b;
		}

		if(size>=CONTENT_SIZE_PER_BLOCK)
		{
			curr_b->size=CONTENT_SIZE_PER_BLOCK;
			memcpy(curr_b->content,((void *)buf)+offset,CONTENT_SIZE_PER_BLOCK);
		}
		else
		{
			curr_b->size=size;
			memcpy(curr_b->content,((void *)buf)+offset,size);
		}

	}
	curr_b->next_block=0;
	p->last_data=i;
	return 1;
}

//读出全部的数据，相当于全部放在内存里
void read_data(Node *p,char *buf)
{
	int curr=p->first_data;
	int last=p->last_data;
	int offset=0;
	Data_block *curr_b;
	while(curr!=last)
	{
		curr_b=(Data_block *)mem[curr];
		memcpy(((void*)buf)+offset,curr_b->content,CONTENT_SIZE_PER_BLOCK);
		curr=curr_b->next_block;
		offset+=CONTENT_SIZE_PER_BLOCK;
	}
	curr_b=(Data_block *)mem[curr];
	memcpy(((void *)buf)+offset,curr_b->content,curr_b->size);
}


void free_data(Node *p)
{
	int first=p->first_data;
	int last=p->last_data;
	p->first_data=0;
	p->last_data=0;
	while(first!=last)
	{
		int next=((Data_block *)mem[first])->next_block;
		munmap(mem[first], BLOCK_SIZE);
		mem[first]=NULL;
		super->data_num--;
		first=next;
	}
	munmap(mem[first], BLOCK_SIZE);
	mem[first]=NULL;
	super->data_num--;
}


void free_data_re(int first,int last)
{
	//int first=p->first_data;
	//int last=p->last_data;
	//p->first_data=0;
	//p->last_data=0;
	while(first!=last)
	{
		int next=((Data_block *)mem[first])->next_block;
		munmap(mem[first], BLOCK_SIZE);
		mem[first]=NULL;
		super->data_num--;
		first=next;
	}
	munmap(mem[first], BLOCK_SIZE);
	mem[first]=NULL;
	super->data_num--;
}

int malloc_data_re(int *first_data,int *last_data, const char *buf, int size)
{
#ifdef DEBUG
		printf("\n\n malloc_data_re size: %d \n",size);
		//printf(" malloc_data buf : %s",buf);
#endif
	if(size/CONTENT_SIZE_PER_BLOCK+1 > BLOCK_NUM_FOR_DATA)
		return 0;

	int first=1;
	int prev=0;
	int i=BLOCK_NUM_FOR_NODE+1;
	Data_block *prev_b;
	Data_block *curr_b;
	int offset=0;

	for(; size>0; size=size - CONTENT_SIZE_PER_BLOCK, offset+=CONTENT_SIZE_PER_BLOCK)
	{
		while(mem[i]!=NULL)
			i++;

#ifdef DEBUG
				printf("\n\nmalloc_data_re i: %d \n\n",i);
#endif

		mem[i]=mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

#ifdef DEBUG
						printf("\n\nmalloc_data_re mem[i]: %ld \n\n",mem[i]);
#endif

		super->data_num++;
		curr_b=(Data_block *)mem[i];
		if(first)
		{
			first=0;
			*first_data=i;

			curr_b->prev_block=0;
			prev=i;
			prev_b=curr_b;
		}
		else
		{
			prev_b->next_block=i;
			curr_b->prev_block=prev;
			prev=i;
			prev_b=curr_b;
		}

		if(size>=CONTENT_SIZE_PER_BLOCK)
		{
			curr_b->size=CONTENT_SIZE_PER_BLOCK;
			memcpy(curr_b->content,((void *)buf)+offset,CONTENT_SIZE_PER_BLOCK);
		}
		else
		{
			curr_b->size=size;
			memcpy(curr_b->content,((void *)buf)+offset,size);
		}

	}
	curr_b->next_block=0;
	*last_data=i;
	return 1;
}

int realloc_data_re(Node *p,const char *buf,int size,int offset)
{
	#ifdef DEBUG
	printf("\n\nrealloc_data_re \n");
	#endif
	if(offset > p->st.st_size)
		return 0;
/*
	char *new_buf;
	int new_size;
	if(offset+size < p->st.st_size)
	{
		new_buf=(char *)malloc(p->st.st_size-offset+10);
		memcpy(new_buf,buf,size);
		memcpy((void *)new_buf+size,)

	}
	else
	{
		new_buf=buf;
		new_size=size;
	}
*/

int valid_num=offset/CONTENT_SIZE_PER_BLOCK;
int last_left=offset%CONTENT_SIZE_PER_BLOCK;
if(last_left)
	valid_num++;
else
{
	if(!offset)
		last_left=0;
	else
		last_left=CONTENT_SIZE_PER_BLOCK;
}

int i;
int cur;
for(i=0,cur=p->first_data;i<valid_num-1;i++)
	cur=((Data_block *)mem[cur])->next_block;
int ret;

	if(size+offset< p->st.st_size)
	{
		Data_block *cur_b;
		cur_b=(Data_block *)mem[cur];
		if(size<=CONTENT_SIZE_PER_BLOCK - last_left)
		{

		memcpy((void *)cur_b->content+last_left,buf,size);
			cur_b->size=last_left+size;
			ret=1;
		}
		else
		{
			int has_copy=CONTENT_SIZE_PER_BLOCK-last_left;
			if(has_copy)
			{

				memcpy((void*)cur_b->content+last_left,buf,has_copy);
				cur_b->size=last_left+has_copy;
			}
			int next=((Data_block *)mem[cur])->next_block;
			int left_size=size-has_copy;
			int buf_off=has_copy;
			while(left_size>0)
			{
				cur_b=(Data_block *)mem[next];
				next=cur_b->next_block;
				if(left_size>=CONTENT_SIZE_PER_BLOCK)
				{
					memcpy(cur_b->content,(void *)buf+buf_off,CONTENT_SIZE_PER_BLOCK);
					left_size-=CONTENT_SIZE_PER_BLOCK;
					buf_off+=CONTENT_SIZE_PER_BLOCK;
				}
				else
				{
					memcpy(cur_b->content,(void *)buf+buf_off,left_size);
					left_size=0;
				}
			}

			ret=1;
			//ret=malloc_data_re(&first_data,&last_data,(void *)buf+has_copy,size-has_copy);
			//((Data_block *)mem[cur])->next_block=first_data;
			//p->last_data=last_data;
		}


	}
	else
	{
	if(cur!=p->last_data)
	{
#ifdef DEBUG
printf("realloc_data_re : free_data_re\n");
#endif

		int next=((Data_block *)mem[cur])->next_block;
		//free_data_re(next,p->last_data);
	}

	//int ret;
	int first_data,last_data;
	Data_block *cur_b;
	cur_b=(Data_block *)mem[cur];

	if(size<=CONTENT_SIZE_PER_BLOCK - last_left)
	{

	memcpy((void *)cur_b->content+last_left,buf,size);
		cur_b->size=last_left+size;
		ret=1;
	}
	else
	{
#ifdef DEBUG
printf("realloc_data_re : size > \n");
#endif

		int has_copy=CONTENT_SIZE_PER_BLOCK-last_left;
		if(has_copy)
		{

			memcpy((void*)cur_b->content+last_left,buf,has_copy);
			cur_b->size=last_left+has_copy;
		}

		ret=malloc_data_re(&first_data,&last_data,(void *)buf+has_copy,size-has_copy);
		((Data_block *)mem[cur])->next_block=first_data;
		p->last_data=last_data;
	}
/*
	if(new_buf!=buf)
		free(new_buf);
*/
}
	return ret;
}

/*
//将buf开始的size字节内容转移到offset偏移的文件中
int realloc_data(Node *p,const char *buf,int size,int offset)
{
#ifdef DEBUG
printf("\n\n realloc data\n");
#endif

	//char read[MAX_READ_NUM];
	//char write[MAX_READ_NUM];
	char *read=(char *)malloc(p->st.st_size+10);
	char *write=(char *)malloc(p->st.st_size+size+10);
	read_data(p,read);
#ifdef DEBUG
printf("realloc_data : read_data\n" );
printf("realloc_data : size %d\toffset %d\n",size,offset);
//printf("\n\nread: %s \n\n",read);
#endif

	memcpy(write,read,offset);

#ifdef DEBUG
printf("realloc_data : memcpy 1\n");
	//printf("\n\nwrite: %s \n\n",write);
#endif

	memcpy(((void *)write)+offset,buf,size);

#ifdef DEBUG
printf("realloc_data : memcpy 2\n");
	//printf("\n\nwrite: %s \n\n",write);
#endif

	free_data(p);
#ifdef DEBUG
		printf("realloc_data : free\n\n");
#endif


	int a=malloc_data(p, write, offset+size);
	free(read);
	free(write);
	return a;
}

*/

int pathType(const char *path)
{
	char *token;
	char dupPath[PATH_SIZE];
	strcpy(dupPath, path);
	token = strtok(dupPath, "/");

	//Check if it is  root
	if (token == NULL && strcmp(path, "/") == 0)
  {
		return 0;
	}
	else
	{
		int childFlag = 0;
		Node *temp = root;
		Node *tempChild = NULL;

		while (token != NULL)
		{
			tempChild = temp->child;
			//Check all child nodes
			while (tempChild)
			{
				if (strcmp(tempChild->name, token) == 0)
				{
					childFlag = 1;
					break;
				}
				//Look for next child in the linked list
				tempChild = tempChild->next;
			}

			token = strtok(NULL, "/");
			if (childFlag == 1)
			{
				if (token == NULL)
					return 0;
			}
			else
			{
				if (token)
					return -1; //invalid path
				else
					return 1; // valid path
			}
			//One level down
			temp = tempChild;
			childFlag = 0;
		}
	}
	return -1;
}

Node *findPathNode(const char *path)
{
	char *token;
	char dupPath[PATH_SIZE];
	strcpy(dupPath, path);
	token = strtok(dupPath, "/");
	//Check if it is  root
	if (token == NULL && strcmp(path, "/") == 0)
	{
		return root;
	}
	else
	{
		int childFlag = 0;
		Node *temp = root;
		Node *tempChild = NULL;
		while (token != NULL)
		{
			tempChild = temp->child;
			//Check all child nodes
			while (tempChild)
			{
				if (strcmp(tempChild->name, token) == 0)
				{
					childFlag = 1;
					break;
				}
				//Look for next child in the linked list
				tempChild = tempChild->next;
			}
			if (childFlag == 1)
			{
				strcpy(newFile, token);
				token = strtok(NULL, "/");
				if (token == NULL)
				{
					if (tempChild == NULL)
						return temp;
					else
						return tempChild;
				}
			}
			else
			{
				strcpy(newFile, token);
				return temp;
			}
			temp = tempChild;
		}
	}
	return NULL;
}



static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	Node *node;
	size_t fileSize;
	node = findPathNode(path);
	if (node->type == 'd') {
		return -EISDIR;
	}
	fileSize = node->st.st_size;

	if (fileSize < offset)
	{
		size = 0;
	}
	else if (offset < fileSize)
	{
		if (offset + size > fileSize)
		{
			size = fileSize - offset;
		}
		//char read[MAX_READ_NUM];
		char *read=(char *)malloc(node->st.st_size+10);
		read_data(node,read);
		memcpy(buf,&read[offset],size);
		//memcpy(buf, node->contents + offset, size);
		free(read);
	}
	if (size > 0)
	{
		node->st.st_atime = time(NULL);
	}
	return size;
}


static int ramdisk_unlink(const char *path)
{
	int type;
	Node *node;
	Node *parent;
	Node *childNode;
	type = pathType(path);
	if (type != 0)
	{
		return -ENOENT;
	}
	node = findPathNode(path);
	parent = node->parent;
	if (parent->child == node)
	{
		if (node->next == NULL)
		{
			//Node to be deleted is only child of the parent
			parent->child = NULL;
		}
		else
		{
			//Not the only child
			parent->child = node->next;
		}
	}
	else
	{
		//Find intermediate node in the linked list
		for (childNode = parent->child; childNode != NULL;
				childNode = childNode->next)
		{
			if (childNode->next == node)
			{
				childNode->next = node->next;
				break;
			}
		}
	}

	if (node->st.st_size > 0)
	{
		free_data(node);
		free_node(node);
	} else {
		free_node(node);
	}
	return 0;
}

static int ramdisk_write(const char *path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi)
{
	size_t fileSize;
	Node *node;
	node = findPathNode(path);
	if (node->type == 'd')
	{
		return -EISDIR;
	}

	fileSize = node->st.st_size;
	if (size > 0)
	{
		if (fileSize == 0)
		{
			offset = 0;

			//node->contents = (char *) malloc(sizeof(char) * size);
			//memcpy(node->contents + offset, buf, size);
			int m_ret=malloc_data(node,buf,size);
			if(m_ret==0)
			{
				return -ENOSPC;
			}
			node->st.st_size = offset + size;
			node->st.st_ctime = time(NULL);
			node->st.st_mtime = time(NULL);
		}
		else
		{
			if (offset > fileSize)
			{
				offset = fileSize;
			}
			//char *more = (char *) realloc(node->contents,
			//		sizeof(char) * (offset + size));
			#ifdef DEBUG
			printf("\n\n realloc data in write\n");
			printf("realloc_data node %ld",node);
			//printf("write : buf:\n%s\n",buf);
			#endif
			int more=realloc_data_re(node,buf,size,offset);
			if (more == 0)
			{
				return -ENOSPC;
			}
			else
			{
				//node->contents = more;
				// Change pointer of file contents to a newly reallocated memory pointer
				//memcpy(node->contents + offset, buf, size);

				node->st.st_size = offset + size;
				node->st.st_ctime = time(NULL);
				node->st.st_mtime = time(NULL);
			}
		}
	}
	return size;
}




static int ramdisk_getattr(const char *path, struct stat *st)
{
	Node *node;
	int ret = 0;
	int type = pathType(path);
	if (type == 0) {
		node = findPathNode(path);

		st->st_uid = node->st.st_uid;
		st->st_gid = node->st.st_gid;
		st->st_atime = node->st.st_atime;
		st->st_mtime = node->st.st_mtime;
		st->st_ctime = node->st.st_ctime;
		st->st_nlink = node->st.st_nlink;
		st->st_size = node->st.st_size;
		st->st_mode = node->st.st_mode;

	}
	else
	{
		ret = -ENOENT;
	}
	return ret;
}


static int ramdisk_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info* fi)
{
	(void) offset;
	(void) fi;
	int ret = 0;

	Node *node;
	Node *childNode = NULL;

	int type = pathType(path);
	if ((type == 0) || (type == 1))
	{
		node = findPathNode(path);
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		childNode = node->child;
		while (childNode != NULL)
		{
			filler(buf, childNode->name, NULL, 0);
			childNode = childNode->next;
		}
		node->st.st_atime = time(NULL);
		ret = 0;
	}
	else
		ret = -ENOENT;

	return ret;
}

static int ramdisk_opendir(const char* path, struct fuse_file_info* fi)
{
	return 0;
}

static int ramdisk_open(const char* path, struct fuse_file_info* fi)
{
	int ret = 0;
	int type = pathType(path);
	if (type == 0)
	{
		ret = 0;
	}
	else
		ret = -ENOENT;
	return ret;
}

static int ramdisk_mkdir(const char* path, mode_t mode)
{
	Node *node = findPathNode(path);
	Node *temp;

	temp = malloc_node();
	#ifdef DEBUG
	printf("mkdir\n");
	#endif
	//temp = (Node *) malloc(sizeof(Node));
	if (temp == NULL)
	{
		return -ENOSPC;
	}

	strcpy(temp->name, newFile);
	temp->type = 'd';
	temp->parent = node;
	temp->next = NULL;
	temp->child = NULL;

	temp->st.st_uid = getuid();
	temp->st.st_mode = S_IFDIR | 0755;
	temp->st.st_gid = getgid();
	temp->st.st_nlink = 2;
	temp->st.st_atime = time(NULL);
	temp->st.st_mtime = time(NULL);
	temp->st.st_ctime = time(NULL);
	temp->st.st_size = 0;

	Node *childNode;
	childNode = node->child;
	if (childNode != NULL)
	{
		while (childNode->next != NULL) {
			childNode = childNode->next;
	  }
		childNode->next = temp;
	}
	else
	{
		node->child = temp;
	}
	node->st.st_nlink += 1;
	return 0;
}

static int ramdisk_rmdir(const char *path)
{
	int type;
	Node *node;
	Node *parent;
	Node *childNode;
	type = pathType(path);

	if (type == 0) {
		node = findPathNode(path);
		if (node->child != NULL)
		{
			return -ENOTEMPTY;
		}
		else
		{
			parent = node->parent;
			if (parent->child == node)
			{
				if (node->next == NULL) {
					//Node to be deleted is only child of the parent
					parent->child = NULL;
				}
				else
				{
					//Not the only child
					parent->child = node->next;
				}
			}
			else
			{
				//Find intermediate node in the linked list
				for (childNode = parent->child; childNode != NULL; childNode =
						childNode->next)
						{
					if (childNode->next == node)
					{
						childNode->next = node->next;
						break;
					}
				}
			}
			free_node(node);
			parent->st.st_nlink -= 1;
			return 0;
		}
	}
	else
		return -ENOENT;
}

static int ramdisk_create(const char *path, mode_t mode,
		struct fuse_file_info *fi)
{

#ifdef DEBUG
printf("\n\ncreate\n\n");
#endif

	Node *node = findPathNode(path);
	Node *temp;
	temp = malloc_node();
//temp = (Node *) malloc(sizeof(Node));
#ifdef DEBUG
	printf("\n\n node is %ld \n\n",temp);
#endif

	if (temp==NULL)
	{
		return -ENOSPC;
	}
	strcpy(temp->name, newFile);
	temp->type = 'f';
	temp->first_data=0;
	temp->last_data=0;

	temp->parent = node;
	temp->next = NULL;
	temp->child = NULL;

	temp->st.st_uid = getuid();
	temp->st.st_mode = S_IFREG | mode;
	temp->st.st_gid = getgid();
	temp->st.st_nlink = 1;
	temp->st.st_atime = time(NULL);
	temp->st.st_mtime = time(NULL);
	temp->st.st_ctime = time(NULL);
	temp->st.st_size = 0;

	Node *childNode;
	childNode = node->child;
	if (childNode != NULL)
	{
		while (childNode->next != NULL)
		{
			childNode = childNode->next;
		}
		childNode->next = temp;
	}
	else
	{
		node->child = temp;
	}
	return 0;
}



static int ramdisk_truncate(const char *path, off_t size)
{
	int type;
	type = pathType(path);
	if (type == 0)
	{
		Node *node = findPathNode(path);
		if(size>node->st.st_size)
			return 0;
		int valid_num=size/CONTENT_SIZE_PER_BLOCK;
		int last_left=size%CONTENT_SIZE_PER_BLOCK;
		if(last_left)
			valid_num++;
		else
		{
			if(!size)
				last_left=0;
			else
				last_left=CONTENT_SIZE_PER_BLOCK;
		}

		int i;
		int cur;

		for(i=0,cur=node->first_data;i<valid_num-1;i++)
			cur=((Data_block *)mem[cur])->next_block;

		if(cur!=node->last_data)
		{
				int next=((Data_block *)mem[cur])->next_block;
				free_data_re(next,node->last_data);
		}
		Data_block *cur_b=(Data_block *)mem[cur];
		node->last_data=cur;
		cur_b->size=last_left;
		cur_b->next_block=0;
		node->st.st_size=size;
		node->st.st_ctime = time(NULL);
		node->st.st_mtime = time(NULL);
		return 0;
	}
 	else
	{
		return -ENOENT;
	}
}

static int ramdisk_utimens(const char* path, const struct timespec ts[2])
{
	int type;
	type = pathType(path);
	if (type == 0)
	{
		return 0;
	}
	else
	{
		return -ENOENT;
	}
}



void ramdisk_init() {
	//mem initial
	int i;
	for(i=0;i<BLOCK_NUM;i++)
		mem[i]=NULL;

	//Super
	//void *
	mem[0] = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	super =(Super *)mem[0];
	super->node_num=0;
	super->node_block=0;
	super->data_num=0;

	//root
	root = malloc_node();
	//root = (Node *) malloc(sizeof(Node));
//初始化
	strcpy(root->name, "/");
	root->type = 'd';
	root->first_data=0;
	root->last_data=0;
	root->parent = NULL;
	root->next = NULL;
	root->child = NULL;

	root->st.st_uid = getuid();
	root->st.st_mode = S_IFDIR | 0755;
	root->st.st_gid = getgid();
	root->st.st_nlink = 2;
	root->st.st_atime = time(NULL);
	root->st.st_mtime = time(NULL);
	root->st.st_ctime = time(NULL);
}


static struct fuse_operations ramdisk_oper = {
				.getattr = ramdisk_getattr,
				.readdir = ramdisk_readdir,
				.open = ramdisk_open,
				.opendir = ramdisk_opendir,
				.read = ramdisk_read,
				.write = ramdisk_write,
				.mkdir = ramdisk_mkdir,
				.rmdir = ramdisk_rmdir,
				.create = ramdisk_create,
				.truncate = ramdisk_truncate,
				.unlink = ramdisk_unlink,
				.utimens = ramdisk_utimens,
			};

int main(int argc, char *argv[])
{
	ramdisk_init();
	fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}
