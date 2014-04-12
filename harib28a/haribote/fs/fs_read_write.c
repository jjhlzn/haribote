/*************************************************************************//**
 *****************************************************************************
 * @file   read_write.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/
#include "bootpack.h"
#include "fs.h"

extern int FSBUF_SIZE;
extern u8 *		fsbuf;

/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 * 
 * @return How many bytes have been read/written.
 *****************************************************************************/
PUBLIC int do_rdwt(MESSAGE * msg,struct TASK *pcaller)
{
	MESSAGE fs_msg = *msg;
	int fd = msg->FD;	/**< file descriptor. */
	void * buf = fs_msg.BUF;/**< r/w buffer */
	int len = fs_msg.CNT;	/**< r/w bytes */

	int src = fs_msg.source;		/* caller proc nr. */
	debug("pos of fd[%d] = %d, inode_number = %d",fd,pcaller->filp[fd]->fd_pos,pcaller->filp[fd]->fd_inode->i_num);
	//sprintf(str,"fd = %d, len = %d", fd, len);
	assert((pcaller->filp[fd] >= &f_desc_table[0]) &&
	       (pcaller->filp[fd] < &f_desc_table[NR_FILE_DESC]));

	//if (!(pcaller->filp[fd]->fd_mode & O_RDWR))
	//	return -1;

	int pos = pcaller->filp[fd]->fd_pos;

	struct inode * pin = pcaller->filp[fd]->fd_inode;
	debug("pin->i_size = %d",pin->i_size);
	//int imode = pin->i_mode & I_TYPE_MASK;
	debug("imode = %d", pin->i_mode);
	assert( (pin >= &inode_table[0] && pin < &inode_table[NR_INODE]) || pin->i_mode == I_CHAR_SPECIAL );
	
	if (pin->i_mode == I_CHAR_SPECIAL) {
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		struct CONSOLE *cons = pcaller->cons;
		if(cons == NULL)
			return 0;

		struct SHEET *sht = cons->sht;
		if(sht == NULL)
			return 0;

		if(fs_msg.type == READ){
			char *charBuf = (char *)buf;
			int ch = read_from_keyboard(pcaller, 1);
			if( ch == -1){
				charBuf[0] = 0;
			}else{
				charBuf[0] = ch;
				charBuf[1] = 0;
			}
		}else{
			
			char *msg = (char *)buf;
			msg[len] = 0;
			cons_putstr0(cons, msg);
		}
		return strlen(buf);
	}
	else {
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		int pos_end;
		if (fs_msg.type == READ){
			pos_end = min(pos + len, pin->i_size);
		}
		else		/* WRITE */
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE);

		debug("pin->i_size = %d, pos_end = %d",pin->i_size, pos_end);
		
		int off = pos % SECTOR_SIZE;
		int rw_sect_min=pin->i_start_sect+(pos>>SECTOR_SIZE_SHIFT);
		int rw_sect_max=pin->i_start_sect+(pos_end>>SECTOR_SIZE_SHIFT);

		int chunk = min(rw_sect_max - rw_sect_min + 1,
				FSBUF_SIZE >> SECTOR_SIZE_SHIFT);

		int bytes_rw = 0;
		int bytes_left = len;
		int i;

		for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
			/* read/write this amount of bytes every time */
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
			debug("bytes = %d",bytes);
			rw_sector(DEV_READ,
				  pin->i_dev,
				  i * SECTOR_SIZE,
				  chunk * SECTOR_SIZE,
				  -1,
				  fsbuf);

			if (fs_msg.type == READ) {
				phys_copy((void*)va2la(src, buf + bytes_rw),
					  (void*)va2la(-1, fsbuf + off),
					  bytes);
			}
			else {	/* WRITE */
				
				phys_copy((void*)va2la(-1, fsbuf + off),
					  (void*)va2la(src, buf + bytes_rw),
					  bytes);

				rw_sector(DEV_WRITE,
					  pin->i_dev,
					  i * SECTOR_SIZE,
					  chunk * SECTOR_SIZE,
					  -1,
					  fsbuf);
			}
			off = 0;
			bytes_rw += bytes;
			pcaller->filp[fd]->fd_pos += bytes;
			debug("pos of fd[%d] = %d",fd,pcaller->filp[fd]->fd_pos);
			
			bytes_left -= bytes;
		}

		if (pcaller->filp[fd]->fd_pos > pin->i_size) {
			/* update inode::size */
			pin->i_size = pcaller->filp[fd]->fd_pos;

			/* write the updated i-node back to disk */
			sync_inode(pin);
		}

		return bytes_rw;
	}
}
