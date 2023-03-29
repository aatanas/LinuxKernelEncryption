#ifndef CRYPT_H_INCLUDED
#define CRYPT_H_INCLUDED
#define FH_SIZE sizeof(struct file_hash)
//int fh_size =

#define DISK 0x301
#define ROOT_ON \
	current->root = iget(DISK,1);\
	current->pwd = current->root;

#define ROOT_OFF \
	iput(current->root);\
	current->root = NULL;\
    current->pwd = NULL;

extern char globkey[1024];
extern int globlen;
extern unsigned long globtime;
extern int keycheck(void);
extern char mask;
extern int rek;
extern void dir_crypt(struct m_inode* inode, int mode);
extern int file_crypt(struct m_inode* inode, int mode);
extern int list_crypt(unsigned short inum,char* key,int mode);
extern char* cryptkey;
extern int keylen;

struct file_hash {
    unsigned short inum;
    unsigned long hash;
};

#endif
