#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <string.h>
#include <crypt.h>

//crypt.h fh size, fh=NULL vs fh[i]=0
//sys_keyset else if return
//sys_chdir reteurn (0)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static void buffer_switch_case(char* s, int l){
    int i;
    for(i=0;i<l;i++){
        if(s[i]>='a'&&s[i]<='z')
            s[i]-=32;
        else if(s[i]>='A'&&s[i]<='Z')
            s[i]+=32;
    }
}

int file_switch_case(struct m_inode * inode){
    int k=0,nr=-1;
    struct buffer_head* bh;
    while((nr=bmap(inode, k++))&&(bh=bread(inode->i_dev, nr))){
        buffer_switch_case(bh->b_data,1024);
        bh->b_dirt=1; brelse(bh);
    } return nr;
}

char* cryptkey;
int keylen;
int rek;
char globkey[1024];
int globlen;
unsigned long globtime;

static void flag_crypt(struct m_inode * ind,int md){
    if(md)  ind->i_mode |= ~0x0EFFF;
    else ind->i_mode &= 0x0EFFF;
    ind->i_dirt = 1;
}

static unsigned long djb2(char* str){
       unsigned long hash = 5381;
       int c;
       while (c = *str++)
           hash = ((hash<<5)+hash)^c;
       return hash;
}

int list_crypt(unsigned short inum,char* key,int mode){
    struct buffer_head* bh = bread(0x301,40948);
    struct file_hash* fh = (struct file_hash*)bh->b_data;
    unsigned long keyhash;
    if(key) keyhash = djb2(key);
    if(mode){
        while(fh->inum) fh++;
        fh->inum=inum;
        fh->hash=keyhash;
    } else {
        int i;
        for(i=0;i<1024/FH_SIZE;i++){
            if(fh[i].inum==inum){
                if(!key) mode=1;
                else if(fh[i].hash==keyhash)
                    fh[i].inum=0,fh[i].hash=0,mode=1;
                else mode=0;
            }
        }
    }
    bh->b_dirt=1;
    brelse(bh);
    return mode;
}

static void swap(char* x, char* y){
    char tmp = *x;
    *x = *y; *y = tmp;
}

static void get_order(char* ord){
    int sorted = 0, i;
    char arr[keylen];
    for(i=0;i<keylen;i++)
        ord[i]=i;
    strncpy(arr, cryptkey, keylen);
    while(!sorted){
		sorted=1;
		for(i=0;i<keylen-1;i++){
            if(arr[i] > arr[i+1]){
                swap(arr+i,arr+i+1);
                swap(ord+i,ord+i+1);
                sorted = 0;
			}
		}
    }
}

void buff_crypt(char* blok,int mode){
    char key[keylen];
    if(mode) strncpy(key,cryptkey,keylen);
    else get_order(key);
    int sorted = 0, i,j;
	while(!sorted){
		sorted=1;
		for(i=0;i<keylen-1;i++){
			if(key[i] > key[i+1]){
                swap(key+i,key+i+1);
                for(j=i;j<1024;j+=keylen)
                    swap(blok+j,blok+j+1);
                sorted = 0;
			}
		}
    }
}

/*static int check_crypt(struct m_inode* inode, int mode){
    if(mode&&S_ISENCR(inode->i_mode) ||
        !mode&&!S_ISENCR(inode->i_mode))
        return 0;
    if(!list_crypt(inode->i_num,cryptkey,mode))
        return 0;
    return 1;
}*/


int keycheck(){
    if(!*current->key){
        if(!*globkey||!globtime) return 0;
        if(CURRENT_TIME>globtime){
            sys_keyset(0,0,0);
            return 0;
        }
        cryptkey=globkey;
        keylen=globlen;
    } else if(CURRENT_TIME>current->keytime){
        sys_keyset(0,0,1);
        return 0;
    } else {
        cryptkey=current->key;
        keylen=current->keylen;
    }
    return 1;
}

int file_crypt(struct m_inode* inode, int mode){
    if(mode&&list_crypt(inode->i_num,0,0)) return -1;
    if(!list_crypt(inode->i_num,cryptkey,mode)) return -1;
    int k=0,nr=-1;
    struct buffer_head* bh;
    while((nr=bmap(inode, k++))&&(bh=bread(inode->i_dev, nr))){
        buff_crypt(bh->b_data,mode);
        bh->b_dirt=1; brelse(bh);
    }
    return nr;
}

void dir_crypt(struct m_inode * ind, int mode){
    if(mode&&list_crypt(ind->i_num,0,0)) return;
    if(!list_crypt(ind->i_num,cryptkey,mode)) return;
    struct buffer_head* bh = read_file_block(ind,0);
    if(!mode){
        flag_crypt(ind, mode);
        buff_crypt(bh->b_data, mode);
        bh->b_dirt=1;
    }
    if(ind->i_size<=32) goto prazan;
    struct dir_entry* dir=(struct dir_entry*)bh->b_data;
    dir+=2;
    struct m_inode * tmp;
    while(dir->inode){
        tmp = iget(DISK,dir++->inode);
        if(S_ISREG(tmp->i_mode))
            file_crypt(tmp,mode);
        else if(S_ISDIR(tmp->i_mode)&&!rek)
            dir_crypt(tmp, mode);
        iput(tmp);
    } prazan:
    if(mode){
        flag_crypt(ind, mode);
        buff_crypt(bh->b_data, mode);
        bh->b_dirt=1;
    }
    brelse(bh);
}

int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left,chars,nr,enc=0;
	struct buffer_head * bh;
    if(keycheck()&&list_crypt(inode->i_num,cryptkey,0)) enc=1;
    if ((left=count)<=0)
		return 0;
	while (left) {
		if ((nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE))) {
            if (!(bh=bread(inode->i_dev,nr))) break;
            if(enc) buff_crypt(bh->b_data, 0);
		} else
			bh = NULL;
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE-nr , left );
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			char * p = nr + bh->b_data;
			while (chars-->0)
				put_fs_byte(*(p++),buf++);
            if(enc){
                buff_crypt(bh->b_data, 1);
                list_crypt(inode->i_num,cryptkey,1);
            }
			brelse(bh);
		} else {
			while (chars-->0)
				put_fs_byte(0,buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count-left)?(count-left):-ERROR;
}

int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;
	int block,c;
	struct buffer_head * bh;
	char * p;
	int i=0,enc=0;
    if(keycheck()){
        if(!list_crypt(inode->i_num,cryptkey,0)) enc=1;
        else enc=2;
    }

/*
 * ok, append may not work when many processes are writing at the same time
 * but so what. That way leads to madness anyway.
 */
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	while (i<count) {
		if (!(block = create_block(inode,pos/BLOCK_SIZE)))
			break;
		if (!(bh=bread(inode->i_dev,block)))
			break;
        if(enc==2){
            buff_crypt(bh->b_data,0);
        }
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE-c;
		if (c > count-i) c = count-i;
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-->0)
			*(p++) = get_fs_byte(buf++);
        if(enc){
            buff_crypt(bh->b_data,1);
            list_crypt(inode->i_num,cryptkey,1);
        }
		brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i?i:-1);
}
