#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <crypt.h>

extern int rw_char(int rw,int dev, char * buf, int count);
extern int read_pipe(struct m_inode * inode, char * buf, int count);
extern int write_pipe(struct m_inode * inode, char * buf, int count);
extern int block_read(int dev, off_t * pos, char * buf, int count);
extern int block_write(int dev, off_t * pos, char * buf, int count);
extern int file_read(struct m_inode * inode, struct file * filp,
		char * buf, int count);
extern int file_write(struct m_inode * inode, struct file * filp,
		char * buf, int count);

int sys_switch_case(int fd)
{
    struct file* file;
    struct m_inode* inode;
    if (fd>=NR_OPEN || !(file=current->filp[fd]))
		return -EINVAL;
	inode = file->f_inode;
    if(S_ISREG(inode->i_mode))
        return file_switch_case(inode);
    return -1;
}

static void printbh(){
    int i;
    struct buffer_head * cr_bh = bread(0x301,40948);
    struct file_hash* fh = (struct file_hash*)cr_bh->b_data;
    for(i=0;i<1024/FH_SIZE;i++){
        //fh[i].inum=0;
        printk("%hu %lu / ",fh[i].inum,fh[i].hash);
    }
    printk("\n");
    cr_bh->b_dirt=1;
    brelse(cr_bh);
}

/*static void printkey(void){
    int i;
    for(i=0;i<1024;i++){
        printk("%c",current->key[i]+'0');
    }
    printk("\n");
}*/

int sys_keyset(char* key, int len, int mode)
{
    //printk("key: %s len: %d mode: %d\n",key,len,mode);
    if(!key){
        if(len) return rek^=1;
        printbh();
        if(mode) return *current->key=0;
        globtime = 0;
        return *globkey=0;
    }
    if(key==1&&len==1){
        if(mode) mask='*';
        else mask=0;
        return 0;
    }
    if(len>3 && !(len & (len - 1))){
        char * tmpkey = globkey;
        unsigned long * tmptime = &globtime;
        int * tmplen = &globlen;
        if(mode){
            tmpkey=current->key;
            tmptime=&current->keytime;
            tmplen=&current->keylen;
        }
        int i;
        for(i = 0; i < len; i++)
            tmpkey[i] = get_fs_byte(key + i);
        *tmptime=CURRENT_TIME+45;
        if(!mode) *tmptime+=75;
        return *tmplen=len;
    } return -1;
}

int sys_crypt(int fd, int mode){
    if(!keycheck()) return -1;
    struct file* file;
    struct m_inode* inode;
    if (fd>=NR_OPEN || !(file=current->filp[fd]))
        return -EINVAL;
    inode = file->f_inode;
    if(S_ISREG(inode->i_mode))
        return file_crypt(inode,mode);
    if(S_ISDIR(inode->i_mode)){
        dir_crypt(inode,mode);
        return S_IFDIR;
    }
    return -1;
}

int sys_lseek(unsigned int fd,off_t offset, int origin)
{
	struct file * file;
	int tmp;

	if (fd >= NR_OPEN || !(file=current->filp[fd]) || !(file->f_inode)
	   || !IS_BLOCKDEV(MAJOR(file->f_inode->i_dev)))
		return -EBADF;
	if (file->f_inode->i_pipe)
		return -ESPIPE;
	switch (origin) {
		case 0:
			if (offset<0) return -EINVAL;
			file->f_pos=offset;
			break;
		case 1:
			if (file->f_pos+offset<0) return -EINVAL;
			file->f_pos += offset;
			break;
		case 2:
			if ((tmp=file->f_inode->i_size+offset) < 0)
				return -EINVAL;
			file->f_pos = tmp;
			break;
		default:
			return -EINVAL;
	}
	return file->f_pos;
}

int sys_read(unsigned int fd,char * buf,int count)
{
	struct file * file;
	struct m_inode * inode;

	if (fd>=NR_OPEN || count<0 || !(file=current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	verify_area(buf,count);
	inode = file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode&1)?read_pipe(inode,buf,count):-1;
	if (S_ISCHR(inode->i_mode))
		return rw_char(READ,inode->i_zone[0],buf,count);
	if (S_ISBLK(inode->i_mode))
		return block_read(inode->i_zone[0],&file->f_pos,buf,count);
	if (S_ISDIR(inode->i_mode) || S_ISREG(inode->i_mode)) {
		if (count+file->f_pos > inode->i_size)
			count = inode->i_size - file->f_pos;
		if (count<=0)
			return 0;
		return file_read(inode,file,buf,count);
	}
	printk("(Read)inode->i_mode=%06o\n\r",inode->i_mode);
	return -EINVAL;
}

int sys_write(unsigned int fd,char * buf,int count)
{
	struct file * file;
	struct m_inode * inode;

	if (fd>=NR_OPEN || count <0 || !(file=current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	inode=file->f_inode;
	if (inode->i_pipe)
		return (file->f_mode&2)?write_pipe(inode,buf,count):-1;
	if (S_ISCHR(inode->i_mode))
		return rw_char(WRITE,inode->i_zone[0],buf,count);
	if (S_ISBLK(inode->i_mode))
		return block_write(inode->i_zone[0],&file->f_pos,buf,count);
	if (S_ISREG(inode->i_mode))
		return file_write(inode,file,buf,count);
	printk("(Write)inode->i_mode=%06o\n\r",inode->i_mode);
	return -EINVAL;
}
