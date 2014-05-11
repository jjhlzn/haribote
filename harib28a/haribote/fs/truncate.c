/*
 *  linux/fs/truncate.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <linux/sched.h>

#include "bootpack.h"
#include <sys/stat.h>
#include "fs.h"

/* �ͷ�����һ�μ�ӿ顣 */
//����dev���ļ�ϵͳ�����豸���豸�ţ�block���߼����
static void free_ind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	//��ȡһ�μ�ӿ飬���ͷ����ϱ���ʹ�õ������߼��飬Ȼ���ͷ�һ�μ�ӿ�Ļ���顣����free_block()
    //�����ͷ��豸��ָ���߼���ŵĴ��̿�
	if ( (bh=bread(dev,block)) ) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_block(dev,*p);
		brelse(bh);
	}
	free_block(dev,block);
}

/* �ͷ����ж��μ�ӿ� */
static void free_dind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if ( (bh=bread(dev,block)) ) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_ind(dev,*p);
		brelse(bh);
	}
	free_block(dev,block);
}

/* �ض��ļ����ݺ��� */
//���ڵ��Ӧ���ļ����Ƚ�Ϊ0,���ͷ�ռ�õ��豸�ռ�
void truncate(struct m_inode * inode)
{
	int i;
	//�����ж�ָ��i�ڵ���Ч�ԡ�������ǳ����ļ���Ŀ¼�ļ����򷵻ء�
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	//�ͷ����ݿ�
	for (i=0;i<7;i++)
		if (inode->i_zone[i]) {
			free_block(inode->i_dev,inode->i_zone[i]);
			inode->i_zone[i]=0;
		}
	free_ind(inode->i_dev,inode->i_zone[7]);
	free_dind(inode->i_dev,inode->i_zone[8]);
	inode->i_zone[7] = inode->i_zone[8] = 0;
	inode->i_size = 0;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}

